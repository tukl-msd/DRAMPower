#include "DRAMPower/standards/ddr4/interface_calculation_DDR4.h"

namespace DRAMPower {

static double calc_static_power(uint64_t NxBits, double R_eq, double t_CK, double voltage, double factor) {
    return NxBits * (voltage * voltage) * factor * t_CK / R_eq;
};

static double calc_dynamic_power(uint64_t transitions, double C_total, double voltage) {
    return 0.5 * transitions * (voltage * voltage) * C_total;
};

InterfaceCalculation_DDR4::InterfaceCalculation_DDR4(const MemSpecDDR4 &memspec) :
    memspec_(memspec), 
    impedances_(memspec_.memImpedanceSpec) 
{
    t_CK_ = memspec_.memTimingSpec.tCK;
    VDDQ_ = memspec_.memPowerSpec[MemSpecDDR4::VoltageDomain::VDDQ].vXX;
}

interface_energy_info_t InterfaceCalculation_DDR4::calculateEnergy(const SimulationStats &stats) {
    // TODO Pull Up / Down Terminierung ??
    interface_energy_info_t result;
    result += calcClockEnergy(stats);
    result += calcDQSEnergy(stats);
    result += calcDQEnergy(stats);
    result += calcCAEnergy(stats);

    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcClockEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Termination???
    result.controller.staticPower =
        calc_static_power(stats.clockStats.ones, impedances_.R_eq_ck, t_CK_, VDDQ_, 1.0);
    result.controller.dynamicPower =
        calc_dynamic_power(stats.clockStats.zeroes_to_ones, impedances_.C_total_ck, VDDQ_);

    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcDQSEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Pull up -> zeros
    // TODO x16 devices have 2 DQS line
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
    
    // TODO add tests and check calculation
    // PrePostamble
    result.dram.staticPower +=
        calc_static_power(preposreadzeroes * (readcount - preposreadseamless), impedances_.R_eq_dqs, t_CK_, VDDQ_, 1);
    result.controller.staticPower +=
        calc_static_power(preposwritezeroes * (writecount - preposwriteseamless), impedances_.R_eq_dqs, t_CK_, VDDQ_, 1);

    result.dram.dynamicPower +=
        calc_dynamic_power(preposreadzero_to_one * (readcount - preposreadseamless), impedances_.C_total_dqs, VDDQ_);
    result.controller.dynamicPower +=
        calc_dynamic_power(preposwritezero_to_one * (writecount - preposwriteseamless), impedances_.C_total_dqs, VDDQ_);

    // Data
    result.dram.staticPower +=
        calc_static_power(stats.readDQSStats.zeroes, impedances_.R_eq_dqs, t_CK_, VDDQ_, 1);
    result.controller.staticPower +=
        calc_static_power(stats.writeDQSStats.zeroes, impedances_.R_eq_dqs, t_CK_, VDDQ_, 1);

    result.dram.dynamicPower +=
        calc_dynamic_power(stats.readDQSStats.zeroes_to_ones, impedances_.C_total_dqs, VDDQ_);
    result.controller.dynamicPower +=
        calc_dynamic_power(stats.writeDQSStats.zeroes_to_ones, impedances_.C_total_dqs, VDDQ_);

    
    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcDQEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Pull up -> zeros
    result.dram.staticPower +=
        calc_static_power(stats.readBus.zeroes, impedances_.R_eq_rb, t_CK_, VDDQ_, 0.5);
    result.controller.staticPower +=
        calc_static_power(stats.writeBus.zeroes, impedances_.R_eq_wb, t_CK_, VDDQ_, 0.5);

    result.dram.dynamicPower +=
        calc_dynamic_power(stats.readBus.zeroes_to_ones, impedances_.C_total_rb, VDDQ_);
    result.controller.dynamicPower +=
        calc_dynamic_power(stats.writeBus.zeroes_to_ones, impedances_.C_total_wb, VDDQ_);

    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcCAEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Pull up -> zeros
    result.controller.staticPower =
        calc_static_power(stats.commandBus.zeroes, impedances_.R_eq_cb, t_CK_, VDDQ_, 0.5);

    result.controller.dynamicPower =
        calc_dynamic_power(stats.commandBus.zeroes_to_ones, impedances_.C_total_cb, VDDQ_);

    return result;
}

}