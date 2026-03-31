#ifndef DRAMPOWER_STANDARDS_LPDDR4_CALCULATION_LPDDR4_H
#define DRAMPOWER_STANDARDS_LPDDR4_CALCULATION_LPDDR4_H

#include "DRAMPower/data/stats.h"
#pragma once

#include <DRAMPower/data/energy.h>
#include <DRAMPower/memspec/MemSpecLPDDR4.h>

#include <DRAMPower/Types.h>

#include <cstddef>
#include <cstdint>

namespace DRAMPower 
{

class LPDDR4;

class Calculation_LPDDR4
{
public:
    Calculation_LPDDR4(const MemSpecLPDDR4 &memSpec);

private:
    double E_act(double VDD, double I_theta, double IDD3N, double tRAS, uint64_t N_act) const;
    double E_pre(double VDD, double IBeta, double IDD2N, double tRP, uint64_t N_pre) const;
    double E_BG_pre(std::size_t B, double VDD, double IDD2N, double T_BG_pre) const;
    double E_BG_act_star(std::size_t B, double VDD, double IDD3N, double I_rho, double T_BG_act_star) const;
    double E_BG_act_shared(double VDD, double I_rho, double T_bg_act) const;
	double E_RD(double VDD, double IDD4R, double IDD3N, std::size_t BL, std::size_t DR, double t_CK, uint64_t N_RD) const;
	double E_WR(double VDD, double IDD4W, double IDD3N, std::size_t BL, std::size_t DR, double t_CK, uint64_t N_WR) const;
    double E_ref_ab(std::size_t B, double VDD, double IDD5B, double approx_IDD3N, double tRFC, uint64_t N_REF) const;
    double E_ref_pb(double VDD, double IDD5PB_B, double IDD3N, double tRFCPB, uint64_t N_PB_REF) const;

public:
	energy_t calcEnergy(const SimulationStats &stats) const;

private:
    const MemSpecLPDDR4 &m_memSpec;
};

};

#endif /* DRAMPOWER_STANDARDS_LPDDR4_CALCULATION_LPDDR4_H */
