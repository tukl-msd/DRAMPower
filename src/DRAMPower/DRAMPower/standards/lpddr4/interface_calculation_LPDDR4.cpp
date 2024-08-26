#include "interface_calculation_LPDDR4.h"

namespace DRAMPower {

InterfaceCalculation_LPDDR4::InterfaceCalculation_LPDDR4(const MemSpecLPDDR4 & memspec)
: memspec_(memspec)
, impedances_(memspec.memImpedanceSpec)
, t_CK(memspec.memTimingSpec.tCK)
, VDDQ(memspec.vddq)
{}

double InterfaceCalculation_LPDDR4::calc_static_energy(uint64_t NxBits, double R_eq, double t_CK, double voltage, double factor) {
    return NxBits * (voltage*voltage) * factor * t_CK / R_eq;
}

double InterfaceCalculation_LPDDR4::calc_dynamic_energy(uint64_t transitions, double C_total, double voltage) {
    return 0.5 * transitions * C_total * (voltage*voltage);
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcClockEnergy(const SimulationStats &stats)
{
    interface_energy_info_t result;

    result.controller.staticEnergy = 
        calc_static_energy(stats.clockStats.ones, impedances_.R_eq_ck, 0.5 * t_CK, VDDQ, 1);
    result.controller.dynamicEnergy = 
        calc_dynamic_energy(stats.clockStats.zeroes_to_ones, impedances_.C_total_ck, VDDQ);
    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcDQSEnergy(const SimulationStats & stats)
{
    interface_energy_info_t result;

    // Datarate of data bus
    result.dram.staticEnergy += calc_static_energy(stats.readDQSStats.ones, impedances_.R_eq_dqs, t_CK / memspec_.dataRate, VDDQ, 1.0);
    result.dram.dynamicEnergy += calc_dynamic_energy(stats.readDQSStats.zeroes_to_ones, impedances_.C_total_dqs, VDDQ);

    result.controller.staticEnergy += calc_static_energy(stats.writeDQSStats.ones, impedances_.R_eq_dqs, t_CK /  memspec_.dataRate, VDDQ, 1.0);
    result.controller.dynamicEnergy += calc_dynamic_energy(stats.writeDQSStats.zeroes_to_ones, impedances_.C_total_dqs, VDDQ);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcCAEnergy(const SimulationStats& bus_stats)
{
    interface_energy_info_t result;
    result.controller.staticEnergy = 
        calc_static_energy(bus_stats.commandBus.ones, impedances_.R_eq_cb, t_CK, VDDQ, 1.0);
    result.controller.dynamicEnergy =
        calc_dynamic_energy(bus_stats.commandBus.zeroes_to_ones, impedances_.C_total_cb, VDDQ);
    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcDQEnergy(const SimulationStats& bus_stats)
{
    interface_energy_info_t result;
    
    result.controller.staticEnergy +=
        calc_static_energy(bus_stats.writeBus.ones, impedances_.R_eq_wb, t_CK /  memspec_.dataRate, VDDQ, 1.0);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(bus_stats.writeBus.zeroes_to_ones, impedances_.C_total_wb, VDDQ);

    result.dram.staticEnergy +=
        calc_static_energy(bus_stats.readBus.ones, impedances_.R_eq_rb, t_CK /  memspec_.dataRate, VDDQ, 1.0);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(bus_stats.readBus.zeroes_to_ones, impedances_.C_total_rb, VDDQ);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcDQEnergyTogglingRate(const TogglingStats &stats)
{
    interface_energy_info_t result;

    // Read
    result.dram.staticEnergy +=
        calc_static_energy(stats.read.ones, impedances_.R_eq_rb, t_CK / memspec_.dataRate, VDDQ, 1.0);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.read.zeroes_to_ones, impedances_.C_total_rb, VDDQ);

    // Write
    result.controller.staticEnergy +=
        calc_static_energy(stats.write.ones, impedances_.R_eq_wb, t_CK / memspec_.dataRate, VDDQ, 1.0);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.write.zeroes_to_ones, impedances_.C_total_wb, VDDQ);
    
    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calculateEnergy(const SimulationStats& stats)
{
    interface_energy_info_t result;
    
    result += calcClockEnergy(stats);
    result += calcDQSEnergy(stats);
    if (stats.togglingStats)
        result += calcDQEnergyTogglingRate(*stats.togglingStats);
    else
        result += calcDQEnergy(stats);
    result += calcCAEnergy(stats);

    return result;
}

} // namespace DRAMPower