#ifndef DRAMPOWER_MEMSPEC_MEMSPECLPDDR5_H
#define DRAMPOWER_MEMSPEC_MEMSPECLPDDR5_H

#include "MemSpec.h"
#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>

namespace DRAMPower {

    class MemSpecLPDDR5 final : public MemSpec<DRAMUtils::MemSpec::MemSpecLPDDR5> {
    public:

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

        // Copy constructor
        MemSpecLPDDR5(const MemSpecLPDDR5&) = default;

        // Move constructor
        MemSpecLPDDR5(MemSpecLPDDR5&&) = default;

        // Move assignment operator
        MemSpecLPDDR5& operator=(MemSpecLPDDR5&&) = default;

	    ~MemSpecLPDDR5() = default;

        uint64_t timeToCompletion(CmdType type) override;


        unsigned numberOfBankGroups;
        unsigned banksPerGroup;
        unsigned numberOfRanks;
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

        struct MemImpedanceSpec {
            double C_total_ck;
            double C_total_wck;
            double C_total_cb;
            double C_total_rb;
            double C_total_wb;
            double C_total_dqs;

            double R_eq_ck;
            double R_eq_wck;
            double R_eq_cb;
            double R_eq_rb;
            double R_eq_wb;
            double R_eq_dqs;
        };

        struct BankWiseParams {
            // ACT Standby power factor
            double bwPowerFactRho;
        };

        MemTimingSpec memTimingSpec;
        MemImpedanceSpec memImpedanceSpec;

class VoltageDomains {
public:
    using VDD1 = ContainerIndex<0>;
    using VDD2H = ContainerIndex<1>;
    using VDD2L = ContainerIndex<2>;
};
        IndexedContainer<MemPowerSpec, DRAMUtils::util::type_sequence<VoltageDomains::VDD1, VoltageDomains::VDD2H, VoltageDomains::VDD2L>> memPowerSpec;
        
        BankWiseParams bwParams;

       private:
        void parseImpedanceSpec(const DRAMUtils::MemSpec::MemSpecLPDDR5 &memspec);

       public:
        static MemSpecLPDDR5 from_memspec(const DRAMUtils::MemSpec::MemSpecVariant&);
    };

}  // namespace DRAMPower

#endif /* DRAMPOWER_MEMSPEC_MEMSPECLPDDR5_H */
