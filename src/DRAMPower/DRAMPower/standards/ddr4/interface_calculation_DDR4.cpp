#include "DRAMPower/standards/ddr4/interface_calculation_DDR4.h"

namespace DRAMPower {

static double calc_static_energy(uint64_t NxBits, double R_eq, double t_CK, double voltage, double factor) {
    return NxBits * (voltage * voltage) * factor * t_CK / R_eq;
};

// Needed for prepostamble calculation
static double calc_static_energy(double NxBits, double R_eq, double t_CK, double voltage, double factor) {
    return NxBits * (voltage * voltage) * factor * t_CK / R_eq;
};

static double calc_dynamic_energy(uint64_t transitions, double C_total, double voltage) {
    return 0.5 * transitions * (voltage * voltage) * C_total;
};

InterfaceCalculation_DDR4::InterfaceCalculation_DDR4(const MemSpecDDR4 &memspec) :
    memspec_(memspec), 
    impedances_(memspec_.memImpedanceSpec) 
{
    t_CK_ = memspec_.memTimingSpec.tCK;
    VDD_ = memspec_.vddq;
}

interface_energy_info_t InterfaceCalculation_DDR4::calculateEnergy(const SimulationStats &stats) {
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

interface_energy_info_t InterfaceCalculation_DDR4::calcClockEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Pull up -> zeros
    result.controller.staticEnergy =
        calc_static_energy(stats.clockStats.zeroes, impedances_.R_eq_ck, 0.5 * t_CK_, VDD_, 1);
    result.controller.dynamicEnergy =
        calc_dynamic_energy(stats.clockStats.zeroes_to_ones, impedances_.C_total_ck, VDD_);

    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcDQSEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Pull up -> zeros
    // TODO x16 devices have 2 DQS line
    uint_fast8_t NumDQsPairs = 1;
    if(memspec_.bitWidth == 16)
        NumDQsPairs = 2;

    uint64_t readcount = 0;
    uint64_t writecount = 0;

    uint64_t preposreadseamless = 0;
    uint64_t preposwriteseamless = 0;

    double preposreadzeroes = memspec_.prePostamble.read_zeroes;
    double preposwritezeroes = memspec_.prePostamble.write_zeroes;
    uint64_t preposreadzero_to_one = memspec_.prePostamble.read_zeroes_to_ones;
    uint64_t preposwritezero_to_one = memspec_.prePostamble.write_zeroes_to_ones;
    

    for (auto& rank : stats.rank_total)
    {
        // Reads
        readcount +=    rank.counter.reads +
                        rank.counter.readAuto;

        // Writes
        writecount +=   rank.counter.writes +
                        rank.counter.writeAuto;

        // PrePostamble
        preposreadseamless += rank.prepos.readSeamless;
        preposwriteseamless += rank.prepos.writeSeamless;
    }
    
    // PrePostamble
    // TODO add test for x16 devices
    result.dram.staticEnergy +=
        calc_static_energy(preposreadzeroes * (readcount - preposreadseamless), impedances_.R_eq_dqs, t_CK_, VDD_, NumDQsPairs);
    result.controller.staticEnergy +=
        calc_static_energy(preposwritezeroes * (writecount - preposwriteseamless), impedances_.R_eq_dqs, t_CK_, VDD_, NumDQsPairs);

    result.dram.dynamicEnergy +=
        calc_dynamic_energy(NumDQsPairs * preposreadzero_to_one * (readcount - preposreadseamless), impedances_.C_total_dqs, VDD_);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(NumDQsPairs * preposwritezero_to_one * (writecount - preposwriteseamless), impedances_.C_total_dqs, VDD_);

    // Data
    result.dram.staticEnergy +=
        calc_static_energy(stats.readDQSStats.zeroes, impedances_.R_eq_dqs, 0.5 * t_CK_, VDD_, 1);
    result.controller.staticEnergy +=
        calc_static_energy(stats.writeDQSStats.zeroes, impedances_.R_eq_dqs, 0.5 * t_CK_, VDD_, 1);

    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.readDQSStats.zeroes_to_ones, impedances_.C_total_dqs, VDD_);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.writeDQSStats.zeroes_to_ones, impedances_.C_total_dqs, VDD_);

    
    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcDQEnergyTogglingRate(const TogglingStats &stats)
{
    interface_energy_info_t result;

    // Read
    result.dram.staticEnergy +=
        calc_static_energy(stats.read.zeroes, impedances_.R_eq_rb, t_CK_ / memspec_.dataRate, VDD_, 1.0);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.read.zeroes_to_ones, impedances_.C_total_rb, VDD_);

    // Write
    result.controller.staticEnergy +=
        calc_static_energy(stats.write.zeroes, impedances_.R_eq_wb, t_CK_ / memspec_.dataRate, VDD_, 1.0);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.write.zeroes_to_ones, impedances_.C_total_wb, VDD_);
    
    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcDQEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Pull up -> zeros
    // Read
    result.dram.staticEnergy +=
        calc_static_energy(stats.readBus.zeroes, impedances_.R_eq_rb, t_CK_ / memspec_.dataRate, VDD_, 1.0);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.readBus.zeroes_to_ones, impedances_.C_total_rb, VDD_);

    // Write
    result.controller.staticEnergy +=
        calc_static_energy(stats.writeBus.zeroes, impedances_.R_eq_wb, t_CK_ / memspec_.dataRate, VDD_, 1.0);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.writeBus.zeroes_to_ones, impedances_.C_total_wb, VDD_);

    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcCAEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Pull up -> zeros
    result.controller.staticEnergy =
        calc_static_energy(stats.commandBus.zeroes, impedances_.R_eq_cb, t_CK_, VDD_, 1);

    result.controller.dynamicEnergy =
        calc_dynamic_energy(stats.commandBus.zeroes_to_ones, impedances_.C_total_cb, VDD_);

    return result;
}

}