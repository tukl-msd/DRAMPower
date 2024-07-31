#include "MemSpecDDR5.h"
using namespace DRAMPower;
using json_t = nlohmann::json;


MemSpecDDR5::MemSpecDDR5(const DRAMUtils::MemSpec::MemSpecDDR5 &memspec)
        : MemSpec(memspec)
{
    numberOfBankGroups      = memspec.memarchitecturespec.nbrOfBankGroups;
    numberOfRanks           = memspec.memarchitecturespec.nbrOfRanks;
    banksPerGroup           = numberOfBanks / numberOfBankGroups;

    memTimingSpec.tCK       = memspec.memtimingspec.tCK;
    memTimingSpec.tRAS      = memspec.memtimingspec.RAS;
    memTimingSpec.tRCD      = memspec.memtimingspec.RCD;
    memTimingSpec.tRTP      = memspec.memtimingspec.RTP;
    memTimingSpec.tWL       = memspec.memtimingspec.WL;
    memTimingSpec.tWR       = memspec.memtimingspec.WR;
    memTimingSpec.tRP       = memspec.memtimingspec.RP;
    memTimingSpec.tRFCsb    = memspec.memtimingspec.RFCsb_slr;

    auto VDD = VoltageDomains::VDD();
    auto VPP = VoltageDomains::VPP();

    memPowerSpec[VDD].vXX       = memspec.mempowerspec.vdd;
    memPowerSpec[VDD].iXX0      = memspec.mempowerspec.idd0;
    memPowerSpec[VDD].iXX2N     = memspec.mempowerspec.idd2n;
    memPowerSpec[VDD].iXX3N     = memspec.mempowerspec.idd3n;
    memPowerSpec[VDD].iXX4R     = memspec.mempowerspec.idd4r;
    memPowerSpec[VDD].iXX4W     = memspec.mempowerspec.idd4w;
    memPowerSpec[VDD].iXX5C     = memspec.mempowerspec.idd5c;
    memPowerSpec[VDD].iXX6N     = memspec.mempowerspec.idd6n;
    memPowerSpec[VDD].iXX2P     = memspec.mempowerspec.idd2p;
    memPowerSpec[VDD].iXX3P     = memspec.mempowerspec.idd3p;

    memPowerSpec[VPP].vXX       = memspec.mempowerspec.vpp;
    memPowerSpec[VPP].iXX0      = memspec.mempowerspec.ipp0;
    memPowerSpec[VPP].iXX2N     = memspec.mempowerspec.ipp2n;
    memPowerSpec[VPP].iXX3N     = memspec.mempowerspec.ipp3n;
    memPowerSpec[VPP].iXX4R     = memspec.mempowerspec.ipp4r;
    memPowerSpec[VPP].iXX4W     = memspec.mempowerspec.ipp4w;
    memPowerSpec[VPP].iXX5C     = memspec.mempowerspec.ipp5c;
    memPowerSpec[VPP].iXX6N     = memspec.mempowerspec.ipp6n;
    memPowerSpec[VPP].iXX2P     = memspec.mempowerspec.ipp2p;
    memPowerSpec[VPP].iXX3P     = memspec.mempowerspec.ipp3p;

    vddq = memspec.mempowerspec.vddq;


    if (memspec.memarchitecturespec.RefMode == 1) {
        memPowerSpec[VDD].iXX5X = memspec.mempowerspec.idd5b;
        memPowerSpec[VPP].iXX5X = memspec.mempowerspec.ipp5b;
        memTimingSpec.tRFC = memspec.memtimingspec.RFC1_slr;
    } else {
        memPowerSpec[VDD].iXX5X = memspec.mempowerspec.idd5f;
        memPowerSpec[VPP].iXX5X = memspec.mempowerspec.ipp5f;
        memTimingSpec.tRFC = memspec.memtimingspec.RFC2_slr;
    }

    memPowerSpec[VDD].iBeta = memspec.mempowerspec.iBeta_vdd.value_or(memspec.mempowerspec.idd0);
    memPowerSpec[VPP].iBeta = memspec.mempowerspec.iBeta_vpp.value_or(memspec.mempowerspec.ipp0);


    if (memspec.bankwisespec.has_value())
    {
        bwParams.bwPowerFactRho = memspec.bankwisespec.value().factRho.value_or(1);
    }
    else
    {
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

void MemSpecDDR5::parseImpedanceSpec(const DRAMUtils::MemSpec::MemSpecDDR5 &memspec) {
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

void MemSpecDDR5::parseDataRateSpec(const DRAMUtils::MemSpec::MemSpecDDR5 &memspec) {
    if (!memspec.dataratespec.has_value()) {
        dataRateSpec = {2, 2, 2};
        return;
    }

    dataRateSpec.commandBusRate = memspec.dataratespec.value().ca_bus_rate;
    dataRateSpec.dataBusRate =  memspec.dataratespec.value().dq_bus_rate;
    dataRateSpec.dqsBusRate = memspec.dataratespec.value().dqs_bus_rate;
}

MemSpecDDR5 MemSpecDDR5::from_memspec(const DRAMUtils::MemSpec::MemSpecVariant& memSpec)
{
    return std::get<DRAMUtils::MemSpec::MemSpecDDR5>(memSpec.getVariant());
}
