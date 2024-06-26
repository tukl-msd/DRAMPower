#include "MemSpecLPDDR4.h"
using namespace DRAMPower;
using json = nlohmann::json;


MemSpecLPDDR4::MemSpecLPDDR4(nlohmann::json &memspec)
    : MemSpec(memspec), memImpedanceSpec{}
{
    numberOfBankGroups = parseUint(memspec["memarchitecturespec"]["nbrOfBankGroups"],"nbrOfBankGroups");
    banksPerGroup = numberOfBanks / numberOfBankGroups;
    numberOfRanks          = parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks");

    memTimingSpec.fck   = (parseUdouble(memspec["memtimingspec"]["clk"], "clk"));
    memTimingSpec.tCK      = (1.0 / memTimingSpec.fck); //clock period in seconds
    memTimingSpec.tRAS     = (parseUint(memspec["memtimingspec"]["RAS"], "RAS"));
    memTimingSpec.tRCD     = (parseUint(memspec["memtimingspec"]["RCD"], "RCD"));
    memTimingSpec.tRL      = (parseUint(memspec["memtimingspec"]["RL"], "RL"));
    memTimingSpec.tRTP     = (parseUint(memspec["memtimingspec"]["RTP"], "RTP"));
    memTimingSpec.tWL      = (parseUint(memspec["memtimingspec"]["WL"], "WL"));
    memTimingSpec.tWR      = (parseUint(memspec["memtimingspec"]["WR"], "WR"));
    memTimingSpec.tRP      = (parseUint(memspec["memtimingspec"]["RP"], "RP"));
    memTimingSpec.tRFCPB   = (parseUint(memspec["memtimingspec"]["RFCpb"], "RFCpb"));
    memTimingSpec.tRFC = (parseUint(memspec["memtimingspec"]["RFCab"], "RFCab"));
    memTimingSpec.tREFI    = (parseUint(memspec["memtimingspec"]["REFI"], "REFI"));

    auto VDD = VoltageDomain::VDD;

    memPowerSpec.push_back(MemPowerSpec());

    memPowerSpec[VDD].iDD0X      = (parseUdouble(memspec["mempowerspec"]["idd0"], "idd0"));
    memPowerSpec[VDD].iDD2NX     = (parseUdouble(memspec["mempowerspec"]["idd2n"], "idd2n"));
    memPowerSpec[VDD].iDD3NX     = (parseUdouble(memspec["mempowerspec"]["idd3n"], "idd3n"));
    memPowerSpec[VDD].iDD4RX     = (parseUdouble(memspec["mempowerspec"]["idd4r"], "idd4r"));
    memPowerSpec[VDD].iDD4WX     = (parseUdouble(memspec["mempowerspec"]["idd4w"], "idd4w"));
    memPowerSpec[VDD].iDD5X      = (parseUdouble(memspec["mempowerspec"]["idd5"], "idd5"));
    memPowerSpec[VDD].iDD5PBX      = (parseUdouble(memspec["mempowerspec"]["idd5pb"], "idd5pb"));
    memPowerSpec[VDD].iDD6X      = (parseUdouble(memspec["mempowerspec"]["idd6"], "idd6"));
    memPowerSpec[VDD].vDDX       = (parseUdouble(memspec["mempowerspec"]["vdd"], "vdd"));
    memPowerSpec[VDD].iDD2PX     = (parseUdouble(memspec["mempowerspec"]["idd2p"], "idd2p"));
    memPowerSpec[VDD].iDD3PX     = (parseUdouble(memspec["mempowerspec"]["idd3p"], "idd3p"));

    if(memspec["mempowerspec"].contains("iBeta")){
        memPowerSpec[VDD].iBeta = parseUdouble( memspec["mempowerspec"]["iBeta"],"iBeta");
    }
    else{
        memPowerSpec[VDD].iBeta = memPowerSpec[VDD].iDD0X;
    }

    if (memspec.contains("bankwisespec")) {
        if (memspec["bankwisespec"].contains("factRho"))
            bwParams.bwPowerFactRho = parseUdouble(memspec["bankwisespec"]["factRho"],"factRho");
        else
            bwParams.bwPowerFactRho = 1;

        if (memspec["bankwisespec"].contains("factSigma"))
            bwParams.bwPowerFactSigma = parseUdouble(memspec["bankwisespec"]["factSigma"],"factSigma");
        else
            bwParams.bwPowerFactSigma = 1;

        bwParams.flgPASR = parseBoolWithDefault(memspec["bankwisespec"]["hasPASR"],"hasPASR");
        if (bwParams.flgPASR) {

            bwParams.pasrMode = parseUint(memspec["bankwisespec"]["pasrMode"],"pasrMode");
            ///////////////////////////////////////////////////////////
            // Activate banks for self refresh based on the PASR mode
            // ACTIVE     - X
            // NOT ACTIVE - 0
            ///////////////////////////////////////////////////////////
            switch(bwParams.pasrMode) {

            case(BankWiseParams::pasrModes::PASR_0): {
                // PASR MODE 0
                // FULL ARRAY
                // |X X X X |
                // |X X X X |
                bwParams.activeBanks.resize(numberOfBanks);
                std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 0);
                break;
            }
            case(BankWiseParams::pasrModes::PASR_1): {
                // PASR MODE 1
                // (1/2) ARRAY
                // |X X X X |
                // |0 0 0 0 |
                bwParams.activeBanks.resize(numberOfBanks - 4);
                std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 0);
                break;
            }
            case(BankWiseParams::pasrModes::PASR_2): {
                // PASR MODE 2
                // (1/4) ARRAY
                // |X X 0 0 |
                // |0 0 0 0 |
                bwParams.activeBanks.resize(numberOfBanks - 6);
                std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 0);
                break;
            }
            case(BankWiseParams::pasrModes::PASR_3): {
                // PASR MODE 3
                // (1/8) ARRAY
                // |X 0 0 0 |
                // |0 0 0 0 |
                bwParams.activeBanks.resize(numberOfBanks - 7);
                std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 0);
                break;
            }
            case(BankWiseParams::pasrModes::PASR_4): {
                // PASR MODE 4
                // (3/4) ARRAY
                // |0 0 X X |
                // |X X X X |
                bwParams.activeBanks.resize(numberOfBanks - 2);
                std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 2);
                break;
            }
            case(BankWiseParams::pasrModes::PASR_5): {
                // PASR MODE 5
                // (1/2) ARRAY
                // |0 0 0 0 |
                // |X X X X |
                bwParams.activeBanks.resize(numberOfBanks - 4);
                std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 4);
                break;
            }
            case(BankWiseParams::pasrModes::PASR_6): {
                // PASR MODE 6
                // (1/4) ARRAY
                // |0 0 0 0 |
                // |0 0 X X |
                bwParams.activeBanks.resize(numberOfBanks - 6);
                std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 6);
                break;
            }
            case(BankWiseParams::pasrModes::PASR_7): {
                // PASR MODE 7
                // (1/8) ARRAY
                // |0 0 0 0 |
                // |0 0 0 X |
                bwParams.activeBanks.resize(numberOfBanks - 7);
                std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 7);
                break;
            }
            default: {
                // PASR MODE 0
                // FULL ARRAY
                // |X X X X |
                // |X X X X |
                bwParams.activeBanks.resize(numberOfBanks);
                std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 0);
                break;
            }
            } //end switch
        } // end IF flgPASR
    } // end if !bankwise.empty()
    else {
        bwParams.bwPowerFactRho = 1;
        bwParams.bwPowerFactSigma = 1;
        bwParams.flgPASR = false;
    }

    //Source: JESD209-4 (LPDDR4); Table 21
    if(burstLength == 16){
        prechargeOffsetRD      = memTimingSpec.tRTP;
    }else{
        prechargeOffsetRD      = 8 + memTimingSpec.tRTP;
    }
    prechargeOffsetWR      =  memTimingSpec.tWL + burstLength/2 + memTimingSpec.tWR + 1;

}

bool MemSpecLPDDR4::BankWiseParams::isBankActiveInPasr(const unsigned bankIdx) const
{
    return (std::find(activeBanks.begin(), activeBanks.end(), bankIdx)
            != activeBanks.end());
}

uint64_t MemSpecLPDDR4::timeToCompletion(DRAMPower::CmdType type)
{
    uint64_t offset = 0;

    if (type == DRAMPower::CmdType::ACT)
        offset = memTimingSpec.tRCD;
    else if (type == DRAMPower::CmdType::RD)
        offset = memTimingSpec.tRL + ((burstLength)/(dataRate));
    else if (type == DRAMPower::CmdType::WR)
        offset = memTimingSpec.tWL + ((burstLength)/(dataRate));
    else if (type == CmdType::REFA)
        offset = memTimingSpec.tRFC;
    else if (type == CmdType::PRE || type == CmdType::PREA)
        return memTimingSpec.tRP;
    //else
        //PRINTDEBUGMESSAGE("timeToCompletion not available for given Command Type", 0, type, 0);
    return offset;
} // MemSpecLPDDR4::timeToCompletion

void MemSpecLPDDR4::parseImpedanceSpec(nlohmann::json &memspec) {
    if (!memspec.contains("memimpedancespec")) {
        // Leaving it to default-initialize to 0 would break static power (div by 0)
        memImpedanceSpec = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
        return;
    }

    memImpedanceSpec.C_total_cb =
        parseUdouble(memspec["memimpedancespec"]["C_total_cb"], "C_total_cb");
    memImpedanceSpec.C_total_ck =
        parseUdouble(memspec["memimpedancespec"]["C_total_ck"], "C_total_ck");
    memImpedanceSpec.C_total_dqs =
        parseUdouble(memspec["memimpedancespec"]["C_total_dqs"], "C_total_dqs");
    memImpedanceSpec.C_total_rb =
        parseUdouble(memspec["memimpedancespec"]["C_total_rb"], "C_total_rb");
    memImpedanceSpec.C_total_wb =
        parseUdouble(memspec["memimpedancespec"]["C_total_wb"], "C_total_wb");

    memImpedanceSpec.R_eq_cb = parseUdouble(memspec["memimpedancespec"]["R_eq_cb"], "R_eq_cb");
    memImpedanceSpec.R_eq_ck = parseUdouble(memspec["memimpedancespec"]["R_eq_ck"], "R_eq_ck");
    memImpedanceSpec.R_eq_dqs = parseUdouble(memspec["memimpedancespec"]["R_eq_dqs"], "R_eq_dqs");
    memImpedanceSpec.R_eq_rb = parseUdouble(memspec["memimpedancespec"]["R_eq_rb"], "R_eq_rb");
    memImpedanceSpec.R_eq_wb = parseUdouble(memspec["memimpedancespec"]["R_eq_wb"], "R_eq_wb");
}
