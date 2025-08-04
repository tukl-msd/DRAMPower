#ifndef DRAMPOWER_STANDARDS_DDR5_INTERFACE_CALCULATION_DDR5_H
#define DRAMPOWER_STANDARDS_DDR5_INTERFACE_CALCULATION_DDR5_H

#include "DRAMPower/Types.h"
#include "DRAMPower/data/energy.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/data/stats.h"

namespace DRAMPower {

class InterfaceCalculation_DDR5 {
   public:
    InterfaceCalculation_DDR5(const MemSpecDDR5 &memspec);

    interface_energy_info_t calculateEnergy(const SimulationStats &stats);

   private:
    const MemSpecDDR5 &memspec_;
    const MemSpecDDR5::MemImpedanceSpec &impedances_;
    double t_CK_;
    double VDDQ_;

    interface_energy_info_t calcClockEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDQSEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDQEnergy(const SimulationStats &stats);
    interface_energy_info_t calcCAEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDQEnergyTogglingRate(const TogglingStats &stats);
};

}  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR5_INTERFACE_CALCULATION_DDR5_H */
