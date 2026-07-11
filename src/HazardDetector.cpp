#include "../include/HazardDetector.h"
#include "../include/Opcodes.h"

HazardSignals HazardDetector::detect(const IFID& ifid, const IDEX& idex, const EXMEM& exmem){

    HazardSignals hazardsignals;

    if (exmem.valid && exmem.branch_taken){
        hazardsignals.flush = true;
    }

    if (idex.valid && idex.decoded.opcode == Opcodes::LOAD && idex.decoded.rd != 0){
        if(ifid.valid){
            uint32_t op = ifid.instruction & 0x7F; 
            
            bool uses_rs1 = (op != Opcodes::JAL && op != Opcodes::LUI && op != Opcodes::AUIPC);
            bool uses_rs2 = (op == Opcodes::OP || op == Opcodes::BRANCH || op == Opcodes::STORE);

            uint32_t rs1_id = (ifid.instruction >> 15) & 0x1F;
            uint32_t rs2_id = (ifid.instruction >> 20) & 0x1F;

            if ((uses_rs1 && idex.decoded.rd == rs1_id) || (uses_rs2 && idex.decoded.rd == rs2_id)){
                hazardsignals.stall = true;
            }
        }
    }

    return hazardsignals;
}
