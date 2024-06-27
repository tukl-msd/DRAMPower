#include "MemSpecLPDDR4.h"
using namespace DRAMPower;
using json = nlohmann::json;


MemSpecLPDDR4::MemSpecLPDDR4(const DRAMUtils::Config::MemSpecLPDDR4 &memspec)
    : MemSpec(memspec), memImpedanceSpec{}
{
    numberOfBankGroups     = memspec.memarchitecturespec.nbrOfBankGroups;
    numberOfRanks          = memspec.memarchitecturespec.nbrOfRanks;
    banksPerGroup = numberOfBanks / numberOfBankGroups;

    memTimingSpec.tCK      = 1000/1300.0;//memspec.memtimingspec.tCK;
    memTimingSpec.tRAS     = memspec.memtimingspec.RAS;
    memTimingSpec.tRCD     = memspec.memtimingspec.RCD;
    memTimingSpec.tRL      = memspec.memtimingspec.RL;
    memTimingSpec.tRTP     = memspec.memtimingspec.RTP;
    memTimingSpec.tWL      = memspec.memtimingspec.WL;
    memTimingSpec.tWR      = memspec.memtimingspec.WR;
    memTimingSpec.tRP      = memspec.memtimingspec.RPpb; // TODO
    memTimingSpec.tRFCPB   = memspec.memtimingspec.RFCpb; // TODO
    memTimingSpec.tRFC     = memspec.memtimingspec.RFCab; // TODO
    memTimingSpec.tREFI    = memspec.memtimingspec.REFI; // TODO

    auto VDD1 = VoltageDomain::VDD1;
    auto VDD2 = VoltageDomain::VDD2;

    memPowerSpec.push_back(MemPowerSpec());

    memPowerSpec[VDD1].vDDX       = memspec.mempowerspec.vdd1;
    memPowerSpec[VDD1].iDD0X      = memspec.mempowerspec.idd01;
    memPowerSpec[VDD1].iDD2NX     = memspec.mempowerspec.idd2n1;
    memPowerSpec[VDD1].iDD3NX     = memspec.mempowerspec.idd3n1;
    memPowerSpec[VDD1].iDD4RX     = memspec.mempowerspec.idd4r1;
    memPowerSpec[VDD1].iDD4WX     = memspec.mempowerspec.idd4w1;
    memPowerSpec[VDD1].iDD5X      = memspec.mempowerspec.idd51;
    memPowerSpec[VDD1].iDD5PBX    = memspec.mempowerspec.idd5pb1;
    memPowerSpec[VDD1].iDD6X     = memspec.mempowerspec.idd61;
    memPowerSpec[VDD1].iDD2PX     = memspec.mempowerspec.idd2p1;
    memPowerSpec[VDD1].iDD3PX     = memspec.mempowerspec.idd3p1;

    memPowerSpec[VDD2].vDDX       = memspec.mempowerspec.vdd2;
    memPowerSpec[VDD2].iDD0X      = memspec.mempowerspec.idd02;
    memPowerSpec[VDD2].iDD2NX     = memspec.mempowerspec.idd2n2;
    memPowerSpec[VDD2].iDD3NX     = memspec.mempowerspec.idd3n2;
    memPowerSpec[VDD2].iDD4RX     = memspec.mempowerspec.idd4r2;
    memPowerSpec[VDD2].iDD4WX     = memspec.mempowerspec.idd4w2;
    memPowerSpec[VDD2].iDD5X      = memspec.mempowerspec.idd52;
    memPowerSpec[VDD2].iDD5PBX    = memspec.mempowerspec.idd5pb2;
    memPowerSpec[VDD2].iDD6X     = memspec.mempowerspec.idd62;
    memPowerSpec[VDD2].iDD2PX     = memspec.mempowerspec.idd2p2;
    memPowerSpec[VDD2].iDD3PX     = memspec.mempowerspec.idd3p2;

    vddq = memspec.mempowerspec.vddq;

    memPowerSpec[VDD1].iBeta = memspec.mempowerspec.iBeta_vdd1.value_or(memspec.mempowerspec.idd01);
    memPowerSpec[VDD2].iBeta = memspec.mempowerspec.iBeta_vdd2.value_or(memspec.mempowerspec.idd02);


    if(memspec.bankwisespec.has_value())
    {
        bwParams.bwPowerFactRho = memspec.bankwisespec.value().factRho.value_or(1);
        bwParams.bwPowerFactSigma = memspec.bankwisespec.value().factSigma.value_or(1);
        bwParams.flgPASR = memspec.bankwisespec.value().hasPASR.value_or(false);
        if (memspec.bankwisespec.value().pasrMode.has_value())
        {
           if(memspec.bankwisespec.value().pasrMode.value() == DRAMUtils::Config::pasrModesType::Invalid)
           {
                // pasrMode invalid
                bwParams.pasrMode = 0;
           }
           else
           {
                // pasrMode valid
                bwParams.pasrMode = static_cast<uint64_t>(memspec.bankwisespec.value().pasrMode.value());
           }
        }
        else
        {
            bwParams.pasrMode = 0;
        }
    }
    else
    {
        bwParams.bwPowerFactRho = 1;
        bwParams.bwPowerFactSigma = 1;
        bwParams.flgPASR = false;
        bwParams.pasrMode = 0;
    }

    if (bwParams.flgPASR) {

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

void MemSpecLPDDR4::parseImpedanceSpec(const DRAMUtils::Config::MemSpecLPDDR4 &memspec) {
    memspec.memimpedancespec.C_total_cb;

    memImpedanceSpec.C_total_cb = memspec.memimpedancespec.C_total_cb;
    memImpedanceSpec.C_total_ck = memspec.memimpedancespec.C_total_ck;
    memImpedanceSpec.C_total_dqs = memspec.memimpedancespec.C_total_dqs;
    memImpedanceSpec.C_total_rb = memspec.memimpedancespec.C_total_rb;
    memImpedanceSpec.C_total_wb = memspec.memimpedancespec.C_total_wb;

    memImpedanceSpec.R_eq_cb = memspec.memimpedancespec.R_eq_cb;
    memImpedanceSpec.R_eq_ck = memspec.memimpedancespec.R_eq_ck;
    memImpedanceSpec.R_eq_dqs = memspec.memimpedancespec.R_eq_dqs;
    memImpedanceSpec.R_eq_rb = memspec.memimpedancespec.R_eq_rb;
    memImpedanceSpec.R_eq_wb = memspec.memimpedancespec.R_eq_wb;
}
