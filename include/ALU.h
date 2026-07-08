#pragma once

#include <cstdint>

enum class ALUOp{
    ADD,
    SUB,
    AND,
    OR,
    XOR,
    SLL,
    SRL,
    SRA,
    SLT,
    SLTU,
};

struct ALUResult {
    uint32_t value;
    bool zero;
};

class ALU {
    public:
        ALUResult execute(ALUOp op, uint32_t operand1, uint32_t operand2);

    private:
};