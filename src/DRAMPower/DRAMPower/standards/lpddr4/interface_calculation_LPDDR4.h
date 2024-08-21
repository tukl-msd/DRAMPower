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
	double calc_static_energy(uint64_t ones, double R_eq, double factor) {
		return ones * (VDDQ*VDDQ) * factor * t_CK / R_eq;
	};

	double calc_dynamic_energy(uint64_t zero_to_ones, double C_total) {
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
		energy.controller.staticEnergy += 2.0 * calc_static_energy(clock_stats.ones, impedanceSpec.R_eq_ck, 0.5);
		energy.controller.dynamicEnergy += 2.0 *calc_dynamic_energy(clock_stats.zeroes_to_ones, impedanceSpec.C_total_ck);

		return energy;
	};

	interface_energy_info_t calcDQEnergyTogglingRate(const TogglingStats &stats)
	{
		interface_energy_info_t result;

		// Read
		result.dram.staticEnergy +=
			calc_static_energy(stats.read.ones, impedanceSpec.R_eq_rb, 1.0 / memspec_.dataRate);
		result.dram.dynamicEnergy +=
			calc_dynamic_energy(stats.read.zeroes_to_ones, impedanceSpec.C_total_rb);

		// Write
		result.controller.staticEnergy +=
			calc_static_energy(stats.write.ones, impedanceSpec.R_eq_wb, 1.0 / memspec_.dataRate);
		result.controller.dynamicEnergy +=
			calc_dynamic_energy(stats.write.zeroes_to_ones, impedanceSpec.C_total_wb);
		
		return result;
	}


	interface_energy_info_t calcDQSEnergy(const SimulationStats & stats)
	{
		interface_energy_info_t energy;

		// Datarate of data bus
		energy.dram.staticEnergy += calc_static_energy(stats.readDQSStats.ones, impedanceSpec.R_eq_dqs, 1.0 / memspec_.dataRate);
		energy.dram.dynamicEnergy += calc_dynamic_energy(stats.readDQSStats.zeroes_to_ones, impedanceSpec.C_total_dqs);

		energy.controller.staticEnergy += calc_static_energy(stats.writeDQSStats.ones, impedanceSpec.R_eq_dqs, 1.0 /  memspec_.dataRate);
		energy.controller.dynamicEnergy += calc_dynamic_energy(stats.writeDQSStats.zeroes_to_ones, impedanceSpec.C_total_dqs);

		return energy;
	};

	interface_energy_info_t calcCAEnergy(const SimulationStats& bus_stats)
	{
		interface_energy_info_t energy;
		energy.controller.staticEnergy += calc_static_energy(bus_stats.commandBus.ones, impedanceSpec.R_eq_cb, 1.0);
		energy.controller.dynamicEnergy += calc_dynamic_energy(bus_stats.commandBus.zeroes_to_ones, impedanceSpec.C_total_cb);
		return energy;
	}

	interface_energy_info_t calcDQEnergy(const SimulationStats& bus_stats)
	{
		interface_energy_info_t energy;
		
		energy.controller.staticEnergy += calc_static_energy(bus_stats.writeBus.ones, impedanceSpec.R_eq_wb, 1.0 /  memspec_.dataRate);
		energy.controller.dynamicEnergy += calc_dynamic_energy(bus_stats.writeBus.zeroes_to_ones, impedanceSpec.C_total_rb);

		energy.dram.staticEnergy += calc_static_energy(bus_stats.readBus.ones, impedanceSpec.R_eq_rb, 1.0 /  memspec_.dataRate);
		energy.dram.dynamicEnergy += calc_dynamic_energy(bus_stats.readBus.zeroes_to_ones, impedanceSpec.C_total_wb);

		return energy;
	};

	interface_energy_info_t calcEnergy(const SimulationStats& stats)
	{
		interface_energy_info_t result;
		result += calcClockEnergy(stats.clockStats);
		result += calcDQSEnergy(stats);
		if (stats.togglingStats)
			result += calcDQEnergyTogglingRate(*stats.togglingStats);
		else
			result += calcDQEnergy(stats);
		result += calcCAEnergy(stats);

		return result;
	};
};

}

#endif /* DRAMPOWER_STANDARDS_LPDDR4_INTERFACE_CALCULATION_LPDDR4_H */
