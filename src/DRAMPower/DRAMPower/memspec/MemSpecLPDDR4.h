#ifndef DRAMPOWER_MEMSPEC_MEMSPECLPDDR4_H
#define DRAMPOWER_MEMSPEC_MEMSPECLPDDR4_H

#include "MemSpec.h"

#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>


namespace DRAMPower {

class MemSpecLPDDR4 final : public MemSpec<DRAMUtils::MemSpec::MemSpecLPDDR4>
{
public:

    enum VoltageDomain {
        VDD1 = 0,
        VDD2 = 1,
    };

public:
	MemSpecLPDDR4() = delete;
	
	MemSpecLPDDR4(const DRAMUtils::MemSpec::MemSpecLPDDR4 &memspec);
	
	MemSpecLPDDR4(json_t &data) = delete;
	MemSpecLPDDR4(const json_t &data) = delete;

    // Copy constructor
    MemSpecLPDDR4(const MemSpecLPDDR4&) = default;

    // Move constructor
    MemSpecLPDDR4(MemSpecLPDDR4&&) = default;

    // Move assignment operator
    MemSpecLPDDR4& operator=(MemSpecLPDDR4&&) = default;

	~MemSpecLPDDR4() = default;
	uint64_t timeToCompletion(CmdType type) override;

    uint64_t numberOfBankGroups;
    uint64_t banksPerGroup;
    uint64_t numberOfRanks;

	double vddq;

	struct MemTimingSpec
	{
		double fck;
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
		double bwPowerFactSigma;
		// Whether PASR is enabled ( true : enabled )
		bool flgPASR;
		// PASR mode utilized (int 0-7)
		uint64_t pasrMode;
		// Whether bank is active in PASR
		bool isBankActiveInPasr(const unsigned bankIdx) const;

	};

	MemTimingSpec memTimingSpec;
	MemImpedanceSpec memImpedanceSpec;
	std::vector<MemPowerSpec> memPowerSpec;
	BankWiseParams bwParams;

private:
    void parseImpedanceSpec(const DRAMUtils::MemSpec::MemSpecLPDDR4 &memspec);

public:
    static MemSpecLPDDR4 from_memspec(const DRAMUtils::MemSpec::MemSpecVariant&);
};

}

#endif /* DRAMPOWER_MEMSPEC_MEMSPECLPDDR4_H */
