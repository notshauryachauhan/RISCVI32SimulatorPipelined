#pragma once
#include <cstdint>

namespace Opcodes {
    constexpr uint32_t OP      = 0x33;
    constexpr uint32_t OP_IMM  = 0x13;
    constexpr uint32_t LOAD    = 0x03;
    constexpr uint32_t STORE   = 0x23;
    constexpr uint32_t BRANCH  = 0x63;
    constexpr uint32_t JAL     = 0x6F;
    constexpr uint32_t JALR    = 0x67;
    constexpr uint32_t LUI     = 0x37;
    constexpr uint32_t AUIPC   = 0x17;
    constexpr uint32_t SYSTEM  = 0x73;
}