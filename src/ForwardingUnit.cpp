#include "../include/ForwardingUnit.h"

ForwardedData ForwardingUnit::forward(const IDEX& idex, const EXMEM& exmem, const MEMWB& memwb) {
    ForwardedData data;
    data.rs1_val = idex.rs1_val;
    data.rs2_val = idex.rs2_val;

    if (exmem.valid && exmem.reg_write && exmem.rd != 0 && exmem.rd == idex.decoded.rs1) {
        data.rs1_val = exmem.alu_result;
    } else if (memwb.valid && memwb.reg_write && memwb.rd != 0 && memwb.rd == idex.decoded.rs1) {
        data.rs1_val = memwb.result;
    }

    if (exmem.valid && exmem.reg_write && exmem.rd != 0 && exmem.rd == idex.decoded.rs2) {
        data.rs2_val = exmem.alu_result;
    } else if (memwb.valid && memwb.reg_write && memwb.rd != 0 && memwb.rd == idex.decoded.rs2) {
        data.rs2_val = memwb.result;
    }

    return data;
}