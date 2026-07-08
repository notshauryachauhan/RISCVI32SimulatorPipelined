#include "../include/ALU.h"

#include <cstdint>
#include <stdexcept>

ALUResult ALU::execute(ALUOp op, uint32_t operand1, uint32_t operand2) {
    ALUResult result = {0, false};
    switch (op) {
        case ALUOp::ADD:
            result.value = operand1 + operand2;
            break;
        case ALUOp::SUB:
            result.value = operand1 - operand2;
            break;
        case ALUOp::AND:
            result.value = operand1 & operand2;
            break;
        case ALUOp::OR:
            result.value = operand1 | operand2;
            break;
        case ALUOp::XOR:
            result.value = operand1 ^ operand2;
            break;
        case ALUOp::SLL:
            result.value = operand1 << (operand2 & 0x1F);
            break;
        case ALUOp::SRL:
            result.value = operand1 >> (operand2 & 0x1F);
            break;
        case ALUOp::SRA:
            result.value = static_cast<int32_t>(operand1) >> (operand2 & 0x1F);
            break;
        case ALUOp::SLT:
            result.value = (static_cast<int32_t>(operand1) < static_cast<int32_t>(operand2)) ? 1 : 0;
            break;
        case ALUOp::SLTU:
            result.value = (operand1 < operand2) ? 1 : 0;
            break;
        default:
            throw std::invalid_argument("Invalid ALU operation");
    }
    result.zero = (result.value == 0);
    return result;
}