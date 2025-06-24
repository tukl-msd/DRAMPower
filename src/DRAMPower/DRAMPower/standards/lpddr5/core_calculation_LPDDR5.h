#ifndef DRAMPOWER_STANDARDS_LPDDR5_CALCULATION_LPDDR5_H
#define DRAMPOWER_STANDARDS_LPDDR5_CALCULATION_LPDDR5_H

#include "DRAMPower/data/stats.h"
#pragma once

#include "DRAMPower/memspec/MemSpecLPDDR5.h"
#include <DRAMPower/data/energy.h>

#include <DRAMPower/Types.h>

#include <cstddef>
#include <cstdint>

namespace DRAMPower
{

    class LPDDR5;

    class Calculation_LPDDR5
    {
    public:
        Calculation_LPDDR5(const MemSpecLPDDR5 &memSpec);
    private:
        double E_act(double VDD, double I_theta, double I_1, double t_RAS, uint64_t N_act);
        double E_pre(double VDD, double IBeta, double IDD2_N, double t_RP, uint64_t N_pre);
        double E_BG_pre(std::size_t B, double VDD, double IDD2_N, double T_BG_pre);
        double E_BG_act_star(std::size_t B, double VDD, double IDD3_N, double I_p, double T_BG_act_star);
        double E_BG_act_shared(double VDD, double I_p, double T_bg_act);
        double E_RD(double VDD, double IDD4_R, double I_i, std::size_t BL, std::size_t DR, double t_WCK, uint64_t N_RD);
        double E_WR(double VDD, double IDD4_W, double I_i, std::size_t BL, std::size_t DR, double t_WCK, uint64_t N_WR);
        double E_ref_ab(std::size_t B, double VDD, double IDD5B, double IDD3_N, double tRFC, uint64_t N_REF);
        double E_ref_pb(double VDD, double IDD5PB_B, double I_1, double tRFCPB, uint64_t N_PB_REF);
        double E_ref_p2b(double VDD, double IDD5PB_B, double I_2, double tRFCPB, uint64_t N_P2B_REF);

    public:
        energy_t calcEnergy(const SimulationStats &stats);
    private:
        const MemSpecLPDDR5 &m_memSpec;
    };

};


#endif /* DRAMPOWER_STANDARDS_LPDDR5_CALCULATION_LPDDR5_H */
