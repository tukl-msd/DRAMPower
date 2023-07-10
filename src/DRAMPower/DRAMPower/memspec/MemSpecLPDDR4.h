#ifndef DRAMPOWER_MEMSPEC_MEMSPECLPDDR4_H
#define DRAMPOWER_MEMSPEC_MEMSPECLPDDR4_H

#include "MemSpec.h"


namespace DRAMPower {

class MemSpecLPDDR4 final : public MemSpec
{
public:

    enum VoltageDomain {
        VDD = 0,
        VDDQ = 1
    };

public:
	MemSpecLPDDR4() = default;
	MemSpecLPDDR4(nlohmann::json &memspec);

	~MemSpecLPDDR4() = default;
	uint64_t timeToCompletion(CmdType type) override;

    unsigned numberOfBankGroups;
    unsigned banksPerGroup;
    unsigned numberOfRanks;

	struct MemTimingSpec
	{
		double fCKMHz;
		double tCK;
		uint64_t tRAS;
		uint64_t tRCD;
		uint64_t tRL;
		uint64_t tRTP;
		uint64_t tWL;
		uint64_t tWR;
		uint64_t tREFI;
		uint64_t tRFC;
		uint64_t tRFCPB;
		uint64_t tRP;
	};

	// Currents and Voltages:
	struct MemPowerSpec
	{
        double vDDX;
        double iDD0X;
        double iDD2NX;
        double iDD3NX;
        double iDD2PX;
        double iDD3PX;
        double iDD4RX;
        double iDD4WX;
        double iDD5X;
        double iDD5PBX;
        double iDD6X;
        double iBeta;

	};

	struct MemImpedanceSpec {
		double C_total_ck;
		double C_total_cb;
		double C_total_rb;
		double C_total_wb;
		double C_total_dqs;

		double R_eq_ck;
		double R_eq_cb;
		double R_eq_rb;
		double R_eq_wb;
		double R_eq_dqs;
	};

	struct DataRateSpec {
		uint32_t commandBusRate;
		uint32_t dataBusRate;
		uint32_t dqsBusRate;
	};

	struct BankWiseParams
	{
		// Set of possible PASR modes
		enum pasrModes {
			PASR_0,
			PASR_1,
			PASR_2,
			PASR_3,
			PASR_4,
			PASR_5,
			PASR_6,
			PASR_7
		};

		// List of active banks under the specified PASR mode
		std::vector<uint64_t> activeBanks;
		// ACT Standby power factor
		double bwPowerFactRho;
		// Self-Refresh power factor
		uint64_t bwPowerFactSigma;
		// Whether PASR is enabled ( true : enabled )
		bool flgPASR;
		// PASR mode utilized (int 0-7)
		uint64_t pasrMode;
		// Whether bank is active in PASR
		bool isBankActiveInPasr(const unsigned bankIdx) const;

	};


	MemTimingSpec memTimingSpec;
	MemImpedanceSpec memImpedanceSpec;
	DataRateSpec dataRateSpec;
	std::vector<MemPowerSpec> memPowerSpec;
	BankWiseParams bwParams;

private:
    void parseImpedanceSpec(nlohmann::json &memspec);
};

}

#endif /* DRAMPOWER_MEMSPEC_MEMSPECLPDDR4_H */
