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
	MemSpecLPDDR4 memspec_;

	MemSpecLPDDR4::MemImpedanceSpec impedanceSpec;
private:
	double calc_static_power(uint64_t ones, double R_eq, double factor) {
		return ones * (VDDQ*VDDQ) * factor * t_CK / R_eq;
	};

	double calc_dynamic_power(uint64_t zero_to_ones, double C_total) {
		return zero_to_ones * (C_total) * 0.5 * (VDDQ*VDDQ);
	};

public:
	InterfacePowerCalculation_LPPDR4(const MemSpecLPDDR4 & memspec)
	: memspec_(memspec)
	{
		VDDQ = memspec.vddq;
		t_CK = memspec.memTimingSpec.tCK;
		impedanceSpec = memspec.memImpedanceSpec;
	};

	interface_energy_info_t calcClockEnergy(const util::Clock::clock_stats_t & clock_stats)
	{
		interface_energy_info_t energy;

		// Clock 0.5 * t_CK high, 0.5 * t_CK low
		energy.controller.staticPower += 2.0 * calc_static_power(clock_stats.ones, impedanceSpec.R_eq_ck, 0.5);
		energy.controller.dynamicPower += 2.0 *calc_dynamic_power(clock_stats.ones_to_zeroes, impedanceSpec.C_total_ck);

		return energy;
	};

	interface_energy_info_t calcDQSEnergy(const SimulationStats & stats)
	{
		interface_energy_info_t energy;

		// Datarate of data bus
		energy.dram.staticPower += calc_static_power(stats.readDQSStats.ones, impedanceSpec.R_eq_dqs, 1.0 / memspec_.dataRate);
		energy.dram.dynamicPower += calc_dynamic_power(stats.readDQSStats.ones_to_zeroes, impedanceSpec.C_total_dqs);

		energy.controller.staticPower += calc_static_power(stats.writeDQSStats.ones, impedanceSpec.R_eq_dqs, 1.0 /  memspec_.dataRate);
		energy.controller.dynamicPower += calc_dynamic_power(stats.writeDQSStats.ones_to_zeroes, impedanceSpec.C_total_dqs);

		return energy;
	};

	interface_energy_info_t calcEnergy(const SimulationStats& bus_stats)
	{
		interface_energy_info_t energy;

		energy.controller.staticPower += calc_static_power(bus_stats.commandBus.ones, impedanceSpec.R_eq_cb, false);
		energy.controller.staticPower += calc_static_power(bus_stats.writeBus.ones, impedanceSpec.R_eq_wb, 1.0 /  memspec_.dataRate);
		energy.dram.staticPower += calc_static_power(bus_stats.readBus.ones, impedanceSpec.R_eq_rb, 1.0 /  memspec_.dataRate);

		energy.controller.dynamicPower += calc_dynamic_power(bus_stats.commandBus.ones_to_zeroes, impedanceSpec.C_total_cb);
		energy.controller.dynamicPower += calc_dynamic_power(bus_stats.writeBus.ones_to_zeroes, impedanceSpec.C_total_rb);
		energy.dram.dynamicPower += calc_dynamic_power(bus_stats.readBus.ones_to_zeroes, impedanceSpec.C_total_wb);

		return energy;
	};
};

}

#endif /* DRAMPOWER_STANDARDS_LPDDR4_INTERFACE_CALCULATION_LPDDR4_H */
