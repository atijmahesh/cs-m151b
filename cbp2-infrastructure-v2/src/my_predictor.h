// my_predictor.h
// Enhanced branch predictor with hybrid gshare and bimodal predictors,
// a chooser mechanism, and 3-bit saturating counters.

class my_update : public branch_update {
public:
    unsigned int gshare_index;
    unsigned int bimodal_index;
    unsigned int chooser_index;
    bool gshare_prediction;
    bool bimodal_prediction;
};

class my_predictor : public branch_predictor {
public:
    // Predictor parameters
    #define HISTORY_LENGTH 20      // History length for gshare
    #define GSHARE_TABLE_BITS 17   // Table size for gshare
    #define BIMODAL_TABLE_BITS 17  // Table size for bimodal predictor
    #define CHOOSER_TABLE_BITS 12  // Table size for chooser 
    #define COUNTER_MAX 7          // Max value for 3-bit saturating counter

    my_update u;
    branch_info bi;
    unsigned int history;

    // Prediction tables
    unsigned char gshare_table[1 << GSHARE_TABLE_BITS];
    unsigned char bimodal_table[1 << BIMODAL_TABLE_BITS];
    unsigned char chooser_table[1 << CHOOSER_TABLE_BITS];

    my_predictor(void) : history(0) {
        memset(gshare_table, COUNTER_MAX / 2, sizeof(gshare_table));  // Initialize to weakly taken
        memset(bimodal_table, COUNTER_MAX / 2, sizeof(bimodal_table));
        memset(chooser_table, COUNTER_MAX / 2, sizeof(chooser_table));  // Initialize chooser to prefer gshare
    }

    branch_update* predict(branch_info& b) {
        bi = b;

        if (b.br_flags & BR_CONDITIONAL) {
            // gshare prediction
            unsigned int gshare_addr_bits = (b.address >> 2) & ((1 << GSHARE_TABLE_BITS) - 1);
            u.gshare_index = (history ^ gshare_addr_bits) & ((1 << GSHARE_TABLE_BITS) - 1);
            unsigned char gshare_counter = gshare_table[u.gshare_index];
            u.gshare_prediction = (gshare_counter > COUNTER_MAX / 2);

            // bimodal prediction
            u.bimodal_index = (b.address >> 2) & ((1 << BIMODAL_TABLE_BITS) - 1);
            unsigned char bimodal_counter = bimodal_table[u.bimodal_index];
            u.bimodal_prediction = (bimodal_counter > COUNTER_MAX / 2);

            // chooser prediction
            u.chooser_index = (b.address >> 2) & ((1 << CHOOSER_TABLE_BITS) - 1);
            unsigned char chooser_counter = chooser_table[u.chooser_index];
            bool chooser_choice = (chooser_counter > COUNTER_MAX / 2);

            // final prediction
            if (chooser_choice) {
                u.direction_prediction(u.gshare_prediction);
            } else {
                u.direction_prediction(u.bimodal_prediction);
            }
        } else {
            u.direction_prediction(true);
        }
        u.target_prediction(0);
        return &u;
    }

    void update(branch_update* bu, bool taken, unsigned int target) {
        if (bi.br_flags & BR_CONDITIONAL) {
            my_update* u = static_cast<my_update*>(bu);

            // update gshare predictor
            unsigned char* gshare_counter = &gshare_table[u->gshare_index];
            if (taken) {
                if (*gshare_counter < COUNTER_MAX) (*gshare_counter)++;
            } else {
                if (*gshare_counter > 0) (*gshare_counter)--;
            }

            // update bimodal predictor
            unsigned char* bimodal_counter = &bimodal_table[u->bimodal_index];
            if (taken) {
                if (*bimodal_counter < COUNTER_MAX) (*bimodal_counter)++;
            } else {
                if (*bimodal_counter > 0) (*bimodal_counter)--;
            }

            // update chooser
            bool gshare_correct = (u->gshare_prediction == taken);
            bool bimodal_correct = (u->bimodal_prediction == taken);

            unsigned char* chooser_counter = &chooser_table[u->chooser_index];
            if (gshare_correct != bimodal_correct) {
                if (gshare_correct) {
                    if (*chooser_counter < COUNTER_MAX) (*chooser_counter)++;
                } else {
                    if (*chooser_counter > 0) (*chooser_counter)--;
                }
            }

            // update global history registers
            history = ((history << 1) | (taken ? 1 : 0)) & ((1 << HISTORY_LENGTH) - 1);
        }
    }
};