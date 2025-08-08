#include "DRAMPower/standards/ddr4/interface_calculation_DDR4.h"
#include "DRAMPower/data/energy.h"

namespace DRAMPower {

    
static double calc_static_energy(double NxBits, double R_eq, double t_CK, double voltage) {
    return NxBits * ((voltage * voltage) / R_eq) * t_CK; // N * P * t = N * E
};

static double calc_dynamic_energy(const uint64_t NxBits, const double energy) {
    return NxBits * energy;
};

static double calcStaticTermination(const bool termination, const DRAMPower::util::bus_stats_t &stats, const double R_eq, const double t_CK, const uint64_t datarate, const double voltage) {
    if (termination == false) {
        return 0; // No static termination
    }
    // zeroes 
    return calc_static_energy(stats.zeroes, R_eq, t_CK / datarate, voltage);
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
    result += calcDQEnergyTogglingRate(stats.togglingStats);
    result += calcDQEnergy(stats);
    result += calcCAEnergy(stats);
    result += calcDBIEnergy(stats);

    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcClockEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Pull up -> zeros
    result.controller.staticEnergy =
        calcStaticTermination(impedances_.ck_termination, stats.clockStats, impedances_.ck_R_eq, t_CK_, 2, VDD_); // datarate 2 -> half the time low other half high
    result.controller.dynamicEnergy =
        calc_dynamic_energy(stats.clockStats.zeroes_to_ones, impedances_.ck_dyn_E);

    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcDQSEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Pull up -> zeros
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
    if (impedances_.wdqs_termination == true) {
        // Write
        result.controller.staticEnergy +=
            calc_static_energy(NumDQsPairs * preposwritezeroes * (writecount - preposwriteseamless), impedances_.wdqs_R_eq, t_CK_, VDD_);
        result.controller.dynamicEnergy +=
            calc_dynamic_energy(NumDQsPairs * preposwritezero_to_one * (writecount - preposwriteseamless), impedances_.wdqs_dyn_E);
    }
    if (impedances_.rdqs_termination == true) {
        // Read
        result.dram.staticEnergy +=
            calc_static_energy(NumDQsPairs * preposreadzeroes * (readcount - preposreadseamless), impedances_.rdqs_R_eq, t_CK_, VDD_);
        result.dram.dynamicEnergy +=
            calc_dynamic_energy(NumDQsPairs * preposreadzero_to_one * (readcount - preposreadseamless), impedances_.rdqs_dyn_E);
    }

    // Data
    // Write
    result.controller.staticEnergy +=
        calcStaticTermination(impedances_.wdqs_termination, stats.writeDQSStats, impedances_.wdqs_R_eq, t_CK_, memspec_.dataRate, VDD_);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.writeDQSStats.zeroes_to_ones, impedances_.wdqs_dyn_E);
    
    // Read
    result.dram.staticEnergy +=
        calcStaticTermination(impedances_.rdqs_termination, stats.readDQSStats, impedances_.rdqs_R_eq, t_CK_, memspec_.dataRate, VDD_);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.readDQSStats.zeroes_to_ones, impedances_.rdqs_dyn_E);

    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcDQEnergyTogglingRate(const TogglingStats &stats)
{
    interface_energy_info_t result;

    // Write
    result.controller.staticEnergy +=
        calcStaticTermination(impedances_.wdq_termination, stats.write, impedances_.wdq_R_eq, t_CK_, memspec_.dataRate, VDD_);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.write.zeroes_to_ones, impedances_.wdq_dyn_E);

    // Read
    result.dram.staticEnergy +=
        calcStaticTermination(impedances_.rdq_termination, stats.read, impedances_.rdq_R_eq, t_CK_, memspec_.dataRate, VDD_);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.read.zeroes_to_ones, impedances_.rdq_dyn_E);
    
    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcDQEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Read
    result.dram.staticEnergy +=
        calcStaticTermination(impedances_.rdq_termination, stats.readBus, impedances_.rdq_R_eq, t_CK_, memspec_.dataRate, VDD_);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.readBus.zeroes_to_ones, impedances_.rdq_dyn_E);

    // Write
    result.controller.staticEnergy +=
        calcStaticTermination(impedances_.wdq_termination, stats.writeBus, impedances_.wdq_R_eq, t_CK_, memspec_.dataRate, VDD_);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.writeBus.zeroes_to_ones, impedances_.wdq_dyn_E);

    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcCAEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Pull up -> zeros
    result.controller.staticEnergy =
        calcStaticTermination(impedances_.ca_termination, stats.commandBus, impedances_.ca_R_eq, t_CK_, 1, VDD_);

    result.controller.dynamicEnergy =
        calc_dynamic_energy(stats.commandBus.zeroes_to_ones, impedances_.ca_dyn_E);

    return result;
}

interface_energy_info_t InterfaceCalculation_DDR4::calcDBIEnergy(const SimulationStats &stats) {
    interface_energy_info_t result;
    // Read
    result.dram.staticEnergy +=
        calcStaticTermination(impedances_.rdbi_termination, stats.readDBI, impedances_.rdbi_R_eq, t_CK_, memspec_.dataRate, VDD_);
    result.dram.dynamicEnergy +=
        calc_dynamic_energy(stats.readDBI.zeroes_to_ones, impedances_.rdbi_dyn_E);

    // Write
    result.controller.staticEnergy +=
        calcStaticTermination(impedances_.wdbi_termination, stats.writeDBI, impedances_.wdbi_R_eq, t_CK_, memspec_.dataRate, VDD_);
    result.controller.dynamicEnergy +=
        calc_dynamic_energy(stats.writeDBI.zeroes_to_ones, impedances_.wdbi_dyn_E);

    return result;
}

}