#ifndef DRAMPOWER_MEMSPEC_MEMSPECLPDDR5_H
#define DRAMPOWER_MEMSPEC_MEMSPECLPDDR5_H

#include "MemSpec.h"


namespace DRAMPower {

    class MemSpecLPDDR5 final : public MemSpec {
    public:
        enum VoltageDomain {
            VDD = 0,
            VDDQ = 1
        };
    public:
        MemSpecLPDDR5() = default;

        MemSpecLPDDR5(nlohmann::json &memspec);

        ~MemSpecLPDDR5() = default;

        uint64_t timeToCompletion(CmdType type) override;


        unsigned numberOfBankGroups;
        unsigned banksPerGroup;
        unsigned numberOfRanks;
        std::size_t perTwoBankOffset = 8;
        bool BGroupMode;



        // Memspec Variables:
        struct MemTimingSpec
        {
            double fCKMHz;
            double tCK;
            double tWCK;
            uint64_t WCKtoCK;
            uint64_t tRAS;
            uint64_t tRCD;
            uint64_t tRL;
            uint64_t tRTP;
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
        std::vector<MemPowerSpec> memPowerSpec;
        BankWiseParams bwParams;
    };

}


#endif /* DRAMPOWER_MEMSPEC_MEMSPECLPDDR5_H */
