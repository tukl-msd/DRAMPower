#ifndef DRAMPOWER_STANDARDS_HBM2_TYPES_H
#define DRAMPOWER_STANDARDS_HBM2_TYPES_H

#include <DRAMUtils/memspec/standards/MemSpecHBM2.h>

#include "DRAMPower/memspec/MemSpecHBM2.h"
#include "DRAMPower/standards/hbm2/HBM2Core.h"
#include "DRAMPower/standards/hbm2/HBM2Interface.h"
#include "DRAMPower/standards/hbm2/core_calculation_HBM2.h"
#include "DRAMPower/standards/hbm2/interface_calculation_HBM2.h"

namespace DRAMPower {

struct HBM2Types {
    using DRAMUtilsMemSpec_t = DRAMUtils::MemSpec::MemSpecHBM2;
    using MemSpec_t = MemSpecHBM2;
    using Core_t = HBM2Core;
    using Interface_t = HBM2Interface;
    using CalcCore_t = Calculation_HBM2;
    using CalcInterface_t = InterfaceCalculation_HBM2;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_HBM2_TYPES_H */
