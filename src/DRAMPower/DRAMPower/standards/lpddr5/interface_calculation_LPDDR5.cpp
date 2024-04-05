#include "DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.h"

namespace DRAMPower {

static double calc_static_power(uint64_t NxBits, double R_eq, double t_CK, double voltage) {
    return NxBits * (voltage * voltage) * t_CK / R_eq;
};

static double calc_dynamic_power(uint64_t transitions, double C_total, double voltage) {
    return transitions * C_total * 0.5 * (voltage * voltage);
};

InterfaceCalculation_LPDDR5::InterfaceCalculation_LPDDR5(const MemSpecLPDDR5 &memspec)
    : memspec_(memspec), impedances_(memspec_.memImpedanceSpec) {
    t_CK_ = memspec_.memTimingSpec.tCK;
    t_WCK_ = memspec_.memTimingSpec.tWCK;
    VDDQ_ = memspec_.memPowerSpec[MemSpecLPDDR5::VoltageDomain::VDDQ].vDDX;
}

interface_energy_info_t InterfaceCalculation_LPDDR5::calculateEnergy(const SimulationStats &stats) {
    interface_energy_info_t clock_energy = calcClockEnergy(stats);
    interface_energy_info_t DQS_energy = calcDQSEnergy(stats);
    interface_energy_info_t DQ_energy = calcDQEnergy(stats);
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

    result.controller.staticPower =
        2.0 * calc_static_power(stats.clockStats.ones, impedances_.R_eq_ck, 0.5 * t_CK_, VDDQ_);
    result.controller.dynamicPower =
        2.0 * calc_dynamic_power(stats.clockStats.zeroes_to_ones, impedances_.C_total_ck, VDDQ_);

    // TODO datarate for wck
    result.controller.staticPower +=
        2.0 * calc_static_power(stats.wClockStats.ones, impedances_.R_eq_wck, 0.5 * t_WCK_, VDDQ_);
    result.controller.dynamicPower +=
        2.0 * calc_dynamic_power(stats.wClockStats.zeroes_to_ones, impedances_.C_total_wck, VDDQ_);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR5::calcDQSEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    result.dram.staticPower +=
        calc_static_power(stats.readDQSStats.ones, impedances_.R_eq_dqs, t_CK_ / memspec_.dataRate, VDDQ_);

    result.dram.dynamicPower +=
        calc_dynamic_power(stats.readDQSStats.zeroes_to_ones, impedances_.C_total_dqs, VDDQ_);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR5::calcDQEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    result.dram.staticPower +=
        calc_static_power(stats.readBus.ones, impedances_.R_eq_rb, t_CK_ / memspec_.dataRate, VDDQ_);
    result.controller.staticPower +=
        calc_static_power(stats.writeBus.ones, impedances_.R_eq_wb, t_CK_ / memspec_.dataRate, VDDQ_);

    result.dram.dynamicPower +=
        calc_dynamic_power(stats.readBus.zeroes_to_ones, impedances_.C_total_rb, VDDQ_);
    result.controller.dynamicPower +=
        calc_dynamic_power(stats.writeBus.zeroes_to_ones, impedances_.C_total_wb, VDDQ_);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR5::calcCAEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    result.controller.staticPower =
        calc_static_power(stats.commandBus.zeroes, impedances_.R_eq_cb, t_CK_, VDDQ_);

    result.controller.dynamicPower =
        calc_dynamic_power(stats.commandBus.zeroes_to_ones, impedances_.C_total_cb, VDDQ_);

    return result;
}

}  // namespace DRAMPower
