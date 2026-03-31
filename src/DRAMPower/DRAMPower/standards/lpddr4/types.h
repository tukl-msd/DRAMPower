#ifndef DRAMPOWER_STANDARDS_LPDDR4_TYPES_H
#define DRAMPOWER_STANDARDS_LPDDR4_TYPES_H

#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>

#include "DRAMPower/memspec/MemSpecLPDDR4.h"
#include "DRAMPower/standards/lpddr4/LPDDR4Core.h"
#include "DRAMPower/standards/lpddr4/LPDDR4Interface.h"
#include "DRAMPower/standards/lpddr4/core_calculation_LPDDR4.h"
#include "DRAMPower/standards/lpddr4/interface_calculation_LPDDR4.h"

namespace DRAMPower {

struct LPDDR4Types {
    using DRAMUtilsMemSpec_t = DRAMUtils::MemSpec::MemSpecLPDDR4;
    using MemSpec_t = MemSpecLPDDR4;
    using Core_t = LPDDR4Core;
    using Interface_t = LPDDR4Interface;
    using CalcCore_t = Calculation_LPDDR4;
    using CalcInterface_t = InterfaceCalculation_LPDDR4;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR4_TYPES_H */
