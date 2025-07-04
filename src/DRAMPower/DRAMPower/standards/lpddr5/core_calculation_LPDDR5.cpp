#include "core_calculation_LPDDR5.h"
#include "DRAMPower/data/stats.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>

namespace DRAMPower {

    Calculation_LPDDR5::Calculation_LPDDR5(const MemSpecLPDDR5 &memSpec)
        : m_memSpec(memSpec)
    {}

    double Calculation_LPDDR5::E_pre(double VDD, double IBeta, double IDD2_N, double t_RP, uint64_t N_pre) {
        return VDD * (IBeta - IDD2_N) * t_RP * N_pre;
    }

    double Calculation_LPDDR5::E_act(double VDD, double I_theta, double I_1, double t_RAS, uint64_t N_act) {
        return VDD * (I_theta - I_1) * t_RAS * N_act;
    }

    double Calculation_LPDDR5::E_BG_pre(std::size_t B, double VDD, double IDD2_N, double T_BG_pre) {
        return (1.0 / B) * VDD * IDD2_N * T_BG_pre;
    }

    double Calculation_LPDDR5::E_BG_act_star(std::size_t B, double VDD, double IDD3_N, double I_p, double T_BG_act_star) {
        return VDD * (1.0 / B) * (IDD3_N - I_p) * T_BG_act_star;
    }

    double Calculation_LPDDR5::E_BG_act_shared(double VDD, double I_p, double T_bg_act) {
        return VDD * I_p * T_bg_act;
    }

    double Calculation_LPDDR5::E_RD(double VDD, double IDD4_R, double I_i, std::size_t BL, std::size_t DR, double t_WCK, uint64_t N_RD) {
        return VDD * (IDD4_R - I_i) * (BL / DR) * t_WCK * N_RD;
    }

    double Calculation_LPDDR5::E_WR(double VDD, double IDD4_R, double I_i, std::size_t BL, std::size_t DR, double t_WCK, uint64_t N_WR) {
        return VDD * (IDD4_R - I_i) * (BL / DR) * t_WCK * N_WR;
    }

    double Calculation_LPDDR5::E_ref_ab(std::size_t B, double VDD, double IDD5B, double IDD3_N, double tRFC, uint64_t N_REF) {
        return (1.0 / B) * VDD * (IDD5B - IDD3_N) * tRFC * N_REF;
    }

    double Calculation_LPDDR5::E_ref_pb(double VDD, double IDD5PB_B, double I_1, double tRFCPB, uint64_t N_PB_REF) {
        return VDD * (IDD5PB_B - I_1) * tRFCPB * N_PB_REF;
    }

    double Calculation_LPDDR5::E_ref_p2b(double VDD, double IDD5PB_B, double I_2, double tRFCPB, uint64_t N_P2B_REF) {
        return 0.5 * VDD * (IDD5PB_B - I_2) * tRFCPB * N_P2B_REF;
    }

    energy_t Calculation_LPDDR5::calcEnergy(const SimulationStats &stats) {
        auto t_CK = m_memSpec.memTimingSpec.tCK;
        auto t_WCK = m_memSpec.memTimingSpec.tWCK;
        auto t_RAS = m_memSpec.memTimingSpec.tRAS * t_CK;
        auto t_RP = m_memSpec.memTimingSpec.tRP * t_CK;
        auto t_RFC = m_memSpec.memTimingSpec.tRFC * t_CK;
        auto t_RFCPB = m_memSpec.memTimingSpec.tRFCPB * t_CK;
        auto t_REFI = m_memSpec.memTimingSpec.tREFI * t_CK;

        auto rho = m_memSpec.bwParams.bwPowerFactRho;
        auto BL = m_memSpec.burstLength;
        auto DR = m_memSpec.dataRate;
        auto B = m_memSpec.numberOfBanks;

        energy_t energy(m_memSpec.numberOfBanks * m_memSpec.numberOfRanks * m_memSpec.numberOfDevices);

        for (auto vd : {MemSpecLPDDR5::VoltageDomain::VDD1, MemSpecLPDDR5::VoltageDomain::VDD2H, MemSpecLPDDR5::VoltageDomain::VDD2L}) {
            auto VDD = m_memSpec.memPowerSpec[vd].vDDX;
            auto IDD_0 = m_memSpec.memPowerSpec[vd].iDD0X;
            auto IDD2N = m_memSpec.memPowerSpec[vd].iDD2NX;
            auto IDD3N = m_memSpec.memPowerSpec[vd].iDD3NX;
            auto IDD2P = m_memSpec.memPowerSpec[vd].iDD2PX;
            auto IDD3P = m_memSpec.memPowerSpec[vd].iDD3PX;
            auto IDD4R = m_memSpec.memPowerSpec[vd].iDD4RX;
            auto IDD4W = m_memSpec.memPowerSpec[vd].iDD4WX;
            auto IDD5 = m_memSpec.memPowerSpec[vd].iDD5X;
            auto IDD5PB = m_memSpec.memPowerSpec[vd].iDD5PBX;
            auto IDD6 = m_memSpec.memPowerSpec[vd].iDD6X;
            auto IDD6DS = m_memSpec.memPowerSpec[vd].iDD6DSX;
            auto IBeta = m_memSpec.memPowerSpec[vd].iBeta;

            auto I_rho = rho * (IDD3N - IDD2N) + IDD2N;
            auto I_2 = IDD3N + (IDD3N - I_rho);
            auto I_theta = (IDD_0 * (t_RP + t_RAS) - IBeta * t_RP) * (1 / t_RAS);
            auto IDD5PB_B =
                (IDD5PB * (t_REFI / 8) - IDD2N * ((t_REFI / 8) - t_RFCPB)) * (1.0 / t_RFCPB);
            auto approx_IDD3N = I_rho + B * (IDD3N - I_rho);

            size_t energy_offset = 0;
            size_t bank_offset = 0;
            for (size_t i = 0; i < m_memSpec.numberOfRanks; ++i) {
                for (size_t d = 0; d < m_memSpec.numberOfDevices; ++d) {
                    energy_offset = i * m_memSpec.numberOfDevices * m_memSpec.numberOfBanks
                                    + d * m_memSpec.numberOfBanks;
                    bank_offset = i * m_memSpec.numberOfBanks;
                    for (std::size_t b = 0; b < m_memSpec.numberOfBanks; ++b) {
                        const auto &bank = stats.bank[bank_offset + b];

                        energy.bank_energy[energy_offset + b].E_act +=
                            E_act(VDD, I_theta, IDD3N, t_RAS, bank.counter.act);
                        energy.bank_energy[energy_offset + b].E_pre +=
                            E_pre(VDD, IBeta, IDD2N, t_RP, bank.counter.pre);
                        energy.bank_energy[energy_offset + b].E_bg_act +=
                            E_BG_act_star(B, VDD, approx_IDD3N, I_rho,
                                        stats.bank[bank_offset + b].cycles.activeTime() * t_CK);
                        energy.bank_energy[energy_offset + b].E_bg_pre +=
                            E_BG_pre(B, VDD, IDD2N, stats.rank_total[i].cycles.pre * t_CK);
                        if (m_memSpec.bank_arch == MemSpecLPDDR5::MBG) {
                            energy.bank_energy[energy_offset + b].E_RD +=
                                E_RD(VDD, IDD4R, I_2, BL, DR, t_WCK, bank.counter.reads);
                            energy.bank_energy[energy_offset + b].E_WR +=
                                E_WR(VDD, IDD4W, I_2, BL, DR, t_WCK, bank.counter.writes);
                            energy.bank_energy[energy_offset + b].E_RDA +=
                                E_RD(VDD, IDD4R, I_2, BL, DR, t_WCK, bank.counter.readAuto);
                            energy.bank_energy[energy_offset + b].E_WRA +=
                                E_WR(VDD, IDD4W, I_2, BL, DR, t_WCK, bank.counter.writeAuto);
                        } else {
                            energy.bank_energy[energy_offset + b].E_RD +=
                                E_RD(VDD, IDD4R, IDD3N, BL, DR, t_WCK, bank.counter.reads);
                            energy.bank_energy[energy_offset + b].E_WR +=
                                E_WR(VDD, IDD4W, IDD3N, BL, DR, t_WCK, bank.counter.writes);
                            energy.bank_energy[energy_offset + b].E_RDA +=
                                E_RD(VDD, IDD4R, IDD3N, BL, DR, t_WCK, bank.counter.readAuto);
                            energy.bank_energy[energy_offset + b].E_WRA +=
                                E_WR(VDD, IDD4W, IDD3N, BL, DR, t_WCK, bank.counter.writeAuto);
                        }
                        energy.bank_energy[energy_offset + b].E_pre_RDA +=
                            E_pre(VDD, IBeta, IDD2N, t_RP, bank.counter.readAuto);
                        energy.bank_energy[energy_offset + b].E_pre_WRA +=
                            E_pre(VDD, IBeta, IDD2N, t_RP, bank.counter.writeAuto);
                        energy.bank_energy[energy_offset + b].E_ref_AB +=
                            E_ref_ab(B, VDD, IDD5, approx_IDD3N, t_RFC, bank.counter.refAllBank);
                        energy.bank_energy[energy_offset + b].E_ref_PB +=
                            E_ref_pb(VDD, IDD5PB_B, IDD3N, t_RFCPB, bank.counter.refPerBank);
                        energy.bank_energy[energy_offset + b].E_ref_2B +=
                            E_ref_p2b(VDD, IDD5PB_B, I_2, t_RFCPB, bank.counter.refPerTwoBanks);
                    }
                }

                energy.E_sref += VDD * IDD6 * stats.rank_total[i].cycles.selfRefresh * t_CK * m_memSpec.numberOfDevices;
                energy.E_PDNA += VDD * IDD3P * stats.rank_total[i].cycles.powerDownAct * t_CK * m_memSpec.numberOfDevices;
                energy.E_PDNP += VDD * IDD2P * stats.rank_total[i].cycles.powerDownPre * t_CK * m_memSpec.numberOfDevices;
                energy.E_dsm += VDD * IDD6DS * stats.rank_total[i].cycles.deepSleepMode * t_CK * m_memSpec.numberOfDevices;
                energy.E_bg_act_shared +=
                    E_BG_act_shared(VDD, I_rho, stats.rank_total[i].cycles.act * t_CK) * m_memSpec.numberOfDevices;
            }
        }

        return energy;
    }
}
