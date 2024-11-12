#ifndef CPU_H
#define CPU_H

#include <iostream>
#include <bitset>
#include <string>
using namespace std;

class CPU {
public:
    int registers[32];   // Register file (32 registers, including a0, a1)
    unsigned int PC;     // Program Counter
    char dmemory[4096];  // Data Memory (4KB)

    // Constructor
    CPU();

    // Core functions
    unsigned long readPC();                                // Read the Program Counter
    void incPC();                                          // Increment the Program Counter
    void executeInstruction(int instruction);              // Execute a decoded instruction
    int fetch(char instMem[], unsigned long PC);           // Fetch a 32-bit instruction from memory
    void decode(int instruction, int &opcode, int &rd,
                int &rs1, int &rs2, int &imm, int &funct3, int &funct7); // Decode the instruction

private:
    int ALUControl(int ALUOp, int funct3, int funct7);     // Determine ALU operation
};

#endif
