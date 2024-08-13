#ifndef STANDARDS_DDR4_INTERFACE_CALCULATION_DDR4_H
#define STANDARDS_DDR4_INTERFACE_CALCULATION_DDR4_H


#include "DRAMPower/Types.h"
#include "DRAMPower/data/energy.h"
#include "DRAMPower/memspec/MemSpecDDR4.h"
#include "DRAMPower/data/stats.h"

namespace DRAMPower {

class InterfaceCalculation_DDR4 {
   public:
    InterfaceCalculation_DDR4(const MemSpecDDR4 &memspec);

    interface_energy_info_t calculateEnergy(const SimulationStats &stats);

   private:
    const MemSpecDDR4 &memspec_;
    const MemSpecDDR4::MemImpedanceSpec &impedances_;
    double t_CK_;
    double VDD_;

    interface_energy_info_t calcClockEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDQSEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDQEnergyTogglingRate(const TogglingStats &stats);
    interface_energy_info_t calcDQEnergy(const SimulationStats &stats);
    interface_energy_info_t calcCAEnergy(const SimulationStats &stats);
};

}  // namespace DRAMPower

#endif /* STANDARDS_DDR4_INTERFACE_CALCULATION_DDR4_H */
