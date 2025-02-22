#include "DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.h"

namespace DRAMPower {

static double calc_static_energy(uint64_t NxBits, double R_eq, double t_CK, double voltage) {
    return NxBits * (voltage * voltage) * t_CK / R_eq;
};

static double calc_dynamic_energy(uint64_t transitions, double C_total, double voltage) {
    return transitions * C_total * 0.5 * (voltage * voltage);
};

InterfaceCalculation_LPDDR5::InterfaceCalculation_LPDDR5(const MemSpecLPDDR5 &memspec)
    : memspec_(memspec), impedances_(memspec_.memImpedanceSpec) {
    t_CK_ = memspec_.memTimingSpec.tCK;
    t_WCK_ = memspec_.memTimingSpec.tWCK;
    VDDQ_ = memspec_.vddq;
}

interface_energy_info_t InterfaceCalculation_LPDDR5::calculateEnergy(const SimulationStats &stats) {
    interface_energy_info_t clock_energy = calcClockEnergy(stats);
    interface_energy_info_t DQS_energy = calcDQSEnergy(stats);
    interface_energy_info_t DQ_energy = calcDQEnergy(stats);
    DQ_energy += calcDQEnergyTogglingRate(stats.togglingStats);
    interface_energy_info_t CA_energy = calcCAEnergy(stats);

    interface_energy_info_t result;
    result += clock_energy;
    result += DQS_energy;
    result += CA_energy;
    result += DQ_energy;

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR5::calcClockEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;

    result.controller.staticEnergy =
        calc_static_energy(stats.clockStats.ones, impedances_.R_eq_ck, 0.5 * t_CK_, VDDQ_);
    result.controller.dynamicEnergy =
        calc_dynamic_energy(stats.clockStats.zeroes_to_ones, impedances_.C_total_ck, VDDQ_);

    // TODO datarate for wck
    result.controller.staticEnergy +=
        calc_static_energy(stats.wClockStats.ones, impedances_.R_eq_wck, 0.5 * t_WCK_, VDDQ_);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.wClockStats.zeroes_to_ones, impedances_.C_total_wck, VDDQ_);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR5::calcDQSEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    result.dram.staticEnergy +=
        calc_static_energy(stats.readDQSStats.ones, impedances_.R_eq_dqs, t_CK_ / memspec_.dataRate, VDDQ_);

    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.readDQSStats.zeroes_to_ones, impedances_.C_total_dqs, VDDQ_);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR5::calcDQEnergyTogglingRate(const TogglingStats &stats)
{
    interface_energy_info_t result;

    // Read
    result.dram.staticEnergy +=
        calc_static_energy(stats.read.ones, impedances_.R_eq_rb, t_CK_ / memspec_.dataRate, VDDQ_);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.read.zeroes_to_ones, impedances_.C_total_rb, VDDQ_);

    // Write
    result.controller.staticEnergy +=
        calc_static_energy(stats.write.ones, impedances_.R_eq_wb, t_CK_ / memspec_.dataRate, VDDQ_);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.write.zeroes_to_ones, impedances_.C_total_wb, VDDQ_);
    
    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR5::calcDQEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    result.dram.staticEnergy +=
        calc_static_energy(stats.readBus.ones, impedances_.R_eq_rb, t_CK_ / memspec_.dataRate, VDDQ_);
    result.controller.staticEnergy +=
        calc_static_energy(stats.writeBus.ones, impedances_.R_eq_wb, t_CK_ / memspec_.dataRate, VDDQ_);

    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.readBus.zeroes_to_ones, impedances_.C_total_rb, VDDQ_);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.writeBus.zeroes_to_ones, impedances_.C_total_wb, VDDQ_);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR5::calcCAEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // TODO datarate for command bus
    result.controller.staticEnergy =
        calc_static_energy(stats.commandBus.ones, impedances_.R_eq_cb, 0.5 * t_CK_, VDDQ_);

    result.controller.dynamicEnergy =
        calc_dynamic_energy(stats.commandBus.zeroes_to_ones, impedances_.C_total_cb, VDDQ_);

    return result;
}

}  // namespace DRAMPower
