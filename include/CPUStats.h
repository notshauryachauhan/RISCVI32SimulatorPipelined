#pragma once

#include <cstdint>
#include <string>

enum class RunMode {
    SingleCycle,
    Pipelined
};

struct CPUStats {
    RunMode mode = RunMode::Pipelined;

    // Cycle & Execution Counts
    uint64_t totalCycles = 0;
    uint64_t instructionsRetired = 0;

    // Hazard & Stall Statistics (Pipelined mode)
    uint64_t stallCycles = 0;        // Total load-use stall cycles inserted
    uint64_t loadUseHazards = 0;     // Total load-use hazard events
    uint64_t flushCount = 0;         // Total pipeline flush events
    uint64_t branchFlushes = 0;      // Flushes due to taken conditional branches
    uint64_t jumpFlushes = 0;        // Flushes due to jumps (JAL/JALR)

    // Forwarding Statistics (Pipelined mode)
    uint64_t totalForwards = 0;      // Total operand forwarding occurrences
    uint64_t fwdFromEXMEM = 0;       // Forwarded from EX/MEM stage (1-cycle RAW distance)
    uint64_t fwdFromMEMWB = 0;       // Forwarded from MEM/WB stage (2-cycle RAW distance)
    uint64_t fwdRs1 = 0;             // Forwarded to rs1 operand
    uint64_t fwdRs2 = 0;             // Forwarded to rs2 operand

    // Branch & Jump Control Flow Statistics
    uint64_t totalBranches = 0;      // Total conditional branches evaluated
    uint64_t branchesTaken = 0;      // Branches evaluated as taken
    uint64_t branchesNotTaken = 0;   // Branches evaluated as not taken
    uint64_t totalJumps = 0;         // Total jumps executed
    uint64_t jalCount = 0;           // JAL count
    uint64_t jalrCount = 0;          // JALR count

    // Memory Access Statistics
    uint64_t memoryReads = 0;        // Total load operations (LB, LH, LW, LBU, LHU)
    uint64_t memoryWrites = 0;       // Total store operations (SB, SH, SW)

    // Instruction Mix (ISA breakdown)
    uint64_t rTypeCount = 0;         // R-type arithmetic & logic
    uint64_t iTypeAluCount = 0;      // I-type ALU arithmetic & logic
    uint64_t loadCount = 0;          // Load instructions
    uint64_t storeCount = 0;         // Store instructions
    uint64_t branchCount = 0;        // B-type branch instructions
    uint64_t jumpCount = 0;          // JAL & JALR instructions
    uint64_t luiCount = 0;           // LUI instructions
    uint64_t auipcCount = 0;         // AUIPC instructions
    uint64_t systemCount = 0;        // ECALL, EBREAK, FENCE instructions

    void reset(RunMode m) {
        *this = CPUStats();
        mode = m;
    }
};
