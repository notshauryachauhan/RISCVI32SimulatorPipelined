#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <unordered_map>

#include "include/Memory.h"
#include "include/ALU.h"
#include "include/RegisterFile.h"
#include "include/CPU.h"
#include "include/Decoder.h"
#include "include/ForwardingUnit.h"
#include "include/HazardDetector.h"
#include "include/Memory.h"


int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <program_file>" << " <mode> (Pipelined/SingleCycle)" << std::endl;
        return 1;
    }

    std::unordered_map<std::string, RunMode> modeMap = {
        {"SingleCycle", RunMode::SingleCycle},
        {"Pipelined", RunMode::Pipelined}
    };

    if (modeMap.find(argv[2]) == modeMap.end()) {
        std::cerr << "Invalid mode. Please choose either 'SingleCycle' or 'Pipelined'." <<std::endl;
        return 1;
    }
    
    RunMode runmode  = modeMap.at(argv[2]);

    CPU cpu;
    cpu.loadProgram(argv[1]);
    cpu.startSimulation(runmode);
    cpu.printStats();


    /* For testing purposes, you can use this main function to load a program and print the instructions and their decoded forms.
    Memory memory;
    memory.loadProgram(argv[1]);

    std::cout << "Loaded program with " << memory.instructionCount() << " instructions." << std::endl;

    for (uint32_t i = 0; i < memory.instructionCount(); ++i) {
        uint32_t instruction = memory.fetchInstruction(i*4);
        std::cout << "Instruction at address " << i << ": " << std::hex << instruction << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    }

    Decoder decoder;
    for (uint32_t i = 0; i < memory.instructionCount(); ++i) {
        uint32_t instruction = memory.fetchInstruction(i*4);
        Instruction decoded = decoder.decode(instruction);
        std::cout << "Decoded instruction at address " << i << ": " << std::hex << decoded.raw << std::endl;
        std::cout << "  Type: " << instrTypeToString(decoded.type) << std::endl;
        std::cout << "  Opcode: " << std::hex << decoded.opcode << std::endl;
        std::cout << "  rd: " << std::dec << decoded.rd << std::endl;
        std::cout << "  rs1: " << std::dec << decoded.rs1 << std::endl;
        std::cout << "  rs2: " << std::dec << decoded.rs2 << std::endl;
        std::cout << "  funct3: " << std::hex << decoded.funct3 << std::endl;
        std::cout << "  funct7: " << std::hex << decoded.funct7 << std::endl;
        std::cout << "  imm: " << std::dec << decoded.imm << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    }

    RegisterFile regFile;

    regFile.write(1, 42);
    regFile.write(2, 100);
    regFile.write(0,69);
    std::cout << "Register x1: " << regFile.read(1) << std::endl;

    regFile.dump();

    */

    return 0;
}

/* For testing purposes, you can use this function to convert InstrType to string for easier debugging.
std::string instrTypeToString(InstrType type){
    switch (type) {
        case InstrType::R: return "R";
        case InstrType::I: return "I";
        case InstrType::S: return "S";
        case InstrType::B: return "B";
        case InstrType::U: return "U";
        case InstrType::J: return "J";
        case InstrType::SYSTEM: return "SYSTEM";
        default: return "UNKNOWN";
    }
}
*/
