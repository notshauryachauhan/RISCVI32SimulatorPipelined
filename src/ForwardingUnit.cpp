#include "../include/ForwardingUnit.h"
#include "../include/Opcodes.h"

ForwardedData ForwardingUnit::forward(const IDEX& idex, const EXMEM& exmem, const MEMWB& memwb) {
    ForwardedData data;
    data.rs1_val = idex.rs1_val;
    data.rs2_val = idex.rs2_val;
    data.rs1_source = ForwardSource::None;
    data.rs2_source = ForwardSource::None;

    if (!idex.valid) {
        return data;
    }

    uint32_t op = idex.decoded.opcode;
    bool uses_rs1 = (op == Opcodes::OP || op == Opcodes::OP_IMM || op == Opcodes::LOAD || 
                     op == Opcodes::STORE || op == Opcodes::BRANCH || op == Opcodes::JALR);
    bool uses_rs2 = (op == Opcodes::OP || op == Opcodes::STORE || op == Opcodes::BRANCH);

    if (uses_rs1 && idex.decoded.rs1 != 0) {
        if (exmem.valid && exmem.reg_write && exmem.rd != 0 && exmem.rd == idex.decoded.rs1) {
            data.rs1_val = exmem.alu_result;
            data.rs1_source = ForwardSource::EX_MEM;
        } else if (memwb.valid && memwb.reg_write && memwb.rd != 0 && memwb.rd == idex.decoded.rs1) {
            data.rs1_val = memwb.result;
            data.rs1_source = ForwardSource::MEM_WB;
        }
    }

    if (uses_rs2 && idex.decoded.rs2 != 0) {
        if (exmem.valid && exmem.reg_write && exmem.rd != 0 && exmem.rd == idex.decoded.rs2) {
            data.rs2_val = exmem.alu_result;
            data.rs2_source = ForwardSource::EX_MEM;
        } else if (memwb.valid && memwb.reg_write && memwb.rd != 0 && memwb.rd == idex.decoded.rs2) {
            data.rs2_val = memwb.result;
            data.rs2_source = ForwardSource::MEM_WB;
        }
    }

    return data;
}