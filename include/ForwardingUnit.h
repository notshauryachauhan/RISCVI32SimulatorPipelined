#pragma once
#include "PipelineRegs.h"
#include <cstdint>

enum class ForwardSource {
    None,
    EX_MEM, // Forwarded from EX/MEM pipeline register (1-cycle RAW distance)
    MEM_WB  // Forwarded from MEM/WB pipeline register (2-cycle RAW distance)
};

struct ForwardedData {
    uint32_t rs1_val = 0;
    uint32_t rs2_val = 0;
    ForwardSource rs1_source = ForwardSource::None;
    ForwardSource rs2_source = ForwardSource::None;
};

class ForwardingUnit {
public:
    ForwardedData forward(const IDEX& idex, const EXMEM& exmem, const MEMWB& memwb);
};
