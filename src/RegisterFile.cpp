#include "../include/RegisterFile.h"

#include <stdexcept>
#include <iostream>
#include <iomanip>

RegisterFile::RegisterFile() {
    for (int i = 0; i < 32; ++i) {
        regs[i] = 0;
    }
}

uint32_t RegisterFile::read(int reg) const {
    if (reg < 0 || reg >= 32) {
        throw std::out_of_range("Register index out of range");
    }
    return regs[reg];
}

void RegisterFile::write(int reg, uint32_t value) {
    if (reg < 0 || reg >= 32) {
        throw std::out_of_range("Register index out of range");
    }

    if (reg == 0) {
        return;
    }

    regs[reg] = value;
}

void RegisterFile::dump() const {
    const char* abiNames[32] = {
        "zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
        "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
        "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
        "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };

    std::cout << "\n====== Register File ======\n";
    std::cout << std::left
              << std::setw(4)  << "x#"
              << std::setw(6)  << "ABI"
              << std::setw(12) << "HEX"
              << "DECIMAL"
              << "\n";
    std::cout << std::string(34, '-') << "\n";

    for (int i = 0; i < 32; i++) {
        std::cout << "x" << std::left  << std::setw(3)  << i
                         << std::setw(6)  << abiNames[i]
                         << "0x" <<  std::right<< std::hex << std::setfill('0')
                         << std::setw(8) << regs[i]
                         << "  " << std::dec << std::setfill(' ')
                         << std::setw(0) << regs[i]
                         << "\n";
    }
    std::cout << "===========================\n\n";
}