#ifndef DRAMPOWER_MEMSPEC_MEMSPECDDR5_H
#define DRAMPOWER_MEMSPEC_MEMSPECDDR5_H

#include "MemSpec.h"


namespace DRAMPower {

    class MemSpecDDR5 final : public MemSpec {
    public:

        enum VoltageDomain {
            VDD = 0,
            VPP = 1,
            VDDQ = 2
        };

    public:
        MemSpecDDR5() = default;

        MemSpecDDR5(nlohmann::json &memspec);

        ~MemSpecDDR5() = default;

        uint64_t timeToCompletion(CmdType type) override;


        unsigned numberOfBankGroups;
        unsigned banksPerGroup;
        unsigned numberOfRanks;

        // Memspec Variables:
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
            uint64_t tRFC;
            uint64_t tRFCsb;
            uint64_t tRP;
            uint64_t tBurst;
        };

        // Currents and Voltages:
        struct MemPowerSpec
        {
            double vXX;
            double iXX0;
            double iXX2N;
            double iXX3N;
            double iXX2P;
            double iXX3P;
            double iXX4R;
            double iXX4W;
            double iXX5X;
            double iXX5C;
            double iXX6N;
            double iBeta;
        };


        struct BankWiseParams
        {
            // ACT Standby power factor
            double bwPowerFactRho;
        };

        uint64_t refreshMode;
        MemTimingSpec memTimingSpec;
        std::vector<MemPowerSpec> memPowerSpec;
        BankWiseParams bwParams;
    };

}

#endif /* DRAMPOWER_MEMSPEC_MEMSPECDDR5_H */
