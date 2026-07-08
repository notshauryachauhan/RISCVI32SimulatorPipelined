#pragma once

#include <cstdint>

#include "Opcodes.h"

enum class InstrType {
    R,
    I,
    S,
    B,
    U,
    J,
    SYSTEM
};

struct Instruction {
    uint32_t raw;
    InstrType type;
    uint32_t opcode;
    uint32_t funct3;
    uint32_t funct7;
    uint32_t rd;
    uint32_t rs1;
    uint32_t rs2;
    int32_t imm;
};

class Decoder {
    public:
        Instruction decode(uint32_t raw_instruction);

    private:
        InstrType getInstrType(uint32_t opcode);
};