#pragma once

#include <cstdint>

#include "ALU.h"
#include "Decoder.h"
#include "RegisterFile.h"
#include "Memory.h"
#include "Opcodes.h"
#include "PipelineRegs.h"
#include "HazardDetector.h"

enum class RunMode {
    SingleCycle,
    Pipelined
};

class CPU {

    public:
        CPU();
        void loadProgram(const std::string& filename);
        void startSimulation(RunMode mode);
        void printStats();

    private:
        ALU alu;
        Decoder decoder;
        RegisterFile regFile;
        Memory memory;

        IFID ifid;
        IDEX idex;
        EXMEM exmem;
        MEMWB memwb;

        uint32_t pc {0};
        uint64_t cycleCount {0};

        bool halted;

        ALUOp MapToALU(uint32_t funct3, uint32_t funct7, uint32_t opcode);

        void runSingleCycle();
        void runPipelined();

        void stageIF();
        void stageID();
        void stageEX();
        void stageMEM();
        void stageWB();

        bool stalled;

        HazardDetector hazarddetector;
};