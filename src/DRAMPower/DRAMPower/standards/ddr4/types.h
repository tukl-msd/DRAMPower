#ifndef DRAMPOWER_STANDARDS_DDR4_TYPES_H
#define DRAMPOWER_STANDARDS_DDR4_TYPES_H

#include <DRAMUtils/memspec/standards/MemSpecDDR4.h>

#include "DRAMPower/memspec/MemSpecDDR4.h"
#include "DRAMPower/standards/ddr4/DDR4Core.h"
#include "DRAMPower/standards/ddr4/DDR4Interface.h"
#include "DRAMPower/standards/ddr4/core_calculation_DDR4.h"
#include "DRAMPower/standards/ddr4/interface_calculation_DDR4.h"

namespace DRAMPower {

struct DDR4Types {
    using DRAMUtilsMemSpec_t = DRAMUtils::MemSpec::MemSpecDDR4;
    using MemSpec_t = MemSpecDDR4;
    using Core_t = DDR4Core;
    using Interface_t = DDR4Interface;
    using CalcCore_t = Calculation_DDR4;
    using CalcInterface_t = InterfaceCalculation_DDR4;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR4_TYPES_H */
