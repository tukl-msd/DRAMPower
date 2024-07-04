#ifndef DRAMPOWER_MEMSPEC_MEMSPECDDR5_H
#define DRAMPOWER_MEMSPEC_MEMSPECDDR5_H

#include "MemSpec.h"
#include <DRAMUtils/memspec/standards/MemSpecDDR5.h>


namespace DRAMPower {

    class MemSpecDDR5 final : public MemSpec<DRAMUtils::Config::MemSpecDDR5> {
    public:

        enum VoltageDomain {
            VDD = 0,
            VPP = 1,
        };

    public:
        MemSpecDDR5() = delete;

        MemSpecDDR5(const DRAMUtils::Config::MemSpecDDR5 &memspec);

        MemSpecDDR5(json &data) = delete;
        MemSpecDDR5(const json &data) = delete;

        ~MemSpecDDR5() = default;

        uint64_t timeToCompletion(CmdType type) override;


        unsigned numberOfBankGroups;
        unsigned banksPerGroup;
        unsigned numberOfRanks;

        double vddq;

        // Memspec Variables:
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

        struct MemImpedanceSpec
        {
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
            // ACT Standby power factor
            double bwPowerFactRho;
        };

        MemTimingSpec memTimingSpec;
        MemImpedanceSpec memImpedanceSpec;
        DataRateSpec dataRateSpec;
        std::vector<MemPowerSpec> memPowerSpec;
        BankWiseParams bwParams;

    private:
        void parseImpedanceSpec(const DRAMUtils::Config::MemSpecDDR5 &memspec);
        void parseDataRateSpec(const DRAMUtils::Config::MemSpecDDR5 &memspec);
    };

}

#endif /* DRAMPOWER_MEMSPEC_MEMSPECDDR5_H */
