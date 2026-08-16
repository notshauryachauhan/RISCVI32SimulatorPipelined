#pragma once

#include <vector>
#include <cstdint>
#include <string>

class Memory{
    public:
        void loadProgram(const std::string& filename);

        uint32_t fetchInstruction(uint32_t address) const;

        uint32_t loadWord(uint32_t address) const;

        void storeWord(uint32_t address, uint32_t value);

        uint16_t loadHalf(uint32_t address) const;

        void storeHalf(uint32_t address, uint16_t value);

        uint8_t loadByte(uint32_t address) const;

        void storeByte(uint32_t address, uint8_t value);

        uint32_t instructionCount() const;

        void reset();
    
    private:

        std::vector<uint32_t> instrMem;

        std::vector<uint8_t> dataMem;
};