#pragma once
#include <cstdint>

class RegisterFile {
    public:
        RegisterFile();
        uint32_t read(int reg) const;
        void write(int reg, uint32_t value);
        void dump() const;

    private:
        uint32_t regs[32];
};