#ifndef DRAMPOWER_STANDARDS_DDR4_INTERFACE_CALCULATION_DDR4_H
#define DRAMPOWER_STANDARDS_DDR4_INTERFACE_CALCULATION_DDR4_H

#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>

#include <DRAMPower/memspec/MemSpecDDR4.h>

#include <DRAMPower/Types.h>

#include <cstddef>
#include <cstdint>

namespace DRAMPower {

    class DRAM;

    class InterfacePowerCalculation_DDR4 {
    private:
        double VDDQ;
        double t_CK;

        MemSpecDDR4::MemImpedanceSpec impedanceSpec;
    private:
        double calc_static_power(uint64_t zeros, double R_eq, bool ddr) {
            double ddr_coeff = ddr ? 0.5 : 1.0;
            return zeros * (VDDQ * VDDQ) * ddr_coeff * t_CK / R_eq;
        };

        double calc_dynamic_power(uint64_t zero_to_ones, double C_total) {
            return zero_to_ones * (C_total) * 0.5 * (VDDQ * VDDQ);
        };

    public:
        InterfacePowerCalculation_DDR4(const MemSpecDDR4 &memspec) {
            VDDQ = memspec.memPowerSpec[MemSpecDDR4::VoltageDomain::VDDQ].vXX;
            t_CK = memspec.memTimingSpec.tCK;
            impedanceSpec = memspec.memImpedanceSpec;
        };

        interface_energy_info_t calcClockEnergy(const util::Clock::clock_stats_t &clock_stats) {
            interface_energy_info_t energy;

            energy.controller.staticPower += calc_static_power(clock_stats.zeroes, impedanceSpec.R_eq_ck, true);
            energy.controller.dynamicPower += calc_dynamic_power(clock_stats.ones_to_zeroes, impedanceSpec.C_total_ck);

            return energy;
        };

        interface_energy_info_t calcDQSEnergy(const SimulationStats &stats) {
            interface_energy_info_t energy;

            energy.dram.staticPower += calc_static_power(stats.readDQSStats.zeroes, impedanceSpec.R_eq_dqs, true);
            energy.dram.dynamicPower += calc_dynamic_power(stats.readDQSStats.ones_to_zeroes,impedanceSpec.C_total_dqs);

            energy.controller.staticPower += calc_static_power(stats.writeDQSStats.zeroes, impedanceSpec.R_eq_dqs, true);
            energy.controller.dynamicPower += calc_dynamic_power(stats.writeDQSStats.ones_to_zeroes,impedanceSpec.C_total_dqs);

            return energy;
        };

        interface_energy_info_t calcEnergy(const SimulationStats &bus_stats) {
            interface_energy_info_t energy;

            // CA lines consume power on both logical levels
            energy.controller.staticPower += calc_static_power(bus_stats.commandBus.zeroes, impedanceSpec.R_eq_cb,false);
            energy.dram.staticPower += calc_static_power(bus_stats.commandBus.ones, impedanceSpec.R_eq_cb,false);
            energy.controller.dynamicPower += calc_dynamic_power(bus_stats.commandBus.ones_to_zeroes,impedanceSpec.C_total_cb);

            // DQ lines
            energy.controller.staticPower += calc_static_power(bus_stats.writeBus.zeroes, impedanceSpec.R_eq_wb, true);
            energy.controller.dynamicPower += calc_dynamic_power(bus_stats.writeBus.ones_to_zeroes,impedanceSpec.C_total_rb);

            energy.dram.staticPower += calc_static_power(bus_stats.readBus.zeroes, impedanceSpec.R_eq_rb, true);
            energy.dram.dynamicPower += calc_dynamic_power(bus_stats.readBus.ones_to_zeroes, impedanceSpec.C_total_wb);

            return energy;
        };
    };

}

#endif /* DRAMPOWER_STANDARDS_DDR4_INTERFACE_CALCULATION_DDR4_H */
