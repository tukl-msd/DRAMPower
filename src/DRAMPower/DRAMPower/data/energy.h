#ifndef DRAMPOWER_CALCULATION_ENERGY_H
#define DRAMPOWER_CALCULATION_ENERGY_H

#pragma once

#include <vector>

namespace DRAMPower {

struct energy_info_t
{
	double E_act = 0.0;
	double E_pre = 0.0;
	double E_bg_act = 0.0;
	double E_bg_pre = 0.0;

	double E_RD = 0.0;
	double E_WR = 0.0;
	double E_RDA = 0.0;
	double E_WRA = 0.0;
	double E_pre_RDA = 0.0;
	double E_pre_WRA = 0.0;

    double E_ref_AB = 0.0;
	double E_ref_PB = 0.0;
	double E_ref_SB = 0.0;
	double E_ref_2B = 0.0;

	double total() const;
	energy_info_t& operator+=(const energy_info_t & other);
};

struct energy_t
{
	std::vector<energy_info_t> bank_energy;
	energy_info_t total_energy();

	double E_bg_act_shared = 0.0;
	double E_PDNA = 0.0;
	double E_PDNP = 0.0;
	double E_sref = 0.0;
	double E_dsm = 0.0;
	double E_refab = 0.0;

	energy_t(std::size_t num_banks) : bank_energy(num_banks) {};
};

struct interface_power_t
{
	double dynamicPower = 0.0;
	double staticPower = 0.0;

	interface_power_t &operator+=(const interface_power_t &rhs) {
		dynamicPower += rhs.dynamicPower;
		staticPower += rhs.staticPower;
		return *this;
	}

	friend interface_power_t operator+(interface_power_t lhs, const interface_power_t &rhs) {
		lhs += rhs;
		return lhs;
	}
};



struct interface_energy_info_t
{
	interface_power_t controller;
	interface_power_t dram;

    interface_energy_info_t &operator+=(const interface_energy_info_t &rhs) {
        controller += rhs.controller;
        dram += rhs.dram;
        return *this;
    }

	friend interface_energy_info_t operator+(interface_energy_info_t lhs,
                                             const interface_energy_info_t &rhs) {
        lhs += rhs;
        return lhs;
    }
};

};

#endif /* DRAMPOWER_CALCULATION_ENERGY_H */
