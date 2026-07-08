#pragma once

#include <cstdint>

#include "ALU.h"
#include "Decoder.h"
#include "RegisterFile.h"
#include "Memory.h"
#include "Opcodes.h"

class CPU {

    public:
        CPU();
        void loadProgram(const std::string& filename);
        void run();
        void printStats();

    private:
        ALU alu;
        Decoder decoder;
        RegisterFile regFile;
        Memory memory;

        uint32_t pc {0};
        uint64_t cycleCount {0};

        bool halted;

        ALUOp MapToALU(uint32_t funct3, uint32_t funct7, uint32_t opcode);

};