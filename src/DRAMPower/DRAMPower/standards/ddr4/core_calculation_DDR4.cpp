#include "core_calculation_DDR4.h"

#include <DRAMPower/standards/ddr4/DDR4.h>

namespace DRAMPower {

    inline double Calculation_DDR4::E_BG_pre(std::size_t B, double VDD, double IDD2_N, double T_BG_pre) {
		return (1.0 / B) * VDD * IDD2_N * T_BG_pre;
	};

	inline double Calculation_DDR4::E_BG_act_shared(double VDD, double I_rho, double T_BG_act){
		return VDD * I_rho * T_BG_act;
	}

	inline double Calculation_DDR4::E_BG_act_star(std::size_t B, double VDD, double IDD3_N, double I_rho, double T_BG_act_star){
        return VDD * (1.0 / B) * (IDD3_N - I_rho) * T_BG_act_star;
	}

    inline double Calculation_DDR4::E_pre(double VDD, double IBeta, double IDD2_N, double t_RP, uint64_t N_pre) {
		return VDD * (IBeta - IDD2_N) * t_RP * N_pre;
	}

	inline double Calculation_DDR4::E_act(double VDD, double I_theta, double I_1, double t_RAS, uint64_t N_act) {
		return VDD * (I_theta - I_1) * t_RAS * N_act;
	}

	inline double Calculation_DDR4::E_RD(double VDD, double IDD4_R, double IDD3_N, double t_CK, std::size_t BL, std::size_t DR, uint64_t N_RD) {
        return VDD * (IDD4_R - IDD3_N) * (double(BL) / DR) * t_CK * N_RD;
	}

	inline double Calculation_DDR4::E_WR(double VDD, double IDD4_W, double IDD3_N, double t_CK, std::size_t BL, std::size_t DR, uint64_t N_WR) {
		return VDD * (IDD4_W - IDD3_N) * (BL / DR) * t_CK * N_WR;
	}

	inline double Calculation_DDR4::E_ref_ab(std::size_t B, double VDD, double IDD5B, double IDD3_N, double tRFC, uint64_t N_REF) {
        return (1.0 / B) * VDD * (IDD5B - IDD3_N) * tRFC * N_REF;
	}

	energy_t Calculation_DDR4::calcEnergy(timestamp_t timestamp, DDR4 & dram) {
            auto stats = dram.getWindowStats(timestamp);
            
            // Timings
            double t_CK = dram.memSpec.memTimingSpec.tCK;
            auto t_RAS = dram.memSpec.memTimingSpec.tRAS * t_CK;
            auto t_RP = dram.memSpec.memTimingSpec.tRP * t_CK;
            auto t_RFC = dram.memSpec.memTimingSpec.tRFC * t_CK;

            auto rho = dram.memSpec.bwParams.bwPowerFactRho;
            auto BL = dram.memSpec.burstLength;
            auto DR = dram.memSpec.dataRate;
            auto B = dram.memSpec.numberOfBanks;

            energy_t energy(dram.memSpec.numberOfBanks * dram.memSpec.numberOfRanks * dram.memSpec.numberOfDevices);

            for (auto vd : {MemSpecDDR4::VoltageDomain::VDD}) {
                auto VXX = dram.memSpec.memPowerSpec[vd].vXX;
                auto IXX_0 = dram.memSpec.memPowerSpec[vd].iXX0;
                auto IXX2N = dram.memSpec.memPowerSpec[vd].iXX2N;
                auto IXX3N = dram.memSpec.memPowerSpec[vd].iXX3N;
                auto IXX2P = dram.memSpec.memPowerSpec[vd].iXX2P;
                auto IXX3P = dram.memSpec.memPowerSpec[vd].iXX3P;
                auto IXX4R = dram.memSpec.memPowerSpec[vd].iXX4R;
                auto IXX4W = dram.memSpec.memPowerSpec[vd].iXX4W;
                auto IXX5X = dram.memSpec.memPowerSpec[vd].iXX5X;
                auto IXX6N = dram.memSpec.memPowerSpec[vd].iXX6N;
                auto IBeta = dram.memSpec.memPowerSpec[vd].iBeta;

                auto I_rho = rho * (IXX3N - IXX2N) + IXX2N;
                auto I_theta = (IXX_0 * (t_RP + t_RAS) - IBeta * t_RP) * (1 / t_RAS);
                auto I_1 = (1.0 / B) * (IXX3N + (B - 1) * (rho * (IXX3N - IXX2N) + IXX2N));

                size_t energy_offset = 0;
                size_t bank_offset = 0;
                for (size_t r = 0; r < dram.memSpec.numberOfRanks; ++r) {
                    for(size_t d = 0; d < dram.memSpec.numberOfDevices; d++)
                    {
                        energy_offset = r * dram.memSpec.numberOfDevices * dram.memSpec.numberOfBanks +
                            d * dram.memSpec.numberOfBanks;
                        // Bank offset doesn't include numberOfDevices, because one device is simulated
                        // The stats only contain one device per rank
                        bank_offset = r * dram.memSpec.numberOfBanks;
                        for (size_t b = 0; b < dram.memSpec.numberOfBanks; ++b) {
                            const auto &bank = stats.bank[bank_offset + b];

                            energy.bank_energy[energy_offset + b].E_act +=
                                E_act(VXX, I_theta, I_1, t_RAS, bank.counter.act);
                            energy.bank_energy[energy_offset + b].E_pre +=
                                E_pre(VXX, IBeta, IXX2N, t_RP, bank.counter.pre);
                            energy.bank_energy[energy_offset + b].E_bg_act +=
                                E_BG_act_star(B, VXX, IXX3N, I_rho,
                                    stats.bank[bank_offset + b].cycles.activeTime() * t_CK);
                            energy.bank_energy[energy_offset + b].E_bg_pre +=
                                E_BG_pre(B, VXX, IXX2N, stats.rank_total[r].cycles.pre * t_CK);
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
                        }
                    }

                    energy.E_sref += VXX * IXX6N * stats.rank_total[r].cycles.selfRefresh * t_CK * dram.memSpec.numberOfDevices;
                    energy.E_PDNA += VXX * IXX3P * stats.rank_total[r].cycles.powerDownAct * t_CK * dram.memSpec.numberOfDevices;
                    energy.E_PDNP += VXX * IXX2P * stats.rank_total[r].cycles.powerDownPre * t_CK * dram.memSpec.numberOfDevices;

                    energy.E_bg_act_shared +=
                        E_BG_act_shared(VXX, I_rho, stats.rank_total[r].cycles.act * t_CK) * dram.memSpec.numberOfDevices;
                }
            }

                return energy;
	}
}
