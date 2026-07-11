#pragma once

#include "Decoder.h"

#include <cstdint>

struct IFID {
    uint32_t instruction;
    uint32_t pc;
    bool valid;

    void reset(){
        instruction = 0;
        pc = 0;
        valid = false;
    }
};

struct IDEX {
    uint32_t pc;
    Instruction decoded;
    uint32_t rs1_val;
    uint32_t rs2_val;
    bool valid;

    void reset(){
        pc = 0;
        decoded = Instruction{};
        rs1_val = 0;
        rs2_val = 0;
        valid = false;
    }
};

struct EXMEM {
    uint32_t alu_result;
    uint32_t rs2_val;
    uint32_t rd;
    uint32_t pc_next;
    uint32_t funct3;
    bool mem_read;
    bool mem_write;
    bool reg_write;
    bool branch_taken;
    bool valid;

    void reset(){
        alu_result = 0;
        rs2_val = 0;
        rd = 0;
        pc_next = 0;
        mem_read = false;
        mem_write = false;
        reg_write = false;
        branch_taken = false;
        valid = false;
    }
};

struct MEMWB {
    uint32_t result;
    uint32_t rd;
    bool reg_write;
    bool valid;

    void reset(){
        result = 0;
        rd = 0;
        reg_write = false;
        valid = false;
    }
};