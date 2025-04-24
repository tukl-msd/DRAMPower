#include "DRAMPower/standards/lpddr6/interface_calculation_LPDDR6.h"

namespace DRAMPower {

static double calc_static_energy(uint64_t NxBits, double R_eq, double t_CK, double voltage) {
    return NxBits * (voltage * voltage) * t_CK / R_eq;
};

static double calc_dynamic_energy(const uint64_t NxBits, const double energy) {
    return NxBits * energy;
};

static double calcStaticTermination(const bool termination, const DRAMPower::util::bus_stats_t &stats, const double R_eq, const double t_CK, const uint64_t datarate, const double voltage)
{
    if (termination == false) {
        return 0; // No static termination
    }
    // ones 
    return calc_static_energy(stats.ones, R_eq, t_CK / datarate, voltage);
}

InterfaceCalculation_LPDDR6::InterfaceCalculation_LPDDR6(const MemSpecLPDDR5 &memspec)
    : memspec_(memspec), impedances_(memspec_.memImpedanceSpec) {
    t_CK_ = memspec_.memTimingSpec.tCK;
    t_WCK_ = memspec_.memTimingSpec.tWCK;
    VDDQ_ = memspec_.vddq;
}

interface_energy_info_t InterfaceCalculation_LPDDR6::calculateEnergy(const SimulationStats &stats) {
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

interface_energy_info_t InterfaceCalculation_LPDDR6::calcClockEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;

    result.controller.staticEnergy =
        calcStaticTermination(impedances_.ck_termination, stats.clockStats, impedances_.ck_R_eq, t_CK_, 2, VDDQ_); // datarate 2 -> half the time low other half high
    result.controller.dynamicEnergy =
        calc_dynamic_energy(stats.clockStats.zeroes_to_ones, impedances_.ck_dyn_E);

    result.controller.staticEnergy +=
        calcStaticTermination(impedances_.wck_termination, stats.wClockStats, impedances_.wck_R_eq, t_WCK_, 2, VDDQ_); // datarate 2 -> half the time low other half high
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.wClockStats.zeroes_to_ones, impedances_.wck_dyn_E);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR6::calcDQSEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;

    // Read
    result.dram.staticEnergy +=
        calcStaticTermination(impedances_.rdqs_termination, stats.readDQSStats, impedances_.rdqs_R_eq, t_CK_, memspec_.dataRate, VDDQ_);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.readDQSStats.zeroes_to_ones, impedances_.rdqs_dyn_E);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR6::calcDQEnergyTogglingRate(const TogglingStats &stats)
{
    interface_energy_info_t result;

    // Write
    result.controller.staticEnergy +=
        calcStaticTermination(impedances_.wb_termination, stats.write, impedances_.wb_R_eq, t_CK_, memspec_.dataRate, VDDQ_);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.write.zeroes_to_ones, impedances_.wb_dyn_E);

    // Read
    result.dram.staticEnergy +=
        calcStaticTermination(impedances_.rb_termination, stats.read, impedances_.rb_R_eq, t_CK_, memspec_.dataRate, VDDQ_);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.read.zeroes_to_ones, impedances_.rb_dyn_E);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR6::calcDQEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;

    // Write
    result.controller.staticEnergy +=
        calcStaticTermination(impedances_.wb_termination, stats.writeBus, impedances_.wb_R_eq, t_CK_, memspec_.dataRate, VDDQ_);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.writeBus.zeroes_to_ones, impedances_.wb_dyn_E);

    // Read
    result.dram.staticEnergy +=
        calcStaticTermination(impedances_.rb_termination, stats.readBus, impedances_.rb_R_eq, t_CK_, memspec_.dataRate, VDDQ_);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.readBus.zeroes_to_ones, impedances_.rb_dyn_E);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR6::calcCAEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;

    result.controller.staticEnergy =
        calcStaticTermination(impedances_.cb_termination, stats.commandBus, impedances_.cb_R_eq, t_CK_, 2, VDDQ_);

    result.controller.dynamicEnergy =
        calc_dynamic_energy(stats.commandBus.zeroes_to_ones, impedances_.cb_dyn_E);

    return result;
}

}  // namespace DRAMPower
