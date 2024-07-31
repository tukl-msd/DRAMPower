#ifndef DRAMPOWER_CALCULATION_ENERGY_H
#define DRAMPOWER_CALCULATION_ENERGY_H

#pragma once

#include <vector>
#include <iostream>

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
	// Operator << for std::cout
	friend std::ostream & operator<<(std::ostream & os, const energy_info_t & ei)
	{
		os << "ACT: " << ei.E_act << " ";
		os << "PRE: " << ei.E_pre << " ";
		os << "BG_ACT: " << ei.E_bg_act << " ";
		os << "BG_PRE: " << ei.E_bg_pre << " ";
		os << "RD: " << ei.E_RD << " ";
		os << "WR: " << ei.E_WR << " ";
		os << "RDA: " << ei.E_RDA << " ";
		os << "WRA: " << ei.E_WRA << " ";
		os << "PRE_RDA: " << ei.E_pre_RDA << " ";
		os << "PRE_WRA: " << ei.E_pre_WRA << " ";
		os << "REF_AB: " << ei.E_ref_AB << " ";
		os << "REF_PB: " << ei.E_ref_PB << " ";
		os << "REF_SB: " << ei.E_ref_SB << " ";
		os << "REF_2B: " << ei.E_ref_2B << " ";

		return os;
	}
};

struct energy_t
{
	std::vector<energy_info_t> bank_energy;
	energy_info_t total_energy(); // TODO rename
	void to_json(json_t &j) const;
	constexpr inline const char * const get_Bank_energy_keyword() const
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
	// get fields of os stream output
	friend std::ostream & operator<<(std::ostream & os, const energy_t & e)
	{
		os << "E_bg_act_shared: " << e.E_bg_act_shared << " ";
		os << "E_PDNA: " << e.E_PDNA << " ";
		os << "E_PDNP: " << e.E_PDNP << " ";
		os << "E_sref: " << e.E_sref << " ";
		os << "E_dsm: " << e.E_dsm << " ";
		os << "E_refab: " << e.E_refab << " ";

    	return os;
	}
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

    friend std::ostream & operator<<(std::ostream & os, const interface_energy_info_t & e)
	{
		os << "Controller: dynamicEnergy: " << e.controller.dynamicEnergy << " ";
        os << "staticEnergy: " << e.controller.staticEnergy << std::endl;
        os << "DRAM: dynamicEnergy: " << e.dram.dynamicEnergy << " ";
        os << "staticEnergy: " << e.dram.staticEnergy << std::endl;
        os << "Total: " << e.total() << std::endl;
    	return os;
	}
	void to_json(json_t &j) const;
};

};

#endif /* DRAMPOWER_CALCULATION_ENERGY_H */
