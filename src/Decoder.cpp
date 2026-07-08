#include "../include/Decoder.h"
#include "../include/Opcodes.h"

#include <stdexcept>
#include <cstdint>

Instruction Decoder::decode(uint32_t raw_instruction) {
    Instruction instr {};
    instr.raw = raw_instruction;
    instr.opcode = raw_instruction & 0x7F;
    instr.type = getInstrType(instr.opcode);

    switch (instr.type) {
        case InstrType::R:
            instr.rd = (raw_instruction >> 7) & 0x1F;
            instr.funct3 = (raw_instruction >> 12) & 0x07;
            instr.rs1 = (raw_instruction >> 15) & 0x1F;
            instr.rs2 = (raw_instruction >> 20) & 0x1F;
            instr.funct7 = (raw_instruction >> 25) & 0x7F;
            break;

        case InstrType::I:
            instr.rd = (raw_instruction >> 7) & 0x1F;
            instr.funct3 = (raw_instruction >> 12) & 0x07;
            instr.rs1 = (raw_instruction >> 15) & 0x1F;
            instr.imm = static_cast<int32_t>(raw_instruction) >> 20;
            break;

        case InstrType::S:
            instr.funct3 = (raw_instruction >> 12) & 0x07;
            instr.rs1 = (raw_instruction >> 15) & 0x1F;
            instr.rs2 = (raw_instruction >> 20) & 0x1F;
            instr.imm = ((static_cast<int32_t>(raw_instruction) >> 25) << 5) | ((raw_instruction >> 7) & 0x1F);
            break;

        case InstrType::B:{
            instr.funct3 = (raw_instruction >> 12) & 0x07;
            instr.rs1 = (raw_instruction >> 15) & 0x1F;
            instr.rs2 = (raw_instruction >> 20) & 0x1F;
            int32_t imm12_B = ((static_cast<int32_t>(raw_instruction) >> 31) << 12);
            int32_t imm11_B = ((raw_instruction >> 7) & 0x01) << 11;
            int32_t imm10_5_B = ((raw_instruction >> 25) & 0x3F) << 5;
            int32_t imm4_1_B = ((raw_instruction >> 8) & 0x0F) << 1;
            instr.imm = imm12_B | imm11_B | imm10_5_B | imm4_1_B;
            break;
        }

        case InstrType::U:
            instr.rd = (raw_instruction >> 7) & 0x1F;
            instr.imm = static_cast<int32_t>(raw_instruction & 0xFFFFF000);
            break;
        
        case InstrType::J:{
            instr.rd = (raw_instruction >> 7) & 0x1F;
            int32_t imm20_J = ((static_cast<int32_t>(raw_instruction) >> 31) << 20);
            int32_t imm19_12_J = ((raw_instruction >> 12) & 0xFF) << 12;
            int32_t imm11_J = ((raw_instruction >> 20) & 0x01) << 11;
            int32_t imm10_1_J = ((raw_instruction >> 21) & 0x3FF) << 1;
            instr.imm = imm20_J | imm19_12_J | imm11_J | imm10_1_J;
            break;
        }

        case InstrType::SYSTEM:
            instr.rd = (raw_instruction >> 7) & 0x1F;
            instr.funct3 = (raw_instruction >> 12) & 0x07;
            instr.rs1 = (raw_instruction >> 15) & 0x1F;
            instr.imm = static_cast<int32_t>(raw_instruction) >> 20;
            break;

    }

    return instr;
}

InstrType Decoder::getInstrType(uint32_t opcode) {
    switch (opcode) {
        case Opcodes::OP:
            return InstrType::R;
            break;
        case Opcodes::OP_IMM:
        case Opcodes::LOAD:
        case Opcodes::JALR:
            return InstrType::I;
            break;
        case Opcodes::STORE:
            return InstrType::S;
            break;
        case Opcodes::BRANCH:
            return InstrType::B;
            break;
        case Opcodes::LUI:
        case Opcodes::AUIPC:
            return InstrType::U;
            break;
        case Opcodes::JAL:
            return InstrType::J;
            break;
        case Opcodes::SYSTEM:
            return InstrType::SYSTEM;
            break;
        default:
            throw std::invalid_argument("Unknown opcode");
    }
}