#include "MemSpecDDR5.h"
using namespace DRAMPower;
using json = nlohmann::json;


MemSpecDDR5::MemSpecDDR5(nlohmann::json &memspec)
        : MemSpec(memspec), memImpedanceSpec{}
{
    numberOfBankGroups = parseUint(memspec["memarchitecturespec"]["nbrOfBankGroups"],"nbrOfBankGroups");
    banksPerGroup = numberOfBanks / numberOfBankGroups;
    numberOfRanks          = parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks");

    refreshMode            = parseUintWithDefaut(memspec["RefreshMode"],"RefreshMode",1);
    memTimingSpec.fCKMHz   = (parseUdouble(memspec["memtimingspec"]["clkMhz"], "clkMhz"));
    memTimingSpec.tCK      = (1000.0 / memTimingSpec.fCKMHz); //clock period in mili seconds
    memTimingSpec.tRAS     = (parseUint(memspec["memtimingspec"]["RAS"], "RAS"));
    memTimingSpec.tRCD     = (parseUint(memspec["memtimingspec"]["RCD"], "RCD"));
    memTimingSpec.tRTP     = (parseUint(memspec["memtimingspec"]["RTP"], "RTP"));
    memTimingSpec.tWL      = (parseUint(memspec["memtimingspec"]["WL"], "WL"));
    memTimingSpec.tWR      = (parseUint(memspec["memtimingspec"]["WR"], "WR"));
    memTimingSpec.tRP      = (parseUint(memspec["memtimingspec"]["RP"], "RP"));
    memTimingSpec.tRFCsb   = (parseUint(memspec["memtimingspec"]["RFCsb"], "RFCsb"));

    auto VDD = VoltageDomain::VDD;
    auto VPP = VoltageDomain::VPP;
    auto VDDQ = VoltageDomain::VDDQ;

    memPowerSpec.push_back(MemPowerSpec());

    memPowerSpec[VDD].iXX0      = (parseUdouble(memspec["mempowerspec"]["idd0"], "idd0"));
    memPowerSpec[VDD].iXX2N     = (parseUdouble(memspec["mempowerspec"]["idd2n"], "idd2n"));
    memPowerSpec[VDD].iXX3N     = (parseUdouble(memspec["mempowerspec"]["idd3n"], "idd3n"));
    memPowerSpec[VDD].iXX4R     = (parseUdouble(memspec["mempowerspec"]["idd4r"], "idd4r"));
    memPowerSpec[VDD].iXX4W     = (parseUdouble(memspec["mempowerspec"]["idd4w"], "idd4w"));
    memPowerSpec[VDD].iXX5C      = (parseUdouble(memspec["mempowerspec"]["idd5C"], "idd5C"));
    memPowerSpec[VDD].iXX6N      = (parseUdouble(memspec["mempowerspec"]["idd6n"], "idd6n"));
    memPowerSpec[VDD].vXX       = (parseUdouble(memspec["mempowerspec"]["vdd"], "vdd"));
    memPowerSpec[VDD].iXX2P     = (parseUdouble(memspec["mempowerspec"]["idd2p"], "idd2p"));
    memPowerSpec[VDD].iXX3P     = (parseUdouble(memspec["mempowerspec"]["idd3p"], "idd3p"));

    memPowerSpec.push_back(MemPowerSpec());

    memPowerSpec[VPP].iXX0      = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp0"], "ipp0"));
    memPowerSpec[VPP].iXX2N     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp2n"], "ipp2n"));
    memPowerSpec[VPP].iXX3N     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp3n"], "ipp3n"));
    memPowerSpec[VPP].iXX4R     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp4r"], "ipp4r"));
    memPowerSpec[VPP].iXX4W     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp4w"], "ipp4w"));
    memPowerSpec[VPP].iXX5C      = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp5C"], "ipp5C"));
    memPowerSpec[VPP].iXX6N      = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp6n"], "ipp6n"));
    memPowerSpec[VPP].vXX       = (parseUdoubleWithDefault(memspec["mempowerspec"]["vpp"], "vpp"));
    memPowerSpec[VPP].iXX2P     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp2p"], "ipp2p"));
    memPowerSpec[VPP].iXX3P     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp3p"], "ipp3p"));

    // TODO: have different spec for vddq?
    memPowerSpec.push_back(MemPowerSpec());
    memPowerSpec[VDDQ] = memPowerSpec[VDD];

    if (refreshMode==1) {
        memPowerSpec[VDD].iXX5X      = (parseUdouble(memspec["mempowerspec"]["idd5B"], "idd5B"));
        memPowerSpec[VPP].iXX5X      = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp5B"], "ipp5B"));
        memTimingSpec.tRFC = (parseUint(memspec["memtimingspec"]["RFC1"], "RFC1"));
    }
    else {
        memPowerSpec[VDD].iXX5X      = (parseUdouble(memspec["mempowerspec"]["idd5F"], "idd5F"));
        memPowerSpec[VPP].iXX5X      = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp5F"], "ipp5F"));
        memTimingSpec.tRFC = (parseUint(memspec["memtimingspec"]["RFC2"], "RFC2"));
    }


    if(memspec["mempowerspec"].contains("iBeta")){
        memPowerSpec[VDD].iBeta = parseUdouble( memspec["mempowerspec"]["iBeta"],"iBeta");
        memPowerSpec[VPP].iBeta = parseUdouble( memspec["mempowerspec"]["iBeta"],"iBeta");
    }
    else{
        memPowerSpec[VDD].iBeta = memPowerSpec[VDD].iXX0;
        memPowerSpec[VPP].iBeta = memPowerSpec[VPP].iXX0;
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

    memTimingSpec.tBurst = burstLength/dataRate;
    prechargeOffsetRD      =  memTimingSpec.tRTP;
    prechargeOffsetWR      =  memTimingSpec.tBurst + memTimingSpec.tWL + memTimingSpec.tWR;

    parseImpedanceSpec(memspec);
    parseDataRateSpec(memspec);
}

// TODO: is this being used?
uint64_t MemSpecDDR5::timeToCompletion(DRAMPower::CmdType type)
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

void MemSpecDDR5::parseImpedanceSpec(nlohmann::json &memspec) {
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

void MemSpecDDR5::parseDataRateSpec(nlohmann::json &memspec) {
    if (!memspec.contains("dataratespec")) {
        dataRateSpec = {2, 2, 2};
        return;
    }

    dataRateSpec.commandBusRate = parseUint(memspec["dataratespec"]["ca_bus_rate"], "ca_bus_rate");
    dataRateSpec.dataBusRate = parseUint(memspec["dataratespec"]["dq_bus_rate"], "dq_bus_rate");
    dataRateSpec.dqsBusRate = parseUint(memspec["dataratespec"]["dqs_bus_rate"], "dqs_bus_rate");
}
