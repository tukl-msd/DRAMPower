#include "core_calculation_DDR5.h"

#include <DRAMPower/standards/ddr5/DDR5.h>

namespace DRAMPower {

    Calculation_DDR5::Calculation_DDR5(const MemSpecDDR5 &memSpec)
        : m_memSpec(memSpec)
    {}

    double Calculation_DDR5::E_BG_pre(std::size_t B, double VDD, double IDD2_N, double T_BG_pre) {
        return (1.0 / B) * VDD * IDD2_N * T_BG_pre;
    };

    double Calculation_DDR5::E_BG_act_shared(double VDD, double I_rho, double T_BG_act) {
        return VDD * I_rho * T_BG_act;
    }

    double
    Calculation_DDR5::E_BG_act_star(std::size_t B, double VDD, double IDD3_N, double I_rho, double T_BG_act_star) {
        return VDD * (1.0 / B) * (IDD3_N - I_rho) * T_BG_act_star;
    }

    double Calculation_DDR5::E_pre(double VDD, double IBeta, double IDD2_N, double t_RP, uint64_t N_pre) {
        return VDD * (IBeta - IDD2_N) * t_RP * N_pre;
    }

    double Calculation_DDR5::E_act(double VDD, double I_theta, double I_1, double t_RAS, uint64_t N_act) {
        return VDD * (I_theta - I_1) * t_RAS * N_act;
    }

    double Calculation_DDR5::E_RD(double VDD, double IDD4_R, double IDD3_N, double t_CK, std::size_t BL, std::size_t DR,
                                  uint64_t N_RD) {
        return VDD * (IDD4_R - IDD3_N) * (double(BL) / DR) * t_CK * N_RD;
    }

    double Calculation_DDR5::E_WR(double VDD, double IDD4_W, double IDD3_N, double t_CK, std::size_t BL, std::size_t DR,
                                  uint64_t N_WR) {
        return VDD * (IDD4_W - IDD3_N) * (BL / DR) * t_CK * N_WR;
    }

    double
    Calculation_DDR5::E_ref_ab(std::size_t B, double VDD, double IDD5B, double IDD3_N, double tRFC, uint64_t N_REF) {
        return (1.0 / B) * VDD * (IDD5B - IDD3_N) * tRFC * N_REF;
    }

    double Calculation_DDR5::E_ref_sb(double VDD, double IDD5C, double I_BG, double tRFCsb, std::size_t BG,
                                      uint64_t N_SB_REF) {
        return VDD * (IDD5C - I_BG) * tRFCsb * N_SB_REF * (1.0 / BG);
    }

    energy_t Calculation_DDR5::calcEnergy(const SimulationStats &stats) {
        double t_CK = m_memSpec.memTimingSpec.tCK;
        auto t_RAS = m_memSpec.memTimingSpec.tRAS * t_CK;
        auto t_RP = m_memSpec.memTimingSpec.tRP * t_CK;
        auto t_RFC = m_memSpec.memTimingSpec.tRFC * t_CK;
        auto t_RFCsb = m_memSpec.memTimingSpec.tRFCsb * t_CK;

        auto rho = m_memSpec.bwParams.bwPowerFactRho;
        auto BL = m_memSpec.burstLength;
        auto DR = m_memSpec.dataRate;
        auto B = m_memSpec.numberOfBanks;
        auto BG = m_memSpec.numberOfBankGroups;

        energy_t energy(m_memSpec.numberOfBanks * m_memSpec.numberOfRanks * m_memSpec.numberOfDevices);

        for (auto vd : {MemSpecDDR5::VoltageDomain::VDD, MemSpecDDR5::VoltageDomain::VPP}) {
            auto VXX = m_memSpec.memPowerSpec[vd].vXX;
            auto IXX_0 = m_memSpec.memPowerSpec[vd].iXX0;
            auto IXX2N = m_memSpec.memPowerSpec[vd].iXX2N;
            auto IXX3N = m_memSpec.memPowerSpec[vd].iXX3N;
            auto IXX2P = m_memSpec.memPowerSpec[vd].iXX2P;
            auto IXX3P = m_memSpec.memPowerSpec[vd].iXX3P;
            auto IXX4R = m_memSpec.memPowerSpec[vd].iXX4R;
            auto IXX4W = m_memSpec.memPowerSpec[vd].iXX4W;
            auto IXX5X = m_memSpec.memPowerSpec[vd].iXX5X;
            auto IXX5C = m_memSpec.memPowerSpec[vd].iXX5C;
            auto IXX6N = m_memSpec.memPowerSpec[vd].iXX6N;
            auto IBeta = m_memSpec.memPowerSpec[vd].iBeta;

            auto I_rho = rho * (IXX3N - IXX2N) + IXX2N;
            auto I_theta = (IXX_0 * (t_RP + t_RAS) - IBeta * t_RP) * (1 / t_RAS);
            auto I_1 = (1.0 / B) * (IXX3N + (B - 1) * (rho * (IXX3N - IXX2N) + IXX2N));
            auto I_BG = I_rho + (I_1 - I_rho) * BG;

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
                            E_act(VXX, I_theta, I_1, t_RAS, bank.counter.act);
                        energy.bank_energy[energy_offset + b].E_pre +=
                            E_pre(VXX, IBeta, IXX2N, t_RP, bank.counter.pre);
                        energy.bank_energy[energy_offset + b].E_bg_act +=
                            E_BG_act_star(B, VXX, IXX3N, I_rho,
                                        stats.bank[bank_offset + b].cycles.activeTime() * t_CK);
                        energy.bank_energy[energy_offset + b].E_bg_pre +=
                            E_BG_pre(B, VXX, IXX2N, stats.rank_total[i].cycles.pre * t_CK);
                        energy.bank_energy[energy_offset + b].E_RD +=
                            E_RD(VXX, IXX4R, IXX3N, t_CK, BL, DR, bank.counter.reads);
                        energy.bank_energy[energy_offset + b].E_WR +=
                            E_WR(VXX, IXX4W, IXX3N, t_CK, BL, DR, bank.counter.writes);
                        energy.bank_energy[energy_offset + b].E_RDA +=
                            E_RD(VXX, IXX4R, IXX3N, t_CK, BL, DR, bank.counter.readAuto);
                        energy.bank_energy[energy_offset + b].E_WRA +=
                            E_WR(VXX, IXX4W, IXX3N, t_CK, BL, DR, bank.counter.writeAuto);
                        energy.bank_energy[energy_offset + b].E_pre_RDA +=
                            E_pre(VXX, IBeta, IXX2N, t_RP, bank.counter.readAuto);
                        energy.bank_energy[energy_offset + b].E_pre_WRA +=
                            E_pre(VXX, IBeta, IXX2N, t_RP, bank.counter.writeAuto);
                        energy.bank_energy[energy_offset + b].E_ref_AB +=
                            E_ref_ab(B, VXX, IXX5X, IXX3N, t_RFC, bank.counter.refAllBank);
                        energy.bank_energy[energy_offset + b].E_ref_SB +=
                            E_ref_sb(VXX, IXX5C, I_BG, t_RFCsb, BG, bank.counter.refSameBank);
                    }
                }

                energy.E_sref += VXX * IXX6N * stats.rank_total[i].cycles.selfRefresh * t_CK * m_memSpec.numberOfDevices;
                energy.E_PDNA += VXX * IXX3P * stats.rank_total[i].cycles.powerDownAct * t_CK * m_memSpec.numberOfDevices;
                energy.E_PDNP += VXX * IXX2P * stats.rank_total[i].cycles.powerDownPre * t_CK * m_memSpec.numberOfDevices;

                energy.E_bg_act_shared +=
                    E_BG_act_shared(VXX, I_rho, stats.rank_total[i].cycles.act * t_CK) * m_memSpec.numberOfDevices;
            }
        }

        return energy;
    }
}
