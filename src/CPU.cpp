#include "../include/CPU.h"
#include "../include/Opcodes.h"
#include "../include/PipelineRegs.h"
#include "../include/HazardDetector.h"
#include "../include/ForwardingUnit.h"
#include "../include/CPUStats.h"

#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <string>

CPU::CPU() : alu(), decoder(), regFile(), memory(), pc(0), cycleCount(0), halted(false), stalled(false), stats() {}

void CPU::loadProgram(const std::string& filename) {
    memory.loadProgram(filename);
    pc = 0;
    cycleCount = 0;
    halted = false;
    stalled = false;
    regFile.reset();
    regFile.write(2, 65536); // initialize sp to top of memory
    ifid.reset();
    idex.reset();
    exmem.reset();
    memwb.reset();
    stats.reset(RunMode::SingleCycle);
}

void CPU::startSimulation(RunMode mode){
    stats.reset(mode);
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
    while (((!halted && pc < memory.instructionCount() * 4) || ifid.valid || idex.valid || exmem.valid || memwb.valid) && cycleCount < 1000000){
    
        MEMWB saved_memwb = memwb; 
        
        stageWB();
        stageMEM();
        
        MEMWB new_memwb = memwb;
        memwb = saved_memwb;
        
        stageEX();
        
        memwb = new_memwb;

        HazardSignals signals = hazarddetector.detect(ifid, idex, exmem);

        if (signals.flush) {
            pc = exmem.pc_next;
            idex.valid= false;
            ifid.valid = false;
            exmem.branch_taken = false;
            stalled = false;
            stats.flushCount++;
        } else if (signals.stall){
            stalled = true;
            stats.stallCycles++;
            stats.loadUseHazards++;
        } else {
            stalled = false;
        }
        stageID();
        stageIF();
        cycleCount++;
        stats.totalCycles = cycleCount;
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
        stats.totalCycles = cycleCount;
        stats.instructionsRetired++;

        switch (id_decoded.type) {
            case InstrType::R:
                stats.rTypeCount++;
                aluop = MapToALU(id_decoded.funct3, id_decoded.funct7, id_decoded.opcode);
                alu_result = alu.execute(aluop, regFile.read(id_decoded.rs1), regFile.read(id_decoded.rs2));
                regFile.write(id_decoded.rd, alu_result.value);
                pc += 4;
                break;

//--------------------------------------------------------------------------------------

            case InstrType::I:
                if (id_decoded.opcode == Opcodes::FENCE) {
                    stats.systemCount++;
                    pc += 4;
                    break;
                }
                aluop = MapToALU(id_decoded.funct3, id_decoded.funct7, id_decoded.opcode);
                alu_result = alu.execute(aluop, regFile.read(id_decoded.rs1), id_decoded.imm);
                if (id_decoded.opcode == Opcodes::LOAD){
                    stats.loadCount++;
                    stats.memoryReads++;
                    switch (id_decoded.funct3) {
                        case 0x0: // LB
                            regFile.write(id_decoded.rd, static_cast<int32_t>(static_cast<int8_t>(memory.loadByte(alu_result.value))));
                            break;
                        case 0x1: // LH
                            regFile.write(id_decoded.rd, static_cast<int32_t>(static_cast<int16_t>(memory.loadHalf(alu_result.value))));
                            break;
                        case 0x2: // LW
                            regFile.write(id_decoded.rd, memory.loadWord(alu_result.value));
                            break;
                        case 0x4: // LBU
                            regFile.write(id_decoded.rd, static_cast<uint32_t>(memory.loadByte(alu_result.value)));
                            break;
                        case 0x5: // LHU
                            regFile.write(id_decoded.rd, static_cast<uint32_t>(memory.loadHalf(alu_result.value)));
                            break;
                        default:
                            throw std::runtime_error("Unsupported load instruction");
                    }
                } else if(id_decoded.opcode == Opcodes::OP_IMM) {
                    stats.iTypeAluCount++;
                    regFile.write(id_decoded.rd, alu_result.value);
                } else if (id_decoded.opcode == Opcodes::JALR) {
                    stats.jumpCount++;
                    stats.totalJumps++;
                    stats.jalrCount++;
                    regFile.write(id_decoded.rd, pc + 4);
                    pc = alu_result.value & ~1u;
                    continue;
                }
                pc += 4;
                break;

//--------------------------------------------------------------------------------------

            case InstrType::S:
                stats.storeCount++;
                stats.memoryWrites++;
                aluop = MapToALU(id_decoded.funct3, id_decoded.funct7, id_decoded.opcode);
                alu_result = alu.execute(aluop, regFile.read(id_decoded.rs1), id_decoded.imm);
                switch (id_decoded.funct3) {
                    case 0x0: // SB
                        memory.storeByte(alu_result.value, static_cast<uint8_t>(regFile.read(id_decoded.rs2) & 0xFF));
                        break;
                    case 0x1: // SH
                        memory.storeHalf(alu_result.value, static_cast<uint16_t>(regFile.read(id_decoded.rs2) & 0xFFFF));
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

            case InstrType::B: {
                stats.branchCount++;
                stats.totalBranches++;
                aluop = MapToALU(id_decoded.funct3, id_decoded.funct7, id_decoded.opcode);
                alu_result = alu.execute(aluop, regFile.read(id_decoded.rs1), regFile.read(id_decoded.rs2));
                bool taken = false;
                switch (id_decoded.funct3) {
                    case 0x0: // BEQ
                        taken = alu_result.zero;
                        break;
                    case 0x1: // BNE
                        taken = !alu_result.zero;
                        break;
                    case 0x4: // BLT
                        taken = (static_cast<int32_t>(regFile.read(id_decoded.rs1)) < static_cast<int32_t>(regFile.read(id_decoded.rs2)));
                        break;
                    case 0x5: // BGE
                        taken = (static_cast<int32_t>(regFile.read(id_decoded.rs1)) >= static_cast<int32_t>(regFile.read(id_decoded.rs2)));
                        break;
                    case 0x6: // BLTU
                        taken = (regFile.read(id_decoded.rs1) < regFile.read(id_decoded.rs2));
                        break;
                    case 0x7: // BGEU
                        taken = (regFile.read(id_decoded.rs1) >= regFile.read(id_decoded.rs2));
                        break;
                    default:
                        throw std::runtime_error("Unsupported branch instruction");
                }
                if (taken) {
                    stats.branchesTaken++;
                    pc += id_decoded.imm;
                    continue;
                } else {
                    stats.branchesNotTaken++;
                }
                pc += 4;
                break;
            }

//--------------------------------------------------------------------------------------

            case InstrType::U:               
                if (id_decoded.opcode == Opcodes::LUI) { // LUI
                    stats.luiCount++;
                    regFile.write(id_decoded.rd, id_decoded.imm);
                } else if (id_decoded.opcode == Opcodes::AUIPC) { // AUIPC
                    stats.auipcCount++;
                    regFile.write(id_decoded.rd, pc + id_decoded.imm);
                } else {
                    throw std::runtime_error("Unsupported U-type instruction");
                }
                pc += 4;
                break;

//--------------------------------------------------------------------------------------

            case InstrType::J:
                stats.jumpCount++;
                stats.totalJumps++;
                stats.jalCount++;
                alu_result = alu.execute(ALUOp::ADD, pc, id_decoded.imm);
                regFile.write(id_decoded.rd, pc + 4);
                pc = alu_result.value;
                break;
                
//--------------------------------------------------------------------------------------

            case InstrType::SYSTEM:
                stats.systemCount++;
                if (id_decoded.funct3 == 0x0 && (id_decoded.imm == 0x000 || id_decoded.imm == 0x001)) {
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

void CPU::printStats() const {
    regFile.dump();

    auto percent = [](uint64_t part, uint64_t total) -> double {
        return total > 0 ? (static_cast<double>(part) * 100.0 / static_cast<double>(total)) : 0.0;
    };

    double cpi = stats.instructionsRetired > 0 ? static_cast<double>(cycleCount) / static_cast<double>(stats.instructionsRetired) : 0.0;
    double ipc = cycleCount > 0 ? static_cast<double>(stats.instructionsRetired) / static_cast<double>(cycleCount) : 0.0;

    std::cout << "\n";
    std::cout << "============================================================\n";
    std::cout << "              EXECUTION & PIPELINE STATISTICS               \n";
    std::cout << "============================================================\n";
    std::cout << std::left << std::setw(32) << "Simulation Mode:" 
              << (stats.mode == RunMode::Pipelined ? "Pipelined (5-Stage)" : "Single-Cycle") << "\n";
    
    std::stringstream pc_ss;
    pc_ss << "0x" << std::hex << pc;
    std::cout << std::left << std::setw(32) << "Final Program Counter (PC):" << pc_ss.str() << "\n";
    std::cout << std::left << std::setw(32) << "Total Cycles Executed:" << std::dec << cycleCount << "\n";
    std::cout << std::left << std::setw(32) << "Instructions Retired:" << stats.instructionsRetired << "\n";
    
    std::cout << std::fixed << std::setprecision(3);
    std::cout << std::left << std::setw(32) << "Cycles Per Instruction (CPI):" << cpi << "\n";
    std::cout << std::left << std::setw(32) << "Instructions Per Cycle (IPC):" << ipc << "\n";

    std::cout << std::fixed << std::setprecision(2);

    if (stats.mode == RunMode::Pipelined) {
        uint64_t flushCycles = stats.flushCount * 2;
        uint64_t totalPenalty = stats.stallCycles + flushCycles;

        std::cout << "\n------------------------------------------------------------\n";
        std::cout << "                PIPELINE HAZARDS & OVERHEAD                 \n";
        std::cout << "------------------------------------------------------------\n";
        std::cout << std::left << std::setw(32) << "Load-Use Data Stalls:" 
                  << stats.stallCycles << " cycle" << (stats.stallCycles == 1 ? "" : "s") 
                  << " (" << percent(stats.stallCycles, cycleCount) << "% of total cycles)\n";
        std::cout << std::left << std::setw(32) << "  - Load-Use Hazard Events:" << stats.loadUseHazards << "\n";

        std::cout << std::left << std::setw(32) << "Control Hazard Flushes:" 
                  << stats.flushCount << " flush" << (stats.flushCount == 1 ? "" : "es") 
                  << " (" << flushCycles << " penalty cycles, " 
                  << percent(flushCycles, cycleCount) << "% of cycles)\n";
        std::cout << std::left << std::setw(32) << "  - Branch Flushes (Taken):" << stats.branchFlushes << "\n";
        std::cout << std::left << std::setw(32) << "  - Jump Flushes (JAL/JALR):" << stats.jumpFlushes << "\n";

        std::cout << std::left << std::setw(32) << "Total Pipeline Penalty:" 
                  << totalPenalty << " cycles (" << percent(totalPenalty, cycleCount) << "% overhead)\n";

        std::cout << "\n------------------------------------------------------------\n";
        std::cout << "                   DATA FORWARDING PATHS                    \n";
        std::cout << "------------------------------------------------------------\n";
        std::cout << std::left << std::setw(32) << "Total Forwarding Events:" << stats.totalForwards << "\n";
        std::cout << std::left << std::setw(32) << "  - EX/MEM -> EX (1-Cycle RAW):" 
                  << stats.fwdFromEXMEM << " (" << percent(stats.fwdFromEXMEM, stats.totalForwards) << "%)\n";
        std::cout << std::left << std::setw(32) << "  - MEM/WB -> EX (2-Cycle RAW):" 
                  << stats.fwdFromMEMWB << " (" << percent(stats.fwdFromMEMWB, stats.totalForwards) << "%)\n";
        std::cout << std::left << std::setw(32) << "Forwarded Operands:" << "\n";
        std::cout << std::left << std::setw(32) << "  - rs1 Operands Forwarded:" 
                  << stats.fwdRs1 << " (" << percent(stats.fwdRs1, stats.totalForwards) << "%)\n";
        std::cout << std::left << std::setw(32) << "  - rs2 Operands Forwarded:" 
                  << stats.fwdRs2 << " (" << percent(stats.fwdRs2, stats.totalForwards) << "%)\n";
    }

    std::cout << "\n------------------------------------------------------------\n";
    std::cout << "                  CONTROL FLOW & BRANCHES                   \n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << std::left << std::setw(32) << "Total Conditional Branches:" << stats.totalBranches << "\n";
    std::cout << std::left << std::setw(32) << "  - Branches Taken:" 
              << stats.branchesTaken << " (" << percent(stats.branchesTaken, stats.totalBranches) << "%)\n";
    std::cout << std::left << std::setw(32) << "  - Branches Not Taken:" 
              << stats.branchesNotTaken << " (" << percent(stats.branchesNotTaken, stats.totalBranches) << "%)\n";
    std::cout << std::left << std::setw(32) << "Total Unconditional Jumps:" 
              << stats.totalJumps << " (JAL: " << stats.jalCount << ", JALR: " << stats.jalrCount << ")\n";

    uint64_t totalMemOps = stats.memoryReads + stats.memoryWrites;
    std::cout << "\n------------------------------------------------------------\n";
    std::cout << "                  MEMORY ACCESS STATISTICS                  \n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << std::left << std::setw(32) << "Total Memory Operations:" << totalMemOps << "\n";
    std::cout << std::left << std::setw(32) << "  - Data Loads (Reads):" 
              << stats.memoryReads << " (" << percent(stats.memoryReads, totalMemOps) << "%)\n";
    std::cout << std::left << std::setw(32) << "  - Data Stores (Writes):" 
              << stats.memoryWrites << " (" << percent(stats.memoryWrites, totalMemOps) << "%)\n";

    uint64_t upperImmTotal = stats.luiCount + stats.auipcCount;
    std::cout << "\n------------------------------------------------------------\n";
    std::cout << "                   INSTRUCTION MIX & ISA                    \n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << std::left << std::setw(32) << "Total Instructions Retired:" << stats.instructionsRetired << "\n";
    std::cout << std::left << std::setw(32) << "  - R-Type (Register ALU):" 
              << stats.rTypeCount << " (" << percent(stats.rTypeCount, stats.instructionsRetired) << "%)\n";
    std::cout << std::left << std::setw(32) << "  - I-Type (Immediate ALU):" 
              << stats.iTypeAluCount << " (" << percent(stats.iTypeAluCount, stats.instructionsRetired) << "%)\n";
    std::cout << std::left << std::setw(32) << "  - Load Instructions:" 
              << stats.loadCount << " (" << percent(stats.loadCount, stats.instructionsRetired) << "%)\n";
    std::cout << std::left << std::setw(32) << "  - Store Instructions:" 
              << stats.storeCount << " (" << percent(stats.storeCount, stats.instructionsRetired) << "%)\n";
    std::cout << std::left << std::setw(32) << "  - Conditional Branches:" 
              << stats.branchCount << " (" << percent(stats.branchCount, stats.instructionsRetired) << "%)\n";
    std::cout << std::left << std::setw(32) << "  - Unconditional Jumps:" 
              << stats.jumpCount << " (" << percent(stats.jumpCount, stats.instructionsRetired) << "%)\n";
    std::cout << std::left << std::setw(32) << "  - Upper Immediate (LUI/AUIPC):" 
              << upperImmTotal << " (" << percent(upperImmTotal, stats.instructionsRetired) << "%)\n";
    std::cout << std::left << std::setw(32) << "  - System & Control (ECALL/FENCE):" 
              << stats.systemCount << " (" << percent(stats.systemCount, stats.instructionsRetired) << "%)\n";
    std::cout << "============================================================\n";
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

    if (opcode == Opcodes::FENCE) { // fence
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
        (idex.decoded.imm == 0x000 || idex.decoded.imm == 0x001)) {
        halted = true;
        ifid.valid = false;
        idex.valid = false;
        stats.instructionsRetired++;
        stats.systemCount++;
    }
}

//------------------------------------------------------------------------------------

void CPU::stageEX(){
    if(!idex.valid){
        exmem.valid = false;
        return;
    }

    ForwardedData fwd = forwardingUnit.forward(idex, exmem, memwb);

    if (fwd.rs1_source == ForwardSource::EX_MEM) {
        stats.totalForwards++;
        stats.fwdFromEXMEM++;
        stats.fwdRs1++;
    } else if (fwd.rs1_source == ForwardSource::MEM_WB) {
        stats.totalForwards++;
        stats.fwdFromMEMWB++;
        stats.fwdRs1++;
    }

    if (fwd.rs2_source == ForwardSource::EX_MEM) {
        stats.totalForwards++;
        stats.fwdFromEXMEM++;
        stats.fwdRs2++;
    } else if (fwd.rs2_source == ForwardSource::MEM_WB) {
        stats.totalForwards++;
        stats.fwdFromMEMWB++;
        stats.fwdRs2++;
    }

    uint32_t rs1_val = fwd.rs1_val;
    uint32_t rs2_val = fwd.rs2_val;

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

    ALUOp aluop = MapToALU(idex.decoded.funct3, idex.decoded.funct7, op);
    ALUResult alu_result = alu.execute(aluop, alu_a, alu_b);

    bool branch_taken = false;
    uint32_t pc_next = idex.pc + 4;

    if (op == Opcodes::BRANCH) {
        stats.totalBranches++;
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

        if (branch_taken) {
            pc_next = branch_target;
            stats.branchesTaken++;
            stats.branchFlushes++;
        } else {
            stats.branchesNotTaken++;
        }
    }

    if (op == Opcodes::JAL) {
        pc_next = alu_result.value;
        stats.totalJumps++;
        stats.jalCount++;
        stats.jumpFlushes++;
    }
    if (op == Opcodes::JALR) {
        pc_next = alu_result.value & ~1u;
        stats.totalJumps++;
        stats.jalrCount++;
        stats.jumpFlushes++;
    }

    exmem.decoded      = idex.decoded;
    exmem.pc           = idex.pc;
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
                          op != Opcodes::SYSTEM && 
                          op != Opcodes::FENCE);

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

    memwb.decoded = exmem.decoded;
    memwb.pc = exmem.pc;
    memwb.result = exmem.alu_result;

    if(exmem.mem_read) {
        stats.memoryReads++;
        switch (exmem.funct3) {
            case 0x0: // LB
                memwb.result = static_cast<int32_t>(static_cast<int8_t>(memory.loadByte(exmem.alu_result)));
                break;
            case 0x1: // LH
                memwb.result = static_cast<int32_t>(static_cast<int16_t>(memory.loadHalf(exmem.alu_result)));
                break;
            case 0x2: // LW
                memwb.result = memory.loadWord(exmem.alu_result);
                break;
            case 0x4: // LBU
                memwb.result = static_cast<uint32_t>(memory.loadByte(exmem.alu_result));
                break;
            case 0x5: // LHU
                memwb.result = static_cast<uint32_t>(memory.loadHalf(exmem.alu_result));
                break;
            default:
                throw std::runtime_error("Unsupported load instruction in MEM stage");
        }
    }

    if(exmem.mem_write) {
        stats.memoryWrites++;
        switch (exmem.funct3) {
            case 0x0: // SB
                memory.storeByte(exmem.alu_result, static_cast<uint8_t>(exmem.rs2_val & 0xFF));
                break;
            case 0x1: // SH
                memory.storeHalf(exmem.alu_result, static_cast<uint16_t>(exmem.rs2_val & 0xFFFF));
                break;
            case 0x2: // SW
                memory.storeWord(exmem.alu_result, exmem.rs2_val);
                break;
            default:
                throw std::runtime_error("Unsupported store instruction in MEM stage");
        }
    }

    memwb.rd = exmem.rd;

    memwb.reg_write = exmem.reg_write;

    memwb.valid = true;
}

//------------------------------------------------------------------------------------

void CPU::stageWB(){
    if (!memwb.valid) return;

    if (memwb.reg_write) {
        regFile.write(memwb.rd, memwb.result);
    }

    stats.instructionsRetired++;

    switch (memwb.decoded.type) {
        case InstrType::R:
            stats.rTypeCount++;
            break;
        case InstrType::I:
            if (memwb.decoded.opcode == Opcodes::LOAD) {
                stats.loadCount++;
            } else if (memwb.decoded.opcode == Opcodes::OP_IMM) {
                stats.iTypeAluCount++;
            } else if (memwb.decoded.opcode == Opcodes::JALR) {
                stats.jumpCount++;
            } else if (memwb.decoded.opcode == Opcodes::FENCE) {
                stats.systemCount++;
            }
            break;
        case InstrType::S:
            stats.storeCount++;
            break;
        case InstrType::B:
            stats.branchCount++;
            break;
        case InstrType::U:
            if (memwb.decoded.opcode == Opcodes::LUI) {
                stats.luiCount++;
            } else if (memwb.decoded.opcode == Opcodes::AUIPC) {
                stats.auipcCount++;
            }
            break;
        case InstrType::J:
            stats.jumpCount++;
            break;
        case InstrType::SYSTEM:
            stats.systemCount++;
            break;
    }
}

//-----------------------------------------------------------------------------------