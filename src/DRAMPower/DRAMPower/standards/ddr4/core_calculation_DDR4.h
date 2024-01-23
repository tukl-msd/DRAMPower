#ifndef DRAMPOWER_STANDARDS_DDR4_CALCULATION_DDR4_H
#define DRAMPOWER_STANDARDS_DDR4_CALCULATION_DDR4_H

#pragma once

#include <DRAMPower/data/energy.h>

#include <DRAMPower/Types.h>

#include <cstddef>
#include <cstdint>

namespace DRAMPower 
{

class DDR4;

class Calculation_DDR4
{
private:
	inline double E_act(double VDD, double I_theta, double I_1, double t_RAS, uint64_t N_act);
	inline double E_pre(double VDD, double IBeta, double IDD2_N, double t_RP, uint64_t N_pre);
	inline double E_BG_pre(std::size_t B, double VDD, double IDD2_N, double T_BG_pre);
	inline double E_BG_act_star(std::size_t B, double VDD, double IDD3_N, double I_rho, double T_BG_act_star);
	inline double E_BG_act_shared(double VDD, double I_rho, double T_BG_act);
	inline double E_RD(double VDD, double IDD4_R, double IDD3_N, double t_CK, std::size_t BL, std::size_t DR, uint64_t N_RD);
	inline double E_WR(double VDD, double IDD4_W, double IDD3_N, double t_CK, std::size_t BL, std::size_t DR, uint64_t N_WR);
	inline double E_ref_ab(std::size_t B, double VDD, double IDD5B, double IDD3_N, double tRFC, uint64_t N_REF);
	
public:
	energy_t calcEnergy(timestamp_t timestamp, DDR4 & dram);
};

};

#endif /* DRAMPOWER_STANDARDS_DDR4_CALCULATION_DDR4_H */
