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

class InterfaceCalculation_LPDDR4
{
private:
	const MemSpecLPDDR4 &memspec_;
	const MemSpecLPDDR4::MemImpedanceSpec &impedances_;
	double t_CK;
	double VDDQ;

public:
	InterfaceCalculation_LPDDR4(const MemSpecLPDDR4 & memspec);
private:
	double calcStaticTermination(const bool termination, const DRAMPower::util::bus_stats_t &stats, const double R_eq, const double t_CK, const uint64_t datarate, const double voltage);
	double calc_static_energy(const uint64_t NxBits, const double R_eq, const double t_CK, const double voltage);
	double calc_dynamic_energy(const uint64_t NxBits, const double energy);

	interface_energy_info_t calcClockEnergy(const SimulationStats &stats);
	interface_energy_info_t calcDQSEnergy(const SimulationStats & stats);
	interface_energy_info_t calcCAEnergy(const SimulationStats& bus_stats);
	interface_energy_info_t calcDQEnergy(const SimulationStats& bus_stats);
	interface_energy_info_t calcDQEnergyTogglingRate(const TogglingStats &stats);
    interface_energy_info_t calcDBIEnergy(const SimulationStats &stats);

public:
	interface_energy_info_t calculateEnergy(const SimulationStats& stats);
};

}

#endif /* DRAMPOWER_STANDARDS_LPDDR4_INTERFACE_CALCULATION_LPDDR4_H */
