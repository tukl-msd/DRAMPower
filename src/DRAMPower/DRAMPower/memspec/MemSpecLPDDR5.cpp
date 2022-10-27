#include "MemSpecLPDDR5.h"


using namespace DRAMPower;
using json = nlohmann::json;


MemSpecLPDDR5::MemSpecLPDDR5(nlohmann::json &memspec)
        : MemSpec(memspec)
{
    numberOfBankGroups = parseUint(memspec["memarchitecturespec"]["nbrOfBankGroups"],"nbrOfBankGroups");
    banksPerGroup = numberOfBanks / numberOfBankGroups;
    numberOfRanks          = parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks");

    memTimingSpec.fCKMHz   = (parseUdouble(memspec["memtimingspec"]["clkMhz"], "clkMhz"));
    memTimingSpec.WCKtoCK  = (parseUint(memspec["memtimingspec"]["WCKtoCK"], "WCKtoCK"));
    memTimingSpec.tCK      = (1000.0 / memTimingSpec.fCKMHz);               //clock period in mili seconds
    memTimingSpec.tWCK      = memTimingSpec.tCK / memTimingSpec.WCKtoCK;   //write clock period in mili seconds
    memTimingSpec.tRAS     = (parseUint(memspec["memtimingspec"]["RAS"], "RAS"));
    memTimingSpec.tRCD     = (parseUint(memspec["memtimingspec"]["RCD"], "RCD"));
    memTimingSpec.tRTP     = (parseUint(memspec["memtimingspec"]["RTP"], "RTP"));
    memTimingSpec.tWL      = (parseUint(memspec["memtimingspec"]["WL"], "WL"));
    memTimingSpec.tWR      = (parseUint(memspec["memtimingspec"]["WR"], "WR"));
    memTimingSpec.tRP      = (parseUint(memspec["memtimingspec"]["RP"], "RP"));
    memTimingSpec.tRFCPB   = (parseUint(memspec["memtimingspec"]["RFCpb"], "RFCpb"));
    memTimingSpec.tRFC = (parseUint(memspec["memtimingspec"]["RFCab"], "RFCab"));
    memTimingSpec.tREFI = (parseUint(memspec["memtimingspec"]["REFI"], "REFI"));
    memTimingSpec.tRBTP = (parseUint(memspec["memtimingspec"]["RBTP"], "RBTP"));

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
    memPowerSpec[VDD].iDD6DSX      = (parseUdouble(memspec["mempowerspec"]["idd6ds"], "idd6ds"));
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
    }
    else {
        bwParams.bwPowerFactRho = 1;
    }


    BGroupMode = (numberOfBankGroups > 1);

    // Source: LPDDR5 standard; table 312
    memTimingSpec.tBurst = burstLength/(dataRate * memTimingSpec.WCKtoCK);

    // Source: LPDDR5 standard; figure 96
    prechargeOffsetRD      =   memTimingSpec.tBurst + memTimingSpec.tRBTP;

    // Source: LPDDR5 standard; figure 97
    prechargeOffsetWR      =  memTimingSpec.tWL + memTimingSpec.tBurst + 1 + memTimingSpec.tWR;
}

// TODO: is this being used?
uint64_t MemSpecLPDDR5::timeToCompletion(DRAMPower::CmdType type)
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

    return offset;
}




