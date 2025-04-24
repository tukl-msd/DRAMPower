#include "DRAMPower/memspec/MemSpecLPDDR5.h"

using namespace DRAMPower;


MemSpecLPDDR5::MemSpecLPDDR5(const DRAMUtils::MemSpec::MemSpecLPDDR5 &memspec)
        : MemSpec(memspec)
{
    numberOfChannels        = memspec.memarchitecturespec.nbrOfChannels;
    numberOfBankGroups      = memspec.memarchitecturespec.nbrOfBankGroups;
    numberOfRanks           = memspec.memarchitecturespec.nbrOfRanks;
    banksPerGroup           = numberOfBanks / numberOfBankGroups;

    memTimingSpec.tCK       = memspec.memtimingspec.tCK;
    memTimingSpec.WCKtoCK   = memspec.memtimingspec.WCK2CK;
    memTimingSpec.tWCK      = memTimingSpec.tCK / memTimingSpec.WCKtoCK;
    memTimingSpec.tRAS      = memspec.memtimingspec.RAS;
    memTimingSpec.tRCD      = memspec.memtimingspec.RCD_S;
    memTimingSpec.tRBTP     = memspec.memtimingspec.RBTP;
    memTimingSpec.tWL       = memspec.memtimingspec.WL;
    memTimingSpec.tWR       = memspec.memtimingspec.WR;
    memTimingSpec.tRP       = memspec.memtimingspec.RPpb;
    memTimingSpec.tRFCPB    = memspec.memtimingspec.RFCpb;
    memTimingSpec.tRFC      = memspec.memtimingspec.RFCab;
    memTimingSpec.tREFI     = memspec.memtimingspec.REFI;

    auto VDD1 = VoltageDomain::VDD1;
    auto VDD2H = VoltageDomain::VDD2H;
    auto VDD2L = VoltageDomain::VDD2L;

    memPowerSpec.push_back(MemPowerSpec()); // VDD1
    memPowerSpec.push_back(MemPowerSpec()); // VDD2H
    memPowerSpec.push_back(MemPowerSpec()); // VDD2L

    memPowerSpec[VDD1].vDDX       = memspec.mempowerspec.vdd1;
    memPowerSpec[VDD1].iDD0X      = memspec.mempowerspec.idd01;
    memPowerSpec[VDD1].iDD2NX     = memspec.mempowerspec.idd2n1;
    memPowerSpec[VDD1].iDD3NX     = memspec.mempowerspec.idd3n1;
    memPowerSpec[VDD1].iDD4RX     = memspec.mempowerspec.idd4r1;
    memPowerSpec[VDD1].iDD4WX     = memspec.mempowerspec.idd4w1;
    memPowerSpec[VDD1].iDD5X      = memspec.mempowerspec.idd51;
    memPowerSpec[VDD1].iDD5PBX    = memspec.mempowerspec.idd5pb1;
    memPowerSpec[VDD1].iDD6X      = memspec.mempowerspec.idd61;
    memPowerSpec[VDD1].iDD6DSX    = memspec.mempowerspec.idd6ds1;
    memPowerSpec[VDD1].iDD2PX     = memspec.mempowerspec.idd2p1;
    memPowerSpec[VDD1].iDD3PX     = memspec.mempowerspec.idd3p1;

    memPowerSpec[VDD2H].vDDX       = memspec.mempowerspec.vdd2h;
    memPowerSpec[VDD2H].iDD0X      = memspec.mempowerspec.idd02h;
    memPowerSpec[VDD2H].iDD2NX     = memspec.mempowerspec.idd2n2h;
    memPowerSpec[VDD2H].iDD3NX     = memspec.mempowerspec.idd3n2h;
    memPowerSpec[VDD2H].iDD4RX     = memspec.mempowerspec.idd4r2h;
    memPowerSpec[VDD2H].iDD4WX     = memspec.mempowerspec.idd4w2h;
    memPowerSpec[VDD2H].iDD5X      = memspec.mempowerspec.idd52h;
    memPowerSpec[VDD2H].iDD5PBX    = memspec.mempowerspec.idd5pb2h;
    memPowerSpec[VDD2H].iDD6X      = memspec.mempowerspec.idd62h;
    memPowerSpec[VDD2H].iDD6DSX    = memspec.mempowerspec.idd6ds2h;
    memPowerSpec[VDD2H].iDD2PX     = memspec.mempowerspec.idd2p2h;
    memPowerSpec[VDD2H].iDD3PX     = memspec.mempowerspec.idd3p2h;

    memPowerSpec[VDD2L].vDDX       = memspec.mempowerspec.vdd2l;
    memPowerSpec[VDD2L].iDD0X      = memspec.mempowerspec.idd02l;
    memPowerSpec[VDD2L].iDD2NX     = memspec.mempowerspec.idd2n2l;
    memPowerSpec[VDD2L].iDD3NX     = memspec.mempowerspec.idd3n2l;
    memPowerSpec[VDD2L].iDD4RX     = memspec.mempowerspec.idd4r2l;
    memPowerSpec[VDD2L].iDD4WX     = memspec.mempowerspec.idd4w2l;
    memPowerSpec[VDD2L].iDD5X      = memspec.mempowerspec.idd52l;
    memPowerSpec[VDD2L].iDD5PBX    = memspec.mempowerspec.idd5pb2l;
    memPowerSpec[VDD2L].iDD6X      = memspec.mempowerspec.idd62l;
    memPowerSpec[VDD2L].iDD6DSX    = memspec.mempowerspec.idd6ds2l;
    memPowerSpec[VDD2L].iDD2PX     = memspec.mempowerspec.idd2p2l;
    memPowerSpec[VDD2L].iDD3PX     = memspec.mempowerspec.idd3p2l;

    vddq       = memspec.mempowerspec.vddq;

    memPowerSpec[VDD1].iBeta = memspec.mempowerspec.iBeta_vdd1.value_or(memspec.mempowerspec.idd01);
    memPowerSpec[VDD2H].iBeta = memspec.mempowerspec.iBeta_vdd2h.value_or(memspec.mempowerspec.idd02h);
    memPowerSpec[VDD2L].iBeta = memspec.mempowerspec.iBeta_vdd2l.value_or(memspec.mempowerspec.idd02l);

    if (memspec.bankwisespec.has_value()) {
        bwParams.bwPowerFactRho = memspec.bankwisespec.value().factRho.value_or(1);
    }
    else {
        bwParams.bwPowerFactRho = 1;
    }


    auto BankArchError = [this]() {
        std::cout << "Invalid bank architecture selected" << std::endl;
        std::cout << "Selected values:" << std::endl;
        std::cout << "  - Number of banks: " << numberOfBanks << std::endl;
        std::cout << "  - Number of bank groups: " << numberOfBankGroups << std::endl;
        std::cout << "Valid values are 16|1 (16B mode), 16|4 (BG mode) or 8|1 (8B mode)" << std::endl;
        std::cout << std::endl << "Assuming 16B architecture." << std::endl;
        bank_arch = BankArchitectureMode::M16B;
        numberOfBanks = 16;
        numberOfBankGroups = 1;
    };

    if (numberOfBanks == 16) {
        if (numberOfBankGroups == 1 || numberOfBankGroups == 0) {
            bank_arch = BankArchitectureMode::M16B;
        } else if (numberOfBankGroups == 4) {
            bank_arch = BankArchitectureMode::MBG;
        } else {
            BankArchError();
        }
    } else if (numberOfBanks == 8) {
        if (numberOfBankGroups > 1) {
            BankArchError();
        } else {
            bank_arch = BankArchitectureMode::M8B;
        }
    } else {
        BankArchError();
    }

    // Source: LPDDR5 standard; table 312
    memTimingSpec.tBurst = burstLength/(dataRate * memTimingSpec.WCKtoCK);

    // Source: LPDDR5 standard; figure 96
    prechargeOffsetRD      =   memTimingSpec.tBurst + memTimingSpec.tRBTP;

    // Source: LPDDR5 standard; figure 97
    prechargeOffsetWR      =  memTimingSpec.tWL + memTimingSpec.tBurst + 1 + memTimingSpec.tWR;

    wckAlwaysOnMode = memspec.memarchitecturespec.WCKalwaysOn;

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

void MemSpecLPDDR5::parseImpedanceSpec(const DRAMUtils::MemSpec::MemSpecLPDDR5 &memspec) {

    memImpedanceSpec = memspec.memimpedancespec;
}

MemSpecLPDDR5 MemSpecLPDDR5::from_memspec(const DRAMUtils::MemSpec::MemSpecVariant& memSpec)
{
    return std::get<DRAMUtils::MemSpec::MemSpecLPDDR5>(memSpec.getVariant());
}
