#include "DRAMPower/memspec/MemSpecLPDDR5.h"

using namespace DRAMPower;
using json = nlohmann::json;

MemSpecLPDDR5::MemSpecLPDDR5(nlohmann::json &memspec)
        : MemSpec(memspec)
{
    numberOfBankGroups = parseUint(memspec["memarchitecturespec"]["nbrOfBankGroups"],"nbrOfBankGroups");
    banksPerGroup = numberOfBanks / numberOfBankGroups;
    numberOfRanks          = parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks");

    memTimingSpec.fCKMHz   = (parseUdouble(memspec["memtimingspec"]["clkMhz"], "clkMhz"));
    memTimingSpec.WCKtoCK  = (parseUintWithDefaut(memspec["memtimingspec"]["WCKtoCK"], "WCKtoCK", 2));
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
    auto VDDQ = VoltageDomain::VDDQ;

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

    memPowerSpec.push_back(MemPowerSpec());

    memPowerSpec[VDDQ].vDDX = parseUdouble(memspec["mempowerspec"]["vddq"], "vddq");


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

    wckAlwaysOnMode = parseBoolWithDefault(memspec["memarchitecturespec"]["WCKalwaysOn"], "WCKalwaysOn", true);

    parseImpedanceSpec(memspec);
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

void MemSpecLPDDR5::parseImpedanceSpec(nlohmann::json &memspec) {
    if (!memspec.contains("memimpedancespec")) {
        // Leaving it to default-initialize to 0 would break static power (div by 0)
        memImpedanceSpec = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
        return;
    }

    memImpedanceSpec.C_total_cb =
        parseUdouble(memspec["memimpedancespec"]["C_total_cb"], "C_total_cb");
    memImpedanceSpec.C_total_ck =
        parseUdouble(memspec["memimpedancespec"]["C_total_ck"], "C_total_ck");
    memImpedanceSpec.C_total_wck =
        parseUdouble(memspec["memimpedancespec"]["C_total_wck"], "C_total_wck");
    memImpedanceSpec.C_total_dqs =
        parseUdouble(memspec["memimpedancespec"]["C_total_dqs"], "C_total_dqs");
    memImpedanceSpec.C_total_rb =
        parseUdouble(memspec["memimpedancespec"]["C_total_rb"], "C_total_rb");
    memImpedanceSpec.C_total_wb =
        parseUdouble(memspec["memimpedancespec"]["C_total_wb"], "C_total_wb");

    memImpedanceSpec.R_eq_cb = parseUdouble(memspec["memimpedancespec"]["R_eq_cb"], "R_eq_cb");
    memImpedanceSpec.R_eq_ck = parseUdouble(memspec["memimpedancespec"]["R_eq_ck"], "R_eq_ck");
    memImpedanceSpec.R_eq_wck = parseUdouble(memspec["memimpedancespec"]["R_eq_wck"], "R_eq_wck");
    memImpedanceSpec.R_eq_dqs = parseUdouble(memspec["memimpedancespec"]["R_eq_dqs"], "R_eq_dqs");
    memImpedanceSpec.R_eq_rb = parseUdouble(memspec["memimpedancespec"]["R_eq_rb"], "R_eq_rb");
    memImpedanceSpec.R_eq_wb = parseUdouble(memspec["memimpedancespec"]["R_eq_wb"], "R_eq_wb");
}
