#pragma once

#include <cstdint>
#include <string>

#include "ALU.h"
#include "Decoder.h"
#include "RegisterFile.h"
#include "Memory.h"
#include "Opcodes.h"
#include "PipelineRegs.h"
#include "HazardDetector.h"
#include "ForwardingUnit.h"
#include "CPUStats.h"

class CPU {

    public:
        CPU();
        void loadProgram(const std::string& filename);
        void startSimulation(RunMode mode);
        void printStats() const;
        const CPUStats& getStats() const { return stats; }

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
        bool stalled;

        CPUStats stats;

        ALUOp MapToALU(uint32_t funct3, uint32_t funct7, uint32_t opcode);

        void runSingleCycle();
        void runPipelined();

        void stageIF();
        void stageID();
        void stageEX();
        void stageMEM();
        void stageWB();

        HazardDetector hazarddetector;
        ForwardingUnit forwardingUnit;
};