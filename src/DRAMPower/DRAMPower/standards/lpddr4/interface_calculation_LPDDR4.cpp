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

double InterfaceCalculation_LPDDR4::calcStaticTermination(const DRAMPower::util::bus_stats_t &stats, const DRAMPower::MemSpecLPDDR4::MemStaticSpecContainer &static_container, const double t_CK, const uint64_t datarate, const double voltage)
{
    switch( static_container.entry.termination ) {
        case DRAMUtils::MemSpec::TerminationScheme::Invalid:
            assert(false);
            // TODO throw error?
            // throw std::runtime_error("Invalid termination");
            return 0; // No energy returned
        case DRAMUtils::MemSpec::TerminationScheme::PUSH_PULL:
            // E_UP = E_DOWN -> E = 2 * E_UP = 2 * E_DOWN
            return calc_static_energy(stats.ones, static_container.equivalent_resistance, t_CK / datarate, voltage)
                + calc_static_energy(stats.zeroes, static_container.equivalent_resistance, t_CK / datarate, voltage);
        case DRAMUtils::MemSpec::TerminationScheme::LWSTL:
            return calc_static_energy(stats.ones, static_container.equivalent_resistance, t_CK / datarate, voltage);
            break;
        case DRAMUtils::MemSpec::TerminationScheme::PODL:
            return calc_static_energy(stats.zeroes, static_container.equivalent_resistance, t_CK / datarate, voltage);
            break;
        case DRAMUtils::MemSpec::TerminationScheme::UNTERMINATED:
            return 0; // No energy returned
            break;
    }
    return 0;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcClockEnergy(const SimulationStats &stats)
{
    interface_energy_info_t result;

    result.controller.staticEnergy =
        calcStaticTermination(stats.clockStats, impedances_.static_ck, t_CK, 2, VDDQ); // datarate 2 -> half the time low other half high

    result.controller.dynamicEnergy = 
        calc_dynamic_energy(stats.clockStats.zeroes_to_ones, impedances_.dynamicEnergy_ck);
    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcDQSEnergy(const SimulationStats & stats)
{
    interface_energy_info_t result;

    // Datarate of data bus
    result.dram.staticEnergy += calcStaticTermination(stats.readDQSStats, impedances_.static_dqs, t_CK, memspec_.dataRate, VDDQ);
    result.dram.dynamicEnergy += calc_dynamic_energy(stats.readDQSStats.zeroes_to_ones, impedances_.dynamicEnergy_dqs);

    result.controller.staticEnergy += calcStaticTermination(stats.writeDQSStats, impedances_.static_dqs, t_CK, memspec_.dataRate, VDDQ);
    result.controller.dynamicEnergy += calc_dynamic_energy(stats.writeDQSStats.zeroes_to_ones, impedances_.dynamicEnergy_dqs);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcCAEnergy(const SimulationStats& bus_stats)
{
    interface_energy_info_t result;
    result.controller.staticEnergy = 
        calcStaticTermination(bus_stats.commandBus, impedances_.static_cb, t_CK, 1, VDDQ);
    result.controller.dynamicEnergy =
        calc_dynamic_energy(bus_stats.commandBus.zeroes_to_ones, impedances_.dynamicEnergy_cb);;
    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcDQEnergy(const SimulationStats& bus_stats)
{
    interface_energy_info_t result;
    
    result.controller.staticEnergy +=
        calcStaticTermination(bus_stats.writeBus, impedances_.static_wb, t_CK, memspec_.dataRate, VDDQ);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(bus_stats.writeBus.zeroes_to_ones, impedances_.dynamicEnergy_wb);

    result.dram.staticEnergy +=
        calcStaticTermination(bus_stats.readBus, impedances_.static_rb, t_CK, memspec_.dataRate, VDDQ);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(bus_stats.readBus.zeroes_to_ones, impedances_.dynamicEnergy_rb);

    return result;
}

interface_energy_info_t InterfaceCalculation_LPDDR4::calcDQEnergyTogglingRate(const TogglingStats &stats)
{
    interface_energy_info_t result;

    // Read
    result.dram.staticEnergy +=
        calcStaticTermination(stats.read, impedances_.static_rb, t_CK, memspec_.dataRate, VDDQ);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.read.zeroes_to_ones, impedances_.dynamicEnergy_rb);

    // Write
    result.controller.staticEnergy +=
        calcStaticTermination(stats.write, impedances_.static_wb, t_CK, memspec_.dataRate, VDDQ);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.write.zeroes_to_ones, impedances_.dynamicEnergy_wb);
    
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