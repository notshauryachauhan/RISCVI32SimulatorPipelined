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
    uint32_t raw = 0;
    InstrType type = InstrType::SYSTEM;
    uint32_t opcode = 0;
    uint32_t funct3 = 0;
    uint32_t funct7= 0;
    uint32_t rd = 0;
    uint32_t rs1 = 0;
    uint32_t rs2 = 0;
    int32_t imm = 0;
};

class Decoder {
    public:
        Instruction decode(uint32_t raw_instruction);

    private:
        InstrType getInstrType(uint32_t opcode);
};