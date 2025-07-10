#ifndef DRAMPOWER_MEMSPEC_MEMSPECLPDDR5_H
#define DRAMPOWER_MEMSPEC_MEMSPECLPDDR5_H

#include "MemSpec.h"
#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>

namespace DRAMPower {

    class MemSpecLPDDR5 final : public MemSpec<DRAMUtils::MemSpec::MemSpecLPDDR5> {
    public:
        using MemImpedanceSpec = DRAMUtils::MemSpec::MemImpedanceSpecTypeLPDDR5;
        enum VoltageDomain {
            VDD1 = 0,
            VDD2H,
            VDD2L,
        };

        enum BankArchitectureMode {
            MBG,   // 4 banks, 4 bank groups
            M16B,  // 16 banks, no bank groups
            M8B    // 8 banks, no bank groups
        };

    public:
        MemSpecLPDDR5() = delete;

        MemSpecLPDDR5(const DRAMUtils::MemSpec::MemSpecLPDDR5 &memspec);
	
        MemSpecLPDDR5(json_t &data) = delete;
        MemSpecLPDDR5(const json_t &data) = delete;

        // Copy constructor and assignment operator
        MemSpecLPDDR5(const MemSpecLPDDR5&) = default;
        MemSpecLPDDR5& operator=(const MemSpecLPDDR5&) = default;

        // Move constructor and assignment operator
        MemSpecLPDDR5(MemSpecLPDDR5&&) = default;
        MemSpecLPDDR5& operator=(MemSpecLPDDR5&&) = default;

	    ~MemSpecLPDDR5() = default;

        uint64_t numberOfBankGroups;
        uint64_t banksPerGroup;
        uint64_t numberOfRanks;
        std::size_t perTwoBankOffset = 8;
        BankArchitectureMode bank_arch;
        bool wckAlwaysOnMode;

        double vddq;

        // Memspec Variables:
        struct MemTimingSpec
        {
            double fck;
            double tCK;
            double tWCK;
            uint64_t WCKtoCK;
            uint64_t tRAS;
            uint64_t tRCD;
            uint64_t tRL;
            uint64_t tWL;
            uint64_t tWR;
            uint64_t tRFC;
            uint64_t tRFCPB;
            uint64_t tREFI;
            uint64_t tRP;
            uint64_t tRBTP;
            uint64_t tBurst;
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
            double iDD6DSX;
            double iBeta;
        };

        struct BankWiseParams {
            // ACT Standby power factor
            double bwPowerFactRho;
        };

        MemTimingSpec memTimingSpec;
        MemImpedanceSpec memImpedanceSpec;
        std::vector<MemPowerSpec> memPowerSpec;
        BankWiseParams bwParams;

       private:
        void parseImpedanceSpec(const DRAMUtils::MemSpec::MemSpecLPDDR5 &memspec);

       public:
        static MemSpecLPDDR5 from_memspec(const DRAMUtils::MemSpec::MemSpecVariant&);
    };

}  // namespace DRAMPower

#endif /* DRAMPOWER_MEMSPEC_MEMSPECLPDDR5_H */
