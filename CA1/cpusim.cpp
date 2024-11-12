#include "CPU.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm> 
using namespace std;

int main(int argc, char* argv[]) {
    // Ensure an input file is provided
    if (argc < 2) {
        cout << "No file name entered. Exiting..." << endl;
        return -1;
    }

    // Open the input file
    ifstream infile(argv[1]);
    if (!infile.is_open()) {
        cout << "Error opening file." << endl;
        return 0;
    }

    // Initialize instruction memory (4KB = 4096 bytes)
    char instMem[4096] = {0};

    // Read the input file line by line and load into instruction memory
    string line;
    int memIndex = 0;
    while (getline(infile, line)) {
        // Skip empty lines or lines starting with '#'
        if (line.empty() || line[0] == '#')
            continue;

        // Remove whitespaces and newline characters
        line.erase(remove(line.begin(), line.end(), ' '), line.end());
        line.erase(remove(line.begin(), line.end(), '\n'), line.end());
        line.erase(remove(line.begin(), line.end(), '\r'), line.end());

        if (line.empty())
            continue;

        // Convert the hex string to an integer
        int byteValue = stoi(line, nullptr, 16);

        // Store the byte in instruction memory
        if (memIndex < 4096) {
            instMem[memIndex++] = static_cast<char>(byteValue);
        } else {
            cout << "Instruction memory overflow." << endl;
            break;
        }
    }

    // Set the maximum PC value (total bytes loaded)
    int maxPC = memIndex;

    // Create a CPU instance and initialize it
    CPU myCPU;

    // Processor's main loop (each iteration = one clock cycle)
    while (true) {
        // Fetch the next instruction from memory
        unsigned long pc = myCPU.readPC();
        if (pc + 3 >= 4096) {
            cout << "Program Counter out of bounds." << endl;
            break;
        }

        int instruction = myCPU.fetch(instMem, pc);

        // Execute the fetched instruction
        myCPU.executeInstruction(instruction);

        // Break if we reach the end of the instructions
        if (myCPU.readPC() >= (unsigned long)maxPC) {
            break;
        }
    }
    // Retrieve and print the values of a0 (x10) and a1 (x11) registers
    int a0 = myCPU.registers[10];  // Register a0 (x10)
    int a1 = myCPU.registers[11];  // Register a1 (x11)

    // Print the final result in the required format "(a0,a1)"
    cout << "(" << a0 << "," << a1 << ")" << endl;

    return 0;
}
