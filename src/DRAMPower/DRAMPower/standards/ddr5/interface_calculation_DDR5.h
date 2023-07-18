#ifndef DRAMPOWER_STANDARDS_DDR5_INTERFACE_CALCULATION_DDR5_H
#define DRAMPOWER_STANDARDS_DDR5_INTERFACE_CALCULATION_DDR5_H

#include "DRAMPower/Types.h"
#include "DRAMPower/data/energy.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/data/stats.h"

namespace DRAMPower {

class DDR5;

class InterfaceCalculation_DDR5 {
   public:
    InterfaceCalculation_DDR5(DDR5 &ddr);

    interface_energy_info_t calculateEnergy(timestamp_t timestamp);

   private:
    DDR5 &ddr_;
    const MemSpecDDR5 &memspec_;
    const MemSpecDDR5::MemImpedanceSpec &impedances_;
    double t_CK_;
    double VDDQ_;

    interface_energy_info_t calcClockEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDQSEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDQEnergy(const SimulationStats &stats);
    interface_energy_info_t calcCAEnergy(const SimulationStats &stats);
};

}  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR5_INTERFACE_CALCULATION_DDR5_H */
