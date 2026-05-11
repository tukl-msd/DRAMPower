#ifndef DRAMPOWER_STANDARDS_LPDDR6_TYPES_H
#define DRAMPOWER_STANDARDS_LPDDR6_TYPES_H

#include <DRAMUtils/memspec/standards/MemSpecLPDDR6.h>

#include "DRAMPower/memspec/MemSpecLPDDR6.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Core.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Interface.h"
#include "DRAMPower/standards/lpddr6/core_calculation_LPDDR6.h"
#include "DRAMPower/standards/lpddr6/interface_calculation_LPDDR6.h"

namespace DRAMPower {

struct LPDDR6Types {
    using DRAMUtilsMemSpec_t = DRAMUtils::MemSpec::MemSpecLPDDR6;
    using MemSpec_t = MemSpecLPDDR6;
    using Core_t = LPDDR6Core;
    using Interface_t = LPDDR6Interface;
    using CalcCore_t = Calculation_LPDDR6;
    using CalcInterface_t = InterfaceCalculation_LPDDR6;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR6_TYPES_H */
