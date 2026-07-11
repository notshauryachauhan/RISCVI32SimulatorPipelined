#pragma once
#include "PipelineRegs.h"
#include <cstdint>

struct ForwardedData {
    uint32_t rs1_val = 0;
    uint32_t rs2_val = 0;
};

class ForwardingUnit {
public:
    ForwardedData forward(const IDEX& idex, const EXMEM& exmem, const MEMWB& memwb);
};
