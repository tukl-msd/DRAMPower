#ifndef STANDARDS_HBM2_INTERFACE_CALCULATION_HBM2_H
#define STANDARDS_HBM2_INTERFACE_CALCULATION_HBM2_H


#include "DRAMPower/Types.h"
#include "DRAMPower/data/energy.h"
#include "DRAMPower/memspec/MemSpecHBM2.h"
#include "DRAMPower/data/stats.h"

namespace DRAMPower {

class InterfaceCalculation_HBM2 {
   public:
    InterfaceCalculation_HBM2(const MemSpecHBM2 &memspec);

    interface_energy_info_t calculateEnergy(const SimulationStats &stats);

   private:
    const MemSpecHBM2 &memspec_;
    const MemSpecHBM2::MemImpedanceSpec &impedances_;
    double t_CK_;
    double VDD_;

    interface_energy_info_t calcClockEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDQSEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDQEnergyTogglingRate(const TogglingStats &stats);
    interface_energy_info_t calcDQEnergy(const SimulationStats &stats);
    interface_energy_info_t calcCAEnergy(const SimulationStats &stats);
    interface_energy_info_t calcDBIEnergy(const SimulationStats &stats);
};

}  // namespace DRAMPower

#endif /* STANDARDS_HBM2_INTERFACE_CALCULATION_HBM2_H */
