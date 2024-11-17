// my_predictor.h
// Optimized perceptron predictor
#include <cstring>
#include <vector>
#include <cmath>
#include <cstdint>
using namespace std;

class my_update : public branch_update {
public:
    int y;          // Perceptron Output
    int index;      // Index of perceptron in table
    bool prediction;
};

class my_predictor : public branch_predictor {
public:
    // Predictor params
    static const int HISTORY_LENGTH = 100;  
    static const int NUM_PERCEPTRONS = 16384; 
    static const int WEIGHT_MAX = 127;         // Max val for int8_t
    static const int WEIGHT_MIN = -128;        // Min val for int8_t
    static const int THRESHOLD = (int)(1.6 * HISTORY_LENGTH + 11); 

    my_update u;
    branch_info bi;
    int ghistory[HISTORY_LENGTH];
    // using int8_t to reduce size
    vector<vector<int8_t> > perceptrons;

    my_predictor() : perceptrons(NUM_PERCEPTRONS, vector<int8_t>(HISTORY_LENGTH + 1, 0)) {
        // Initialize global history to -1 (not taken)
        fill_n(ghistory, HISTORY_LENGTH, -1);
    }

    branch_update* predict(branch_info& b) {
        bi = b;

        if (b.br_flags & BR_CONDITIONAL) {
            // Compute perceptron index
            u.index = ((b.address >> 2) ^ (b.address >> 5) ^ (b.address >> 7) ^ history_hash()) % NUM_PERCEPTRONS;
            // Fetch perceptron weights
            vector<int8_t>& weights = perceptrons[u.index];
            // Compute dot product of weights and global history
            int y = weights[0]; // Bias weight
            for (int i = 0; i < HISTORY_LENGTH; i++) {
                y += weights[i + 1] * ghistory[i];
            }
            u.y = y;
            u.prediction = (y >= 0);
            u.direction_prediction(u.prediction);
        } else {
            u.direction_prediction(true); // Default prediction
        }

        u.target_prediction(0);
        return &u;
    }

    void update(branch_update* u_ptr, bool taken, unsigned int target) {
        if (bi.br_flags & BR_CONDITIONAL) {
            my_update* u = static_cast<my_update*>(u_ptr);
            // Convert taken/not taken to 1/-1
            int t = taken ? 1 : -1;
            // Train perceptron if the prediction was wrong or |y| <= threshold
            if ((u->prediction != taken) || (abs(u->y) <= THRESHOLD)) {
                vector<int8_t>& weights = perceptrons[u->index];
                // Update weight
                weights[0] = saturate(weights[0] + t);
                // Update weights for each history bit
                for (int i = 0; i < HISTORY_LENGTH; i++) {
                    weights[i + 1] = saturate(weights[i + 1] + t * ghistory[i]);
                }
            }
            // Update global history
            for (int i = 0; i < HISTORY_LENGTH - 1; i++) {
                ghistory[i] = ghistory[i + 1];
            }
            ghistory[HISTORY_LENGTH - 1] = t;
        }
    }

private:
    // Saturate weight between weight_min - weight_max
    int8_t saturate(int value) const {
        if (value > WEIGHT_MAX) {
            return WEIGHT_MAX;
        } else if (value < WEIGHT_MIN) {
            return WEIGHT_MIN;
        } else {
            return value;
        }
    }
    // hash func
    unsigned int history_hash() const {
        unsigned int hash = 0;
        for (int i = 0; i < HISTORY_LENGTH; i++) {
            hash = ((hash << 1) | (ghistory[i] > 0 ? 1 : 0)) & 0xFFFF;
        }
        return hash;
    }
};
