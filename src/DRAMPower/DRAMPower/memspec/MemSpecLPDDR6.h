#ifndef DRAMPOWER_MEMSPEC_MEMSPECLPDDR6_H
#define DRAMPOWER_MEMSPEC_MEMSPECLPDDR6_H

#include "MemSpec.h"
#include <DRAMUtils/memspec/standards/MemSpecLPDDR6.h>

namespace DRAMPower {

    class MemSpecLPDDR6 final : public MemSpec<DRAMUtils::MemSpec::MemSpecLPDDR6> {
    public:
        using MemImpedanceSpec = DRAMUtils::MemSpec::MemImpedanceSpecTypeLPDDR6;
        enum VoltageDomain {
            VDD1 = 0,
            VDD2C,
            VDD2D,
        };

    public:
        MemSpecLPDDR6() = delete;

        MemSpecLPDDR6(const DRAMUtils::MemSpec::MemSpecLPDDR6 &memspec);
	
        MemSpecLPDDR6(json_t &data) = delete;
        MemSpecLPDDR6(const json_t &data) = delete;

        // Copy constructor and assignment operator
        MemSpecLPDDR6(const MemSpecLPDDR6&) = default;
        MemSpecLPDDR6& operator=(const MemSpecLPDDR6&) = default;

        // Move constructor and assignment operator
        MemSpecLPDDR6(MemSpecLPDDR6&&) = default;
        MemSpecLPDDR6& operator=(MemSpecLPDDR6&&) = default;

	    ~MemSpecLPDDR6() = default;

        uint64_t numberOfBankGroups;
        uint64_t banksPerGroup;
        uint64_t numberOfRanks;
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
            uint64_t tRCDR;
            uint64_t tRCDW;
            uint64_t tRL;
            uint64_t tWL;
            uint64_t tWR;
            uint64_t tRFCDB;
            uint64_t tRFCAB;
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
            double iDD5PDBX;
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
        void parseImpedanceSpec(const DRAMUtils::MemSpec::MemSpecLPDDR6 &memspec);

       public:
        static MemSpecLPDDR6 from_memspec(const DRAMUtils::MemSpec::MemSpecVariant&);
    };

}  // namespace DRAMPower

#endif /* DRAMPOWER_MEMSPEC_MEMSPECLPDDR6_H */
