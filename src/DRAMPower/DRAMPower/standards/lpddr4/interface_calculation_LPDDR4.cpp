#include "interface_calculation_LPDDR4.h"

namespace DRAMPower {

InterfaceCalculation_LPDDR4::InterfaceCalculation_LPDDR4(const MemSpecLPDDR4 & memspec)
: memspec_(memspec)
, impedances_(memspec.memImpedanceSpec)
, t_CK(memspec.memTimingSpec.tCK)
, VDDQ(memspec.vddq)
{}

double InterfaceCalculation_LPDDR4::calc_static_energy(const uint64_t NxBits, const double R_eq, const double t_CK, const double voltage) {
    return NxBits * ((voltage * voltage) / R_eq) * t_CK; // N * P * t = N * E
}

double InterfaceCalculation_LPDDR4::calc_dynamic_energy(const uint64_t NxBits, const double energy) {
    return NxBits * energy;
}

double InterfaceCalculation_LPDDR4::calcStaticTermination(const bool termination, const DRAMPower::util::bus_stats_t &stats, const double R_eq, const double t_CK, const uint64_t datarate, const double voltage)
{
    if (termination == false) {
        return 0; // No static termination
    }
    // ones 
    return calc_static_energy(stats.ones, R_eq, t_CK / datarate, voltage);
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcClockEnergy(const SimulationStats &stats)
{
    interface_energy_info_t result;

    result.controller.staticEnergy =
        calcStaticTermination(impedances_.ck_termination, stats.clockStats, impedances_.ck_R_eq, t_CK, 2, VDDQ); // datarate 2 -> half the time low other half high

    result.controller.dynamicEnergy = 
        calc_dynamic_energy(stats.clockStats.zeroes_to_ones, impedances_.ck_dyn_E);
    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcDQSEnergy(const SimulationStats & stats)
{
    // Datarate of data bus
    interface_energy_info_t result;

    // Write
    result.controller.staticEnergy += calcStaticTermination(impedances_.wdqs_termination, stats.writeDQSStats, impedances_.wdqs_R_eq, t_CK, memspec_.dataRate, VDDQ);
    result.controller.dynamicEnergy += calc_dynamic_energy(stats.writeDQSStats.zeroes_to_ones, impedances_.wdqs_dyn_E);

    // Read
    result.dram.staticEnergy += calcStaticTermination(impedances_.rdqs_termination, stats.readDQSStats, impedances_.rdqs_R_eq, t_CK, memspec_.dataRate, VDDQ);
    result.dram.dynamicEnergy += calc_dynamic_energy(stats.readDQSStats.zeroes_to_ones, impedances_.rdqs_dyn_E);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcCAEnergy(const SimulationStats& bus_stats)
{
    interface_energy_info_t result;
    result.controller.staticEnergy = 
        calcStaticTermination(impedances_.ca_termination, bus_stats.commandBus, impedances_.ca_R_eq, t_CK, 1, VDDQ);
    result.controller.dynamicEnergy =
        calc_dynamic_energy(bus_stats.commandBus.zeroes_to_ones, impedances_.ca_dyn_E);;
    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcDQEnergy(const SimulationStats& bus_stats)
{
    interface_energy_info_t result;

    // Write
    result.controller.staticEnergy +=
        calcStaticTermination(impedances_.wdq_termination, bus_stats.writeBus, impedances_.wdq_R_eq, t_CK, memspec_.dataRate, VDDQ);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(bus_stats.writeBus.zeroes_to_ones, impedances_.wdq_dyn_E);

    // Read
    result.dram.staticEnergy +=
        calcStaticTermination(impedances_.rdq_termination, bus_stats.readBus, impedances_.rdq_R_eq, t_CK, memspec_.dataRate, VDDQ);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(bus_stats.readBus.zeroes_to_ones, impedances_.rdq_dyn_E);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcDQEnergyTogglingRate(const TogglingStats &stats)
{
    interface_energy_info_t result;

    // Write
    result.controller.staticEnergy +=
        calcStaticTermination(impedances_.wdq_termination, stats.write, impedances_.wdq_R_eq, t_CK, memspec_.dataRate, VDDQ);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.write.zeroes_to_ones, impedances_.wdq_dyn_E);

    // Read
    result.dram.staticEnergy +=
        calcStaticTermination(impedances_.rdq_termination, stats.read, impedances_.rdq_R_eq, t_CK, memspec_.dataRate, VDDQ);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.read.zeroes_to_ones, impedances_.rdq_dyn_E);
    
    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calculateEnergy(const SimulationStats& stats)
{
    interface_energy_info_t result;
    
    result += calcClockEnergy(stats);
    result += calcDQSEnergy(stats);
    result += calcDQEnergyTogglingRate(stats.togglingStats);
    result += calcDQEnergy(stats);
    result += calcCAEnergy(stats);

    return result;
}

} // namespace DRAMPower