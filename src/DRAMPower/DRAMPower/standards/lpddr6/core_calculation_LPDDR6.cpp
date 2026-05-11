#include "core_calculation_LPDDR6.h"

#include <DRAMPower/standards/lpddr6/LPDDR6.h>

namespace DRAMPower {

    Calculation_LPDDR6::Calculation_LPDDR6(const MemSpecLPDDR6 &memSpec)
        : m_memSpec(memSpec)
    {}

    double Calculation_LPDDR6::E_pre(double VDD, double IBeta, double IDD2_N, double t_RP, uint64_t N_pre) const {
        return VDD * (IBeta - IDD2_N) * t_RP * N_pre;
    }

    double Calculation_LPDDR6::E_act(double VDD, double I_theta, double I_1, double t_RAS, uint64_t N_act) const {
        return VDD * (I_theta - I_1) * t_RAS * N_act;
    }

    double Calculation_LPDDR6::E_BG_pre(std::size_t B, double VDD, double IDD2_N, double T_BG_pre) const {
        return (1.0 / B) * VDD * IDD2_N * T_BG_pre;
    }

    double Calculation_LPDDR6::E_BG_act_star(double VDD, double I_1, double I_rho, double T_BG_act_star) const {
        return VDD * (I_1 - I_rho) * T_BG_act_star;
    }

    double Calculation_LPDDR6::E_BG_act_shared(double VDD, double I_rho, double T_bg_act) const {
        return VDD * I_rho * T_bg_act;
    }

    double Calculation_LPDDR6::E_RD(double VDD, double IDD4_R, double I_2, std::size_t BL, std::size_t DR, double t_WCK, uint64_t N_RD) const {
        return VDD * (IDD4_R - I_2) * (BL / DR) * t_WCK * N_RD;
    }

    double Calculation_LPDDR6::E_WR(double VDD, double IDD4_W, double I_2, std::size_t BL, std::size_t DR, double t_WCK, uint64_t N_WR) const {
        return VDD * (IDD4_W - I_2) * (BL / DR) * t_WCK * N_WR;
    }

    double Calculation_LPDDR6::E_ref_ab(std::size_t B, double VDD, double IDD5B, double I_B, double tRFC, uint64_t N_REF) const {
        return (1.0 / B) * VDD * (IDD5B - I_B) * tRFC * N_REF;
    }

    double Calculation_LPDDR6::E_ref_db(double VDD, double IDD5DB_B, double I_2, double tRFCDB, uint64_t N_DB_REF) const {
        // halved cause only half of the energy is contributed by this bank
        return 0.5 * VDD * (IDD5DB_B - I_2) * tRFCDB * N_DB_REF;
    }

    energy_t Calculation_LPDDR6::calcEnergy(const SimulationStats &stats) const {
        auto t_CK = m_memSpec.memTimingSpec.tCK;
        auto t_WCK = m_memSpec.memTimingSpec.tWCK;
        auto t_RAS = m_memSpec.memTimingSpec.tRAS * t_CK;
        auto t_RP = m_memSpec.memTimingSpec.tRP * t_CK;
        auto t_RFCAB = m_memSpec.memTimingSpec.tRFCAB * t_CK;
        auto t_RFCDB = m_memSpec.memTimingSpec.tRFCDB * t_CK;
        auto t_REFI = m_memSpec.memTimingSpec.tREFI * t_CK;

        auto rho = m_memSpec.bwParams.bwPowerFactRho;
        auto BL = m_memSpec.burstLength;
        auto DR = m_memSpec.dataRate;
        auto B = m_memSpec.numberOfBanks;

        energy_t energy(m_memSpec.numberOfBanks * m_memSpec.numberOfRanks * m_memSpec.numberOfDevices);

        for (auto vd : {MemSpecLPDDR6::VoltageDomain::VDD1, MemSpecLPDDR6::VoltageDomain::VDD2C, MemSpecLPDDR6::VoltageDomain::VDD2D}) {
            auto VDD = m_memSpec.memPowerSpec[vd].vDDX;
            auto IDD_0 = m_memSpec.memPowerSpec[vd].iDD0X;
            auto IDD2N = m_memSpec.memPowerSpec[vd].iDD2NX;
            auto I_1 = m_memSpec.memPowerSpec[vd].iDD3NX;
            auto IDD2P = m_memSpec.memPowerSpec[vd].iDD2PX;
            auto IDD3P = m_memSpec.memPowerSpec[vd].iDD3PX;
            auto IDD4R = m_memSpec.memPowerSpec[vd].iDD4RX;
            auto IDD4W = m_memSpec.memPowerSpec[vd].iDD4WX;
            auto IDD5 = m_memSpec.memPowerSpec[vd].iDD5X;
            auto IDD5PDB = m_memSpec.memPowerSpec[vd].iDD5PDBX;
            auto IDD6 = m_memSpec.memPowerSpec[vd].iDD6X;
            auto IDD6DS = m_memSpec.memPowerSpec[vd].iDD6DSX;
            auto IBeta = m_memSpec.memPowerSpec[vd].iBeta;

            auto t1 = B * rho;
            auto t2 = 1 - rho;
            auto I_rho = (t1 * I_1 + t2 * IDD2N) / (t1 + t2);
            auto I_B = I_rho + B * (I_1 - I_rho);
            auto I_2 = I_1 + (I_1 - I_rho);
            auto I_theta = (IDD_0 * (t_RP + t_RAS) - IBeta * t_RP) * (1 / t_RAS);
            auto IDD5PDB_B =
                (IDD5PDB * (t_REFI / 8) - IDD2N * ((t_REFI / 8) - t_RFCDB)) * (1.0 / t_RFCDB);

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
                            E_act(VDD, I_theta, I_1, t_RAS, bank.counter.act);
                        energy.bank_energy[energy_offset + b].E_pre +=
                            E_pre(VDD, IBeta, IDD2N, t_RP, bank.counter.pre);
                        energy.bank_energy[energy_offset + b].E_bg_act +=
                            E_BG_act_star(VDD, I_1, I_rho,
                                        stats.bank[bank_offset + b].cycles.activeTime() * t_CK);
                        energy.bank_energy[energy_offset + b].E_bg_pre +=
                            E_BG_pre(B, VDD, IDD2N, stats.rank_total[i].cycles.pre * t_CK);
                        energy.bank_energy[energy_offset + b].E_RD +=
                            E_RD(VDD, IDD4R, I_2, BL, DR, t_WCK, bank.counter.reads);
                        energy.bank_energy[energy_offset + b].E_WR +=
                            E_WR(VDD, IDD4W, I_2, BL, DR, t_WCK, bank.counter.writes);
                        energy.bank_energy[energy_offset + b].E_RDA +=
                            E_RD(VDD, IDD4R, I_2, BL, DR, t_WCK, bank.counter.readAuto);
                        energy.bank_energy[energy_offset + b].E_WRA +=
                            E_WR(VDD, IDD4W, I_2, BL, DR, t_WCK, bank.counter.writeAuto);
                        energy.bank_energy[energy_offset + b].E_pre_RDA +=
                            E_pre(VDD, IBeta, IDD2N, t_RP, bank.counter.readAuto);
                        energy.bank_energy[energy_offset + b].E_pre_WRA +=
                            E_pre(VDD, IBeta, IDD2N, t_RP, bank.counter.writeAuto);
                        energy.bank_energy[energy_offset + b].E_ref_AB +=
                            E_ref_ab(B, VDD, IDD5, I_B, t_RFCAB, bank.counter.refAllBank);
                        energy.bank_energy[energy_offset + b].E_ref_DB +=
                            E_ref_db(VDD, IDD5PDB_B, I_2, t_RFCDB, bank.counter.refDualBanks);
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
