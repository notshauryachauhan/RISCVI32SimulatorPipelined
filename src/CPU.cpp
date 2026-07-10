#include "../include/CPU.h"
#include "../include/Opcodes.h"
#include "../include/PipelineRegs.h"

#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <string>

CPU::CPU() : alu(), decoder(), regFile(), memory(), pc(0), cycleCount(0), halted(false) {}

void CPU::loadProgram(const std::string& filename) {
    memory.loadProgram(filename);
    pc = 0;
    cycleCount = 0;
    halted = false;
    regFile.write(2, 65536); // initialize sp to top of memory
    ifid.reset();
    idex.reset();
    exmem.reset();
    memwb.reset();
}

void CPU::startSimulation(RunMode mode){
    switch(mode){
        case RunMode::Pipelined:
            runPipelined();
            break;
        case RunMode::SingleCycle:
            runSingleCycle();
            break;
        default:
            std::cout << "Input does not belong to available modes, defaulting to Single Cycle" << std::endl;
            runSingleCycle();
            break;
    }

}

void CPU::runPipelined(){
    std::cout << "in wrok lil bro" << std::endl;
}

void CPU::runSingleCycle() {
    while (!halted && pc < memory.instructionCount() * 4 && cycleCount < 1000000) {
        uint32_t if_raw_instr = memory.fetchInstruction(pc);
        Instruction id_decoded = decoder.decode(if_raw_instr);

        ALUOp aluop = ALUOp::ADD;
        ALUResult alu_result = {0, false};

        cycleCount++;

        switch (id_decoded.type) {
            case InstrType::R:
                aluop = MapToALU(id_decoded.funct3, id_decoded.funct7, id_decoded.opcode);
                alu_result = alu.execute(aluop, regFile.read(id_decoded.rs1), regFile.read(id_decoded.rs2));
                regFile.write(id_decoded.rd, alu_result.value);
                pc += 4;
                break;

//--------------------------------------------------------------------------------------

            case InstrType::I:
                aluop = MapToALU(id_decoded.funct3, id_decoded.funct7, id_decoded.opcode);
                alu_result = alu.execute(aluop, regFile.read(id_decoded.rs1), id_decoded.imm);
                if (id_decoded.opcode == Opcodes::LOAD){
                    if (id_decoded.funct3 == 0x0) { // LB
                        regFile.write(id_decoded.rd, static_cast<int32_t>(static_cast<int8_t>(memory.loadByte(alu_result.value))));
                    } else if (id_decoded.funct3 == 0x2) { // LW
                        regFile.write(id_decoded.rd, memory.loadWord(alu_result.value));
                    } else {
                        throw std::runtime_error("Unsupported load instruction");
                    }
                } else if(id_decoded.opcode == Opcodes::OP_IMM) {
                    regFile.write(id_decoded.rd, alu_result.value);
                } else if (id_decoded.opcode == Opcodes::JALR) {
                    regFile.write(id_decoded.rd, pc + 4);
                    pc = alu_result.value & ~1u;
                    continue;
                }
                pc += 4;
                break;

//--------------------------------------------------------------------------------------

            case InstrType::S:
                aluop = MapToALU(id_decoded.funct3, id_decoded.funct7, id_decoded.opcode);
                alu_result = alu.execute(aluop, regFile.read(id_decoded.rs1), id_decoded.imm);
                switch (id_decoded.funct3) {
                    case 0x0: // SB
                        memory.storeByte(alu_result.value, static_cast<uint8_t>(regFile.read(id_decoded.rs2) & 0xFF));
                        break;
                    case 0x2: // SW
                        memory.storeWord(alu_result.value, regFile.read(id_decoded.rs2));
                        break;
                    default:
                        throw std::runtime_error("Unsupported store instruction");
                }
                pc += 4;
                break;

//--------------------------------------------------------------------------------------

            case InstrType::B:
                aluop = MapToALU(id_decoded.funct3, id_decoded.funct7, id_decoded.opcode);
                alu_result = alu.execute(aluop, regFile.read(id_decoded.rs1), regFile.read(id_decoded.rs2));
                switch (id_decoded.funct3) {
                    case 0x0: // BEQ
                        if (alu_result.zero) {
                            pc += id_decoded.imm;
                            continue;
                        }
                        break;
                    case 0x1: // BNE
                        if (!alu_result.zero) {
                            pc += id_decoded.imm;
                            continue;
                        }
                        break;
                    case 0x4: // BLT
                        if (static_cast<int32_t>(regFile.read(id_decoded.rs1)) < static_cast<int32_t>(regFile.read(id_decoded.rs2))) {
                            pc += id_decoded.imm;
                            continue;
                        }
                        break;
                    case 0x5: // BGE
                        if (static_cast<int32_t>(regFile.read(id_decoded.rs1)) >= static_cast<int32_t>(regFile.read(id_decoded.rs2))) {
                            pc += id_decoded.imm;
                            continue;
                        }
                        break;
                    default:
                        throw std::runtime_error("Unsupported branch instruction");
                }
                pc += 4;
                break;

//--------------------------------------------------------------------------------------

            case InstrType::U:               
                if (id_decoded.opcode == Opcodes::LUI) { // LUI
                    regFile.write(id_decoded.rd, id_decoded.imm);
                } else if (id_decoded.opcode == Opcodes::AUIPC) { // AUIPC
                    regFile.write(id_decoded.rd, pc + id_decoded.imm);
                } else {
                    throw std::runtime_error("Unsupported U-type instruction");
                }
                pc += 4;
                break;

//--------------------------------------------------------------------------------------

            case InstrType::J:
                alu_result = alu.execute(ALUOp::ADD, pc, id_decoded.imm);
                regFile.write(id_decoded.rd, pc + 4);
                pc = alu_result.value;
                break;
                
//--------------------------------------------------------------------------------------

            case InstrType::SYSTEM:
                if (id_decoded.funct3 == 0x0 && id_decoded.imm == 0x000) {
                    halted = true;
                } else {
                    throw std::runtime_error("Unsupported SYSTEM instruction");
                }
                break;
            default:
                throw std::runtime_error("Unknown instruction type");
        }
    }
}

void CPU::printStats() {
    regFile.dump();
    std::cout << "Final PC: 0x" << std::hex << pc << std::endl;
    std::cout << "Total cycles executed: " << std::dec << cycleCount << std::endl;
}

ALUOp CPU::MapToALU(uint32_t funct3, uint32_t funct7, uint32_t opcode) {
    if (opcode == Opcodes::OP){ // R-type
        switch (funct3){
            case 0x0:
                return (funct7 == 0x20) ? ALUOp::SUB : ALUOp::ADD;
            case 0x1:
                return ALUOp::SLL;
            case 0x2:
                return ALUOp::SLT;
            case 0x3:
                return ALUOp::SLTU;
            case 0x4:
                return ALUOp::XOR;
            case 0x5:
                return (funct7 == 0x00) ? ALUOp::SRL : ALUOp::SRA;
            case 0x6:
                return ALUOp::OR;
            case 0x7:
                return ALUOp::AND;
            default:
                throw std::invalid_argument("Invalid funct3 for R-type instruction");
        }
    }

    if (opcode == Opcodes::OP_IMM) {
        switch (funct3) { // I-type
            case 0x0:
                return ALUOp::ADD; // ADDI
            case 0x2:
                return ALUOp::SLT; // SLTI
            case 0x3:
                return ALUOp::SLTU; // SLTIU
            case 0x4:
                return ALUOp::XOR; // XORI
            case 0x6:
                return ALUOp::OR; // ORI
            case 0x7:
                return ALUOp::AND; // ANDI
            case 0x1:
                return ALUOp::SLL; // SLLI
            case 0x5:
                return (funct7 == 0x00) ? ALUOp::SRL : ALUOp::SRA; // SRLI or SRAI
            default:
                throw std::invalid_argument("Invalid funct3 for I-type instruction");
        }
    }

    if (opcode == Opcodes::LOAD) { //load
        return ALUOp::ADD;
    }

    if (opcode == Opcodes::BRANCH) { // branch
        return ALUOp::SUB;
    }

    if (opcode == Opcodes::JALR) { // jalr
        return ALUOp::ADD;
    }

    if (opcode == Opcodes::STORE) { // store
        return ALUOp::ADD;
    }

    if (opcode == Opcodes::LUI || opcode == Opcodes::AUIPC) { // lui or auipc
        return ALUOp::SLL;
    }

    if (opcode == Opcodes::JAL) { // jal
        return ALUOp::ADD;
    }


    throw std::invalid_argument("Unsupported instruction type for ALU mapping");
}
