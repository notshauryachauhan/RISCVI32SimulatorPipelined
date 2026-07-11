#include "../include/CPU.h"
#include "../include/Opcodes.h"
#include "../include/PipelineRegs.h"
#include "../include/HazardDetector.h"

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
    while ((!halted || ifid.valid || idex.valid || exmem.valid || memwb.valid) && cycleCount < 1000000){
        stageWB();
        stageMEM();
        stageEX();

        HazardSignals signals = hazarddetector.detect(ifid, idex, exmem);

        if (signals.flush) {
            pc = exmem.pc_next;
            idex.valid= false;
            ifid.valid = false;
            stalled = false;
        } else if (signals.stall){
            stalled = true;
        } else {
            stalled = false;
        }
        stageID();
        stageIF();
        cycleCount++;
    }
}


//=============================================================================================================
// Single Cycle Implementation
//=============================================================================================================

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

//======================================================================================================================

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
        return ALUOp::ADD;
    }

    if (opcode == Opcodes::JAL) { // jal
        return ALUOp::ADD;
    }


    throw std::invalid_argument("Unsupported instruction type for ALU mapping");
}

//======================================================================================================================


//======================================================================================================================
// Pipelined Implementation
//======================================================================================================================

void CPU::stageIF(){

    if (stalled){
        return;
    }

    if (halted || pc >= memory.instructionCount() * 4) {
        ifid.valid = false;
        return;
    }
    ifid.instruction = memory.fetchInstruction(pc);
    ifid.pc = pc;
    ifid.valid = true;

    pc += 4;
}

//------------------------------------------------------------------------------------

void CPU::stageID(){
    if(!ifid.valid) {
        idex.valid = false;
        return;
    }

    if(stalled){
        idex.valid = false;
        return;
    }

    idex.decoded = decoder.decode(ifid.instruction);
    idex.rs1_val = regFile.read(idex.decoded.rs1);
    idex.rs2_val = regFile.read(idex.decoded.rs2);
    idex.pc = ifid.pc;
    idex.valid = true;

    if (idex.decoded.type == InstrType::SYSTEM && 
        idex.decoded.funct3 == 0x0 && 
        idex.decoded.imm == 0x000) {
        halted = true;
        ifid.valid = false;
        idex.valid = false;
    }
}

//------------------------------------------------------------------------------------

void CPU::stageEX(){
    if(!idex.valid){
        exmem.valid = false;
        return;
    }

    if(stalled){
        exmem.valid = false;
        return;
    }

    uint32_t rs1_val = idex.rs1_val;
    uint32_t rs2_val = idex.rs2_val;

    uint32_t alu_a = rs1_val;
    uint32_t alu_b = rs2_val;

    uint32_t op = idex.decoded.opcode;
    if (op == Opcodes::OP_IMM || 
        op == Opcodes::LOAD   || 
        op == Opcodes::STORE  || 
        op == Opcodes::JALR) {
        alu_b = static_cast<uint32_t>(idex.decoded.imm);
    }

    if (op == Opcodes::AUIPC) {
        alu_a = idex.pc;
        alu_b = static_cast<uint32_t>(idex.decoded.imm);
    }

    if (op == Opcodes::JAL) {
        alu_a = idex.pc;
        alu_b = static_cast<uint32_t>(idex.decoded.imm);
    }

    // added onlyfor testing std::cout << "stageEX: opcode=0x" << std::hex << op << std::endl;

    ALUOp aluop = MapToALU(idex.decoded.funct3, idex.decoded.funct7, op);
    ALUResult alu_result = alu.execute(aluop, alu_a, alu_b);

    bool branch_taken = false;
    uint32_t pc_next = idex.pc + 4;

    if (op == Opcodes::BRANCH) {
        uint32_t r1 = rs1_val;
        uint32_t r2 = rs2_val;
        uint32_t branch_target = idex.pc + static_cast<uint32_t>(idex.decoded.imm);

        switch (idex.decoded.funct3) {
            case 0x0: branch_taken = (r1 == r2); break;                                              // BEQ
            case 0x1: branch_taken = (r1 != r2); break;                                              // BNE
            case 0x4: branch_taken = (static_cast<int32_t>(r1) <  static_cast<int32_t>(r2)); break; // BLT
            case 0x5: branch_taken = (static_cast<int32_t>(r1) >= static_cast<int32_t>(r2)); break; // BGE
            case 0x6: branch_taken = (r1 <  r2); break;                                              // BLTU
            case 0x7: branch_taken = (r1 >= r2); break;                                              // BGEU
        }

        if (branch_taken) pc_next = branch_target;
    }

    if (op == Opcodes::JAL)  pc_next = alu_result.value;
    if (op == Opcodes::JALR) pc_next = alu_result.value & ~1u;

    exmem.alu_result   = alu_result.value;
    exmem.rs2_val      = rs2_val;           // needed by SW in MEM stage
    exmem.rd           = idex.decoded.rd;
    exmem.pc_next      = pc_next;
    exmem.funct3       = idex.decoded.funct3;
    exmem.branch_taken = branch_taken || (op == Opcodes::JAL) || (op == Opcodes::JALR);
    exmem.mem_read     = (op == Opcodes::LOAD);
    exmem.mem_write    = (op == Opcodes::STORE);

    exmem.reg_write    = (op != Opcodes::STORE && 
                          op != Opcodes::BRANCH && 
                          op != Opcodes::SYSTEM);

    exmem.valid        = true;

    if (op == Opcodes::JAL || op == Opcodes::JALR) {
        exmem.alu_result = idex.pc + 4;
    }

    if (op == Opcodes::LUI) {
        exmem.alu_result = static_cast<uint32_t>(idex.decoded.imm);
        exmem.reg_write  = true;
    }
}

//------------------------------------------------------------------------------------

void CPU::stageMEM(){
    if (!exmem.valid){
        memwb.valid = false;
        return;
    }

    memwb.result = exmem.alu_result;

    if(exmem.mem_read) {
        if (exmem.funct3 == 0x2) {
            memwb.result = memory.loadWord(exmem.alu_result);
        } else if (exmem.funct3 == 0x0) {
            memwb.result = static_cast<int32_t>(static_cast<int8_t>(memory.loadByte(exmem.alu_result)));
        } else {
            memwb.result = exmem.alu_result;
        }
    }

    if(exmem.mem_write) {
        if (exmem.funct3 == 0x2) { // SW
            memory.storeWord(exmem.alu_result, exmem.rs2_val);
        } else if (exmem.funct3 == 0x0) { // SB          
            memory.storeByte(exmem.alu_result, static_cast<uint8_t>(exmem.rs2_val & 0xFF));
        }
    }

    memwb.rd = exmem.rd;

    memwb.reg_write = exmem.reg_write;

    memwb.valid = true;
}

//------------------------------------------------------------------------------------

void CPU::stageWB(){
    if (memwb.valid && memwb.reg_write) regFile.write(memwb.rd, memwb.result);
}

//-----------------------------------------------------------------------------------