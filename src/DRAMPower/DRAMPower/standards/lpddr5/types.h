#ifndef DRAMPOWER_STANDARDS_LPDDR5_TYPES_H
#define DRAMPOWER_STANDARDS_LPDDR5_TYPES_H

#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>

#include "DRAMPower/memspec/MemSpecLPDDR5.h"
#include "DRAMPower/standards/lpddr5/LPDDR5Core.h"
#include "DRAMPower/standards/lpddr5/LPDDR5Interface.h"
#include "DRAMPower/standards/lpddr5/core_calculation_LPDDR5.h"
#include "DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.h"

namespace DRAMPower {

struct LPDDR5Types {
    using DRAMUtilsMemSpec_t = DRAMUtils::MemSpec::MemSpecLPDDR5;
    using MemSpec_t = MemSpecLPDDR5;
    using Core_t = LPDDR5Core;
    using Interface_t = LPDDR5Interface;
    using CalcCore_t = Calculation_LPDDR5;
    using CalcInterface_t = InterfaceCalculation_LPDDR5;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR5_TYPES_H */
