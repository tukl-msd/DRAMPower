#ifndef DRAMPOWER_STANDARDS_LPDDR5_INTERFACE_CALCULATION_LPDDR5_H
#define DRAMPOWER_STANDARDS_LPDDR5_INTERFACE_CALCULATION_LPDDR5_H

#include "DRAMPower/Types.h"
#include "DRAMPower/data/energy.h"
#include "DRAMPower/memspec/MemSpecLPDDR5.h"
#include "DRAMPower/data/stats.h"

namespace DRAMPower {

class InterfaceCalculation_LPDDR5 {
   public:
    InterfaceCalculation_LPDDR5(const MemSpecLPDDR5 &memspec);

    interface_energy_info_t calculateEnergy(const SimulationStats &stats);

   private:
    const MemSpecLPDDR5 &memspec_;
    const MemSpecLPDDR5::MemImpedanceSpec &impedances_;
    double t_CK_;
    double t_WCK_;
    double VDDQ_;

    interface_energy_info_t calcClockEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDQSEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDQEnergy(const SimulationStats &stats);
    interface_energy_info_t calcCAEnergy(const SimulationStats &stats);
};

}  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR5_INTERFACE_CALCULATION_LPDDR5_H */
