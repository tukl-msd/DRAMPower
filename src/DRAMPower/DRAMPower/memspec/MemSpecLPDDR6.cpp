#include "DRAMPower/memspec/MemSpecLPDDR6.h"

using namespace DRAMPower;


MemSpecLPDDR6::MemSpecLPDDR6(const DRAMUtils::MemSpec::MemSpecLPDDR6 &memspec)
        : MemSpec(memspec)
{
    numberOfBankGroups      = memspec.memarchitecturespec.nbrOfBankGroups;
    numberOfRanks           = memspec.memarchitecturespec.nbrOfRanks;
    perTwoBankOffset        = memspec.memarchitecturespec.per2BankOffset;
    banksPerGroup           = numberOfBanks / numberOfBankGroups;

    memTimingSpec.tCK       = memspec.memtimingspec.tCK;
    memTimingSpec.WCKtoCK   = memspec.memtimingspec.WCK2CK;
    memTimingSpec.tWCK      = memTimingSpec.tCK / memTimingSpec.WCKtoCK;
    memTimingSpec.tRAS      = memspec.memtimingspec.RAS;
    memTimingSpec.tRCDR      = memspec.memtimingspec.RCD_r;
    memTimingSpec.tRCDW      = memspec.memtimingspec.RCD_w;
    memTimingSpec.tRBTP     = memspec.memtimingspec.RBTP;
    memTimingSpec.tWL       = memspec.memtimingspec.WL;
    memTimingSpec.tWR       = memspec.memtimingspec.WR;
    memTimingSpec.tRP       = memspec.memtimingspec.RPpb;
    memTimingSpec.tRFCDB    = memspec.memtimingspec.RFCdb;
    memTimingSpec.tRFCAB      = memspec.memtimingspec.RFCab;
    memTimingSpec.tREFI     = memspec.memtimingspec.REFI;

    auto VDD1 = VoltageDomain::VDD1;
    auto VDD2C = VoltageDomain::VDD2C;
    auto VDD2D = VoltageDomain::VDD2D;

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
    memPowerSpec[VDD1].iDD5PDBX    = memspec.mempowerspec.idd5pdb1;
    memPowerSpec[VDD1].iDD6X      = memspec.mempowerspec.idd61;
    memPowerSpec[VDD1].iDD6DSX    = memspec.mempowerspec.idd6ds1;
    memPowerSpec[VDD1].iDD2PX     = memspec.mempowerspec.idd2p1;
    memPowerSpec[VDD1].iDD3PX     = memspec.mempowerspec.idd3p1;

    memPowerSpec[VDD2C].vDDX       = memspec.mempowerspec.vdd2c;
    memPowerSpec[VDD2C].iDD0X      = memspec.mempowerspec.idd02c;
    memPowerSpec[VDD2C].iDD2NX     = memspec.mempowerspec.idd2n2c;
    memPowerSpec[VDD2C].iDD3NX     = memspec.mempowerspec.idd3n2c;
    memPowerSpec[VDD2C].iDD4RX     = memspec.mempowerspec.idd4r2c;
    memPowerSpec[VDD2C].iDD4WX     = memspec.mempowerspec.idd4w2c;
    memPowerSpec[VDD2C].iDD5X      = memspec.mempowerspec.idd52c;
    memPowerSpec[VDD2C].iDD5PDBX    = memspec.mempowerspec.idd5pdb2c;
    memPowerSpec[VDD2C].iDD6X      = memspec.mempowerspec.idd62c;
    memPowerSpec[VDD2C].iDD6DSX    = memspec.mempowerspec.idd6ds2c;
    memPowerSpec[VDD2C].iDD2PX     = memspec.mempowerspec.idd2p2c;
    memPowerSpec[VDD2C].iDD3PX     = memspec.mempowerspec.idd3p2c;

    memPowerSpec[VDD2D].vDDX       = memspec.mempowerspec.vdd2d;
    memPowerSpec[VDD2D].iDD0X      = memspec.mempowerspec.idd02d;
    memPowerSpec[VDD2D].iDD2NX     = memspec.mempowerspec.idd2n2d;
    memPowerSpec[VDD2D].iDD3NX     = memspec.mempowerspec.idd3n2d;
    memPowerSpec[VDD2D].iDD4RX     = memspec.mempowerspec.idd4r2d;
    memPowerSpec[VDD2D].iDD4WX     = memspec.mempowerspec.idd4w2d;
    memPowerSpec[VDD2D].iDD5X      = memspec.mempowerspec.idd52d;
    memPowerSpec[VDD2D].iDD5PDBX    = memspec.mempowerspec.idd5pdb2d;
    memPowerSpec[VDD2D].iDD6X      = memspec.mempowerspec.idd62d;
    memPowerSpec[VDD2D].iDD6DSX    = memspec.mempowerspec.idd6ds2d;
    memPowerSpec[VDD2D].iDD2PX     = memspec.mempowerspec.idd2p2d;
    memPowerSpec[VDD2D].iDD3PX     = memspec.mempowerspec.idd3p2d;

    vddq       = memspec.mempowerspec.vddq;

    memPowerSpec[VDD1].iBeta = memspec.mempowerspec.iBeta_vdd1.value_or(memspec.mempowerspec.idd01);
    memPowerSpec[VDD2C].iBeta = memspec.mempowerspec.iBeta_vdd2c.value_or(memspec.mempowerspec.idd02c);
    memPowerSpec[VDD2D].iBeta = memspec.mempowerspec.iBeta_vdd2d.value_or(memspec.mempowerspec.idd02d);

    if (memspec.bankwisespec.has_value()) {
        bwParams.bwPowerFactRho = memspec.bankwisespec.value().factRho.value_or(1);
    }
    else {
        bwParams.bwPowerFactRho = 1;
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

void MemSpecLPDDR6::parseImpedanceSpec(const DRAMUtils::MemSpec::MemSpecLPDDR6 &memspec) {
    memImpedanceSpec = memspec.memimpedancespec;
}

MemSpecLPDDR6 MemSpecLPDDR6::from_memspec(const DRAMUtils::MemSpec::MemSpecVariant& memSpec)
{
    return std::get<DRAMUtils::MemSpec::MemSpecLPDDR6>(memSpec.getVariant());
}
