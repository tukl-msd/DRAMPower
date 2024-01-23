/*
 * Copyright (c) 2019, University of Kaiserslautern
 * Copyright (c) 2012-2021, Fraunhofer IESE
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *    Lukas Steiner
 *    Luiza Correa
 */

#include "MemSpecDDR4.h"
using namespace DRAMPower;
using json = nlohmann::json;


MemSpecDDR4::MemSpecDDR4(nlohmann::json &memspec)
    : MemSpec(memspec)
{

    numberOfBankGroups     = parseUint(memspec["memarchitecturespec"]["nbrOfBankGroups"],"nbrOfBankGroups");
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
    memTimingSpec.tAL      = (parseUint(memspec["memtimingspec"]["AL"], "AL"));


    auto VDD = VoltageDomain::VDD;
    auto VPP = VoltageDomain::VPP;

    memPowerSpec.push_back(MemPowerSpec());

    memPowerSpec[VDD].iXX0      = (parseUdouble(memspec["mempowerspec"]["idd0"], "idd0"));
    memPowerSpec[VDD].iXX2N     = (parseUdouble(memspec["mempowerspec"]["idd2n"], "idd2n"));
    memPowerSpec[VDD].iXX3N     = (parseUdouble(memspec["mempowerspec"]["idd3n"], "idd3n"));
    memPowerSpec[VDD].iXX4R     = (parseUdouble(memspec["mempowerspec"]["idd4r"], "idd4r"));
    memPowerSpec[VDD].iXX4W     = (parseUdouble(memspec["mempowerspec"]["idd4w"], "idd4w"));
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
    memPowerSpec[VPP].iXX6N     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp6"], "ipp6"));
    memPowerSpec[VPP].vXX       = (parseUdoubleWithDefault(memspec["mempowerspec"]["vpp"], "vpp"));
    memPowerSpec[VPP].iXX2P     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp2p"], "ipp2p"));
    memPowerSpec[VPP].iXX3P     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp3p"], "ipp3p"));


    if (refreshMode==1) {
        memPowerSpec[VDD].iXX5X      = (parseUdouble(memspec["mempowerspec"]["idd5B"], "idd5B"));
        memPowerSpec[VPP].iXX5X      = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp5B"], "ipp5B"));
        memTimingSpec.tRFC = (parseUint(memspec["memtimingspec"]["RFC1"], "RFC1"));
    }
    else if (refreshMode==2) {
        memPowerSpec[VDD].iXX5X      = (parseUdouble(memspec["mempowerspec"]["idd5F2"], "idd5F2"));
        memPowerSpec[VPP].iXX5X      = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp5F2"], "ipp5F2"));
        memTimingSpec.tRFC = (parseUint(memspec["memtimingspec"]["RFC2"], "RFC2"));
    }else {
        memPowerSpec[VDD].iXX5X      = (parseUdouble(memspec["mempowerspec"]["idd5F4"], "idd5F4"));
        memPowerSpec[VPP].iXX5X      = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp5F4"], "ipp5F4"));
        memTimingSpec.tRFC = (parseUint(memspec["memtimingspec"]["RFC4"], "RFC4"));
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
    prechargeOffsetRD      =  memTimingSpec.tAL + memTimingSpec.tRTP;
    prechargeOffsetWR      =  memTimingSpec.tBurst + memTimingSpec.tWL + memTimingSpec.tWR;

    parseImpedanceSpec(memspec);
}

void MemSpecDDR4::parseImpedanceSpec(nlohmann::json &memspec) {
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

// TODO: is this being used?
uint64_t MemSpecDDR4::timeToCompletion(DRAMPower::CmdType type)
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







