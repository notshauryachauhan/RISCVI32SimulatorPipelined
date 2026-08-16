#include "../include/Memory.h"

#include <string>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>

void Memory::reset() {
    instrMem.clear();
    dataMem.assign(65536, 0);
}

void Memory::loadProgram(const std::string& filename) {
    reset();

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        return;
    }

    std::string line;

    while(std::getline(file, line)){
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            continue;
        }
        size_t last = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(first, (last - first + 1));
        if (trimmed.empty()) {
            continue;
        }

        uint32_t instruction = static_cast<uint32_t>(std::stoul(trimmed, nullptr, 2));
        instrMem.push_back(instruction);
    }
}

uint32_t Memory::fetchInstruction(uint32_t address) const {
    if (address < instrMem.size() * 4) {
        return instrMem[address/4];
    } else {
        std::cerr << "Error: Instruction address out of bounds." << std::endl;
        return 0;
    }
}

uint32_t Memory::instructionCount() const {
    return static_cast<uint32_t>(instrMem.size());
}

uint32_t Memory::loadWord(uint32_t address) const {
    if (address + 3 < dataMem.size()) {
        return (dataMem[address+3] << 24) | (dataMem[address + 2] << 16) | (dataMem[address + 1] << 8) | dataMem[address];
    } else {
        std::cerr << "Error: Data address out of bounds." << std::endl;
        return 0;
    }
}

void Memory::storeWord(uint32_t address, uint32_t value) {
    if (address + 3 < dataMem.size()) {
        dataMem[address + 3] = (value >> 24) & 0xFF;
        dataMem[address + 2] = (value >> 16) & 0xFF;
        dataMem[address + 1] = (value >> 8) & 0xFF;
        dataMem[address] = value & 0xFF;
    } else {
        std::cerr << "Error: Data address out of bounds." << std::endl;
    }
}

uint16_t Memory::loadHalf(uint32_t address) const {
    if (address + 1 < dataMem.size()) {
        return static_cast<uint16_t>(dataMem[address] | (dataMem[address + 1] << 8));
    } else {
        std::cerr << "Error: Data address out of bounds." << std::endl;
        return 0;
    }
}

void Memory::storeHalf(uint32_t address, uint16_t value) {
    if (address + 1 < dataMem.size()) {
        dataMem[address] = value & 0xFF;
        dataMem[address + 1] = (value >> 8) & 0xFF;
    } else {
        std::cerr << "Error: Data address out of bounds." << std::endl;
    }
}

uint8_t Memory::loadByte(uint32_t address) const {
    if (address < dataMem.size()) {
        return dataMem[address];
    } else {
        std::cerr << "Error: Data address out of bounds." << std::endl;
        return 0;
    }
}

void Memory::storeByte(uint32_t address, uint8_t value) {
    if (address < dataMem.size()) {
        dataMem[address] = value;
    } else {
        std::cerr << "Error: Data address out of bounds." << std::endl;
    }
}