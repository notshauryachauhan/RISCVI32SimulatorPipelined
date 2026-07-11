#pragma once

#include "PipelineRegs.h"

#include <cstdint>

struct HazardSignals{
    bool flush = false;
    bool stall = false;
};

class HazardDetector{
    public:
        HazardSignals detect(const IFID& ifid, const IDEX& idex, const EXMEM& exmem);
};