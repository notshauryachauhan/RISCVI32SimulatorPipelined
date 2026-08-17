#pragma once

#include "Decoder.h"

#include <cstdint>

struct IFID {
    uint32_t instruction = 0;
    uint32_t pc = 0;
    bool valid = false;

    void reset(){
        instruction = 0;
        pc = 0;
        valid = false;
    }
};

struct IDEX {
    uint32_t pc = 0;
    Instruction decoded {};
    uint32_t rs1_val = 0;
    uint32_t rs2_val = 0;
    bool valid = false;

    void reset(){
        pc = 0;
        decoded = Instruction{};
        rs1_val = 0;
        rs2_val = 0;
        valid = false;
    }
};

struct EXMEM {
    Instruction decoded {};
    uint32_t pc = 0;
    uint32_t alu_result = 0;
    uint32_t rs2_val = 0;
    uint32_t rd = 0;
    uint32_t pc_next = 0;
    uint32_t funct3 = 0;
    bool mem_read = false;
    bool mem_write = false;
    bool reg_write = false;
    bool branch_taken = false;
    bool valid = false;

    void reset(){
        decoded = Instruction{};
        pc = 0;
        alu_result = 0;
        rs2_val = 0;
        rd = 0;
        pc_next = 0;
        funct3 = 0;
        mem_read = false;
        mem_write = false;
        reg_write = false;
        branch_taken = false;
        valid = false;
    }
};

struct MEMWB {
    Instruction decoded {};
    uint32_t pc = 0;
    uint32_t result = 0;
    uint32_t rd = 0;
    bool reg_write = false;
    bool valid = false;

    void reset(){
        decoded = Instruction{};
        pc = 0;
        result = 0;
        rd = 0;
        reg_write = false;
        valid = false;
    }
};