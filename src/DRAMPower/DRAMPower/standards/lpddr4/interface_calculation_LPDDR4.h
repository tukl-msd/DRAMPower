#ifndef DRAMPOWER_STANDARDS_LPDDR4_INTERFACE_CALCULATION_LPDDR4_H
#define DRAMPOWER_STANDARDS_LPDDR4_INTERFACE_CALCULATION_LPDDR4_H

#pragma once

#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>

#include <DRAMPower/memspec/MemSpecLPDDR4.h>

#include <DRAMPower/Types.h>

#include <cstddef>
#include <cstdint>

namespace DRAMPower
{

class DRAM;

class InterfacePowerCalculation_LPPDR4
{
private:
	double VDDQ;
	double t_CK;

	MemSpecLPDDR4::MemImpedanceSpec impedanceSpec;
private:
	double calc_static_power(uint64_t ones, double R_eq, bool ddr) {
		double ddr_coeff = ddr ? 0.5 : 1.0;
		return ones * (VDDQ*VDDQ) * ddr_coeff * t_CK / R_eq;
	};

	double calc_dynamic_power(uint64_t zero_to_ones, double C_total) {
		return zero_to_ones * (C_total) * 0.5 * (VDDQ*VDDQ);
	};

public:
	InterfacePowerCalculation_LPPDR4(const MemSpecLPDDR4 & memspec)
	{
		VDDQ = memspec.memPowerSpec[MemSpecLPDDR4::VoltageDomain::VDDQ].vDDX;
		t_CK = memspec.memTimingSpec.tCK;
		impedanceSpec = memspec.memImpedanceSpec;
	};

	interface_energy_info_t calcClockEnergy(const util::Clock::clock_stats_t & clock_stats)
	{
		interface_energy_info_t energy;

		energy.controller.staticPower += calc_static_power(clock_stats.ones, impedanceSpec.R_eq_ck, true);
		energy.controller.dynamicPower += calc_dynamic_power(clock_stats.ones_to_zeroes, impedanceSpec.C_total_ck);

		return energy;
	};

	interface_energy_info_t calcDQSEnergy(const SimulationStats & stats)
	{
		interface_energy_info_t energy;

		energy.dram.staticPower += calc_static_power(stats.readDQSStats.ones, impedanceSpec.R_eq_dqs, true);
		energy.dram.dynamicPower += calc_dynamic_power(stats.readDQSStats.ones_to_zeroes, impedanceSpec.C_total_dqs);

		energy.controller.staticPower += calc_static_power(stats.writeDQSStats.ones, impedanceSpec.R_eq_dqs, true);
		energy.controller.dynamicPower += calc_dynamic_power(stats.writeDQSStats.ones_to_zeroes, impedanceSpec.C_total_dqs);

		return energy;
	};

	interface_energy_info_t calcEnergy(const SimulationStats& bus_stats)
	{
		interface_energy_info_t energy;

		energy.controller.staticPower += calc_static_power(bus_stats.commandBus.ones, impedanceSpec.R_eq_cb, false);
		energy.controller.staticPower += calc_static_power(bus_stats.writeBus.ones, impedanceSpec.R_eq_wb, true);
		energy.dram.staticPower += calc_static_power(bus_stats.readBus.ones, impedanceSpec.R_eq_rb, true);

		energy.controller.dynamicPower += calc_dynamic_power(bus_stats.commandBus.ones_to_zeroes, impedanceSpec.C_total_cb);
		energy.controller.dynamicPower += calc_dynamic_power(bus_stats.writeBus.ones_to_zeroes, impedanceSpec.C_total_rb);
		energy.dram.dynamicPower += calc_dynamic_power(bus_stats.readBus.ones_to_zeroes, impedanceSpec.C_total_wb);

		return energy;
	};
};

}

#endif /* DRAMPOWER_STANDARDS_LPDDR4_INTERFACE_CALCULATION_LPDDR4_H */
