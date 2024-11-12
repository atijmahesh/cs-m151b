#include "CPU.h"
#include <iostream>
#include <bitset>
#include <string>
#include <cstring>  
using namespace std;

CPU::CPU() {
    PC = 0; // Set PC to 0
    memset(dmemory, 0, sizeof(dmemory));  // Initialize data memory to 0
    memset(registers, 0, sizeof(registers));  // Initialize registers to 0
}

int CPU::fetch(char instMem[], unsigned long PC) {
    return (unsigned char)instMem[PC] |
           ((unsigned char)instMem[PC + 1] << 8) |
           ((unsigned char)instMem[PC + 2] << 16) |
           ((unsigned char)instMem[PC + 3] << 24);
}

void CPU::decode(int instruction, int &opcode, int &rd, int &rs1, int &rs2,
                 int &imm, int &funct3, int &funct7) {
    opcode = instruction & 0x7F; // Opcode (bits 6-0)
    rd = (instruction >> 7) & 0x1F; // Destination register (bits 11-7)
    funct3 = (instruction >> 12) & 0x7; // Funct3 (bits 14-12)
    rs1 = (instruction >> 15) & 0x1F; // Source register 1 (bits 19-15)
    rs2 = (instruction >> 20) & 0x1F; // Source register 2 (bits 24-20)
    funct7 = (instruction >> 25) & 0x7F; // Funct7 (bits 31-25)

    if (opcode == 0x37) { // LUI
        imm = instruction & 0xFFFFF000;
    } else if (opcode == 0x6F) { // JAL
        int imm20 = (instruction >> 31) & 0x1;
        int imm10_1 = (instruction >> 21) & 0x3FF;
        int imm11 = (instruction >> 20) & 0x1;
        int imm19_12 = (instruction >> 12) & 0xFF;
        imm = (imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1);
        // Sign-extend imm
        if (imm & 0x100000)
            imm |= 0xFFE00000;
    } else if (opcode == 0x63) { // B-type instructions (e.g., BEQ)
        int imm12 = (instruction >> 31) & 0x1;
        int imm10_5 = (instruction >> 25) & 0x3F;
        int imm4_1 = (instruction >> 8) & 0xF;
        int imm11 = (instruction >> 7) & 0x1;
        imm = (imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1);
        // Sign-extend imm
        if (imm & 0x1000)
            imm |= 0xFFFFE000;
    } else if (opcode == 0x23) { // Store
        int imm4_0 = (instruction >> 7) & 0x1F; // Bits 11-7
        int imm11_5 = (instruction >> 25) & 0x7F; // Bits 31-25
        imm = (imm11_5 << 5) | imm4_0;
        // Sign-extend imm
        if (imm & 0x800)
            imm |= 0xFFFFF000;
    } else if (opcode == 0x03 || opcode == 0x13) { // I-type
        imm = (instruction >> 20) & 0xFFF;
        // Sign-extend imm
        if (imm & 0x800)
            imm |= 0xFFFFF000;
    } else if (opcode == 0x33) { // R-type
        imm = 0;
    } else {
        imm = 0;
    }
}

int CPU::ALUControl(int ALUOp, int funct3, int funct7) {
    if (ALUOp == 10) {  // R-type instructions
        if (funct3 == 0 && funct7 == 0x00) return 0b0010;  // ADD
        if (funct3 == 4 && funct7 == 0x00) return 0b0011;  // XOR
    } else if (ALUOp == 1) {  // I-type instructions
        if (funct3 == 6) return 0b0001;  // ORI
        if (funct3 == 5 && funct7 == 0x20) return 0b0111;  // SRAI
    }
    return 0;  // Default no-op
}

void CPU::executeInstruction(int instruction) {
    int opcode, rd, rs1, rs2, imm, funct3, funct7;
    decode(instruction, opcode, rd, rs1, rs2, imm, funct3, funct7);

    unsigned int nextPC = PC + 4;

    if (opcode == 0x37) {  // LUI
        registers[rd] = imm;
    } else if (opcode == 0x6F) {  // JAL
        registers[rd] = PC + 4;
        nextPC = PC + imm;
    } else if (opcode == 0x63) {  // Branch instructions
        bool takeBranch = false;
        if (funct3 == 0x0) {  // BEQ
            if (registers[rs1] == registers[rs2])
                takeBranch = true;
        }
        // Handle other branch types if needed
        if (takeBranch) {
            nextPC = PC + imm;
        }
    } else if (opcode == 0x03) {  // Load instructions
        // Existing load handling
        int address = registers[rs1] + imm;
        if (address < 0 || address >= 4096) {
            cout << "Memory access out of bounds at address: " << address << endl;
            return;
        }
        if (funct3 == 0x0) {  // LB (Load Byte)
            char value = dmemory[address];  // Read one byte
            registers[rd] = (int8_t)value;  // Sign-extend to 32 bits
        } else if (funct3 == 0x2) {  // LW (Load Word)
            if (address + 3 >= 4096) {
                cout << "Memory access out of bounds at address: " << address << endl;
                return;
            }
            int value = (unsigned char)dmemory[address] |
                        ((unsigned char)dmemory[address + 1] << 8) |
                        ((unsigned char)dmemory[address + 2] << 16) |
                        ((unsigned char)dmemory[address + 3] << 24);
            registers[rd] = value;
        }
        // Handle other load types if needed
    } else if (opcode == 0x23) {  // Store instructions
        // Existing store handling
        int address = registers[rs1] + imm;
        if (address < 0 || address >= 4096) {
            cout << "Memory access out of bounds at address: " << address << endl;
            return;
        }
        if (funct3 == 0x0) {  // SB (Store Byte)
            char value = registers[rs2] & 0xFF;  // Get the lowest byte
            dmemory[address] = value;
        } else if (funct3 == 0x2) {  // SW (Store Word)
            if (address + 3 >= 4096) {
                cout << "Memory access out of bounds at address: " << address << endl;
                return;
            }
            int value = registers[rs2];
            dmemory[address] = value & 0xFF;
            dmemory[address + 1] = (value >> 8) & 0xFF;
            dmemory[address + 2] = (value >> 16) & 0xFF;
            dmemory[address + 3] = (value >> 24) & 0xFF;
        }
        // Handle other store types if needed
    } else {
        // Existing ALU operations
        int ALUOp = (opcode == 0x33) ? 10 : (opcode == 0x13) ? 1 : 0;
        int ALUControlInput = ALUControl(ALUOp, funct3, funct7);

        switch (ALUControlInput) {
            case 0b0010:  // ADD
                if (ALUOp == 10)  // R-type
                    registers[rd] = registers[rs1] + registers[rs2];
                else  // I-type
                    registers[rd] = registers[rs1] + imm;
                break;
            case 0b0011:  // XOR
                if (ALUOp == 10)
                    registers[rd] = registers[rs1] ^ registers[rs2];
                else
                    registers[rd] = registers[rs1] ^ imm;
                break;
            case 0b0001:  // ORI
                registers[rd] = registers[rs1] | imm;
                break;
            case 0b0111:  // SRAI
                {
                    int shamt = imm & 0x1F;  // Extract shift amount from immediate
                    int value = registers[rs1];
                    // Perform arithmetic right shift
                    registers[rd] = value >> shamt;
                }
                break;
            // Add other cases as needed
        }
    }

    // Ensure x0 is always zero
    registers[0] = 0;

    // Update PC
    PC = nextPC;
}

unsigned long CPU::readPC() {
    return PC;
}

void CPU::incPC() {
    PC += 4;  // This function is no longer used
}