#ifndef DRAMPOWER_STANDARDS_HBM2_CALCULATION_HBM2_H
#define DRAMPOWER_STANDARDS_HBM2_CALCULATION_HBM2_H

#pragma once

#include <DRAMPower/data/energy.h>

#include <DRAMPower/Types.h>
#include <DRAMPower/memspec/MemSpecHBM2.h>
#include <DRAMPower/data/stats.h>

#include <cstddef>
#include <cstdint>

namespace DRAMPower 
{

class HBM2;

class Calculation_HBM2
{
public:
	Calculation_HBM2(const MemSpecHBM2 &memSpec);

private:
	inline double E_act(double VDD, double I_theta, double I_1, double t_RAS, uint64_t N_act) const;
	inline double E_pre(double VDD, double IBeta, double IDD2_N, double t_RP, uint64_t N_pre) const;
	inline double E_BG_pre(std::size_t B, double VDD, double IDD2_N, double T_BG_pre) const;
	inline double E_BG_act_star(double VDD, double I_1, double I_rho, double T_BG_act_star) const;
	inline double E_BG_act_shared(double VDD, double I_rho, double T_BG_act) const;
	inline double E_RD(double VDD, double IDD4_R, double I_B, double t_CK, std::size_t BL, std::size_t DR, uint64_t N_RD) const;
	inline double E_WR(double VDD, double IDD4_W, double I_B, double t_CK, std::size_t BL, std::size_t DR, uint64_t N_WR) const;
	inline double E_ref_ab(std::size_t B, double VDD, double IDD5B, double I_B, double tRFC, uint64_t N_REF) const;
	inline double E_ref_pb(double VDD, double IDD5PB_B, double I_1, double tRFCPB, uint64_t N_PB_REF) const;

public:
	energy_t calcEnergy(const SimulationStats &stats);
private:
	const MemSpecHBM2 &m_memSpec;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_HBM2_CALCULATION_HBM2_H */
