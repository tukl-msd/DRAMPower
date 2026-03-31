#ifndef DRAMPOWER_STANDARDS_DDR5_TYPES_H
#define DRAMPOWER_STANDARDS_DDR5_TYPES_H

#include <DRAMUtils/memspec/standards/MemSpecDDR5.h>

#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/standards/ddr5/DDR5Core.h"
#include "DRAMPower/standards/ddr5/DDR5Interface.h"
#include "DRAMPower/standards/ddr5/core_calculation_DDR5.h"
#include "DRAMPower/standards/ddr5/interface_calculation_DDR5.h"

namespace DRAMPower {

struct DDR5Types {
    using DRAMUtilsMemSpec_t = DRAMUtils::MemSpec::MemSpecDDR5;
    using MemSpec_t = MemSpecDDR5;
    using Core_t = DDR5Core;
    using Interface_t = DDR5Interface;
    using CalcCore_t = Calculation_DDR5;
    using CalcInterface_t = InterfaceCalculation_DDR5;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR5_TYPES_H */
