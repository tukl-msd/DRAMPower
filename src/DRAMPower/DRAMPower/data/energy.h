#ifndef DRAMPOWER_CALCULATION_ENERGY_H
#define DRAMPOWER_CALCULATION_ENERGY_H

#pragma once

#include <vector>

#include <DRAMUtils/util/json_utils.h>

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
	void to_json(json_t &j) const;
	energy_info_t& operator+=(const energy_info_t & other);
};

struct energy_t
{
	std::vector<energy_info_t> bank_energy;
	energy_info_t aggregated_bank_energy() const;
	void to_json(json_t &j) const;
	constexpr inline const char * get_Bank_energy_keyword() const
	{
		return "BankEnergy";
	}



	double E_bg_act_shared = 0.0;
	double E_PDNA = 0.0;
	double E_PDNP = 0.0;
	double E_sref = 0.0;
	double E_dsm = 0.0;
	double E_refab = 0.0;

	energy_t(std::size_t num_banks) : bank_energy(num_banks) {};

	double total() const;
};

struct interface_energy_t
{
	double dynamicEnergy = 0.0;
	double staticEnergy = 0.0;

	interface_energy_t &operator+=(const interface_energy_t &rhs) {
		dynamicEnergy += rhs.dynamicEnergy;
		staticEnergy += rhs.staticEnergy;
		return *this;
	}

	friend interface_energy_t operator+(interface_energy_t lhs, const interface_energy_t &rhs) {
		lhs += rhs;
		return lhs;
	}
};



struct interface_energy_info_t
{
	interface_energy_t controller;
	interface_energy_t dram;

    double total() const {
        return controller.dynamicEnergy + controller.staticEnergy + dram.dynamicEnergy + dram.staticEnergy;
    }

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

	void to_json(json_t &j) const;
};

};

#endif /* DRAMPOWER_CALCULATION_ENERGY_H */
