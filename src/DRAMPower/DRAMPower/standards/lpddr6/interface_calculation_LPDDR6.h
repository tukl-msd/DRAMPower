#ifndef DRAMPOWER_STANDARDS_LPDDR6_INTERFACE_CALCULATION_LPDDR6_H
#define DRAMPOWER_STANDARDS_LPDDR6_INTERFACE_CALCULATION_LPDDR6_H

#include "DRAMPower/data/energy.h"
#include "DRAMPower/memspec/MemSpecLPDDR6.h"
#include "DRAMPower/data/stats.h"

namespace DRAMPower {

class InterfaceCalculation_LPDDR6 {
   public:
    InterfaceCalculation_LPDDR6(const MemSpecLPDDR6 &memspec);

    interface_energy_info_t calculateEnergy(const SimulationStats &stats) const;

   private:
    const MemSpecLPDDR6 &memspec_;
    const MemSpecLPDDR6::MemImpedanceSpec &impedances_;
    double t_CK_;
    double t_WCK_;
    double VDDQ_;

    interface_energy_info_t calcClockEnergy(const SimulationStats &stats) const;
    interface_energy_info_t calcDQSEnergy(const SimulationStats &stats) const;
    interface_energy_info_t calcDQEnergy(const SimulationStats &stats) const;
    interface_energy_info_t calcCAEnergy(const SimulationStats &stats) const;
    interface_energy_info_t calcDQEnergyTogglingRate(const TogglingStats &stats) const;
};

}  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR6_INTERFACE_CALCULATION_LPDDR6_H */
