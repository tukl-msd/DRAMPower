/*
 * Copyright (c) 2019, University of Kaiserslautern
 * Copyright (c) 2012-2020, Fraunhofer IESE
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

#include "MemSpecWideIO.h"
using namespace DRAMPower;
using json = nlohmann::json;


MemSpecWideIO::MemSpecWideIO(nlohmann::json &memspec)
    : MemSpec(memspec)
{
    numberOfChannels       = parseUint(memspec["memarchitecturespec"]["nbrOfChannels"],"nbrOfChannels");
    numberOfRanks          = parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks");
    memTimingSpec.fCKMHz   = (parseUdouble(memspec["memtimingspec"]["clkMhz"], "clkMhz"));
    memTimingSpec.tCK      = (1000.0 / memTimingSpec.fCKMHz); //clock period in mili seconds
    memTimingSpec.tCKESR   = (parseUint(memspec["memtimingspec"]["CKESR"], "CKESR"));
    memTimingSpec.tCKE     = (parseUint(memspec["memtimingspec"]["CKE"], "CKE"));
    memTimingSpec.tDQSCK   = (parseUint(memspec["memtimingspec"]["DQSCK"], "DQSCK"));
    memTimingSpec.tRAS     = (parseUint(memspec["memtimingspec"]["RAS"], "RAS"));
    memTimingSpec.tRC      = (parseUint(memspec["memtimingspec"]["RC"], "RC"));
    memTimingSpec.tRCD     = (parseUint(memspec["memtimingspec"]["RCD"], "RCD"));
    memTimingSpec.tRL      = (parseUint(memspec["memtimingspec"]["RL"], "RL"));
    memTimingSpec.tWL      = (parseUint(memspec["memtimingspec"]["WL"], "WL"));
    memTimingSpec.tWR      = (parseUint(memspec["memtimingspec"]["WR"], "WR"));
    memTimingSpec.tXP      = (parseUint(memspec["memtimingspec"]["XP"], "XP"));
    memTimingSpec.tXSR     = (parseUint(memspec["memtimingspec"]["XSR"], "XSR"));
    memTimingSpec.tCCD_R   = (parseUint(memspec["memtimingspec"]["CCD_R"], "CCD_R"));
    memTimingSpec.tCCD_W   = (parseUint(memspec["memtimingspec"]["CCD_W"], "CCD_W"));
    memTimingSpec.tRP      = (parseUint(memspec["memtimingspec"]["RP"], "RP"));
    memTimingSpec.tRFC = (parseUint(memspec["memtimingspec"]["RFC"], "RFC"));
    memTimingSpec.tREFI    = (parseUint(memspec["memtimingspec"]["REFI"], "REFI"));
    memTimingSpec.tRRD     = (parseUint(memspec["memtimingspec"]["RRD"], "RRD"));
    memTimingSpec.tWTR     = (parseUint(memspec["memtimingspec"]["WTR"], "WTR"));
    memTimingSpec.tRTRS    = (parseUint(memspec["memtimingspec"]["RTRS"], "RTRS"));

    prechargeOffsetRD      =  burstLength;
    prechargeOffsetWR      =  memTimingSpec.tWL + burstLength + memTimingSpec.tWR - 1;

    //Push back new subject created with default constructor.
    memPowerSpec.resize(2);

    // Currents in JEDEC are given when 3 channels are in idle power down state
    // Subtract (3/4)*iDD2pX to have the values only for 1 channel.

    memPowerSpec[0].iDD2PX  = (parseUdouble(memspec["mempowerspec"]["idd2p1"], "idd2p1"))/4;

    memPowerSpec[0].iDD0X   = (parseUdouble(memspec["mempowerspec"]["idd01"], "idd01"))
                                                            - 3*memPowerSpec[0].iDD2PX;

    memPowerSpec[0].iDD2NX  = (parseUdouble(memspec["mempowerspec"]["idd2n1"], "idd2n1"))
                                                              - 3*memPowerSpec[0].iDD2PX;

    memPowerSpec[0].iDD3PX  = (parseUdouble(memspec["mempowerspec"]["idd3p1"], "idd3p1"))
                                                              - 3*memPowerSpec[0].iDD2PX;

    memPowerSpec[0].iDD3NX  = (parseUdouble(memspec["mempowerspec"]["idd3n1"], "idd3n1"))
                                                              - 3*memPowerSpec[0].iDD2PX;

    memPowerSpec[0].iDD4RX  = (parseUdouble(memspec["mempowerspec"]["idd4r1"], "idd4r1"))
                                                              - 3*memPowerSpec[0].iDD2PX;

    memPowerSpec[0].iDD4WX  = (parseUdouble(memspec["mempowerspec"]["idd4w1"], "idd4w1"))
                                                              - 3*memPowerSpec[0].iDD2PX;

    memPowerSpec[0].iDD5X   = (parseUdouble(memspec["mempowerspec"]["idd51"], "idd51"))
                                                            - 3*memPowerSpec[0].iDD2PX;

    memPowerSpec[0].iDD6X   = (parseUdouble(memspec["mempowerspec"]["idd61"], "idd61"))
                                                            - 3*memPowerSpec[0].iDD2PX;

    memPowerSpec[0].vDDX    = (parseUdouble(memspec["mempowerspec"]["vdd1"], "vdd1"));


    memPowerSpec[1].iDD2PX  = (parseUdoubleWithDefault(memspec["mempowerspec"]["idd2p2"], "idd2p2"))/4;

    memPowerSpec[1].iDD0X   = (parseUdoubleWithDefault(memspec["mempowerspec"]["idd02"], "idd02"))
                                                                       - 3*memPowerSpec[1].iDD2PX;

    memPowerSpec[1].iDD2NX  = (parseUdoubleWithDefault(memspec["mempowerspec"]["idd2n2"], "idd2n2"))
                                                                         - 3*memPowerSpec[1].iDD2PX;

    memPowerSpec[1].iDD3PX  = (parseUdoubleWithDefault(memspec["mempowerspec"]["idd3p2"], "idd3p2"))
                                                                         - 3*memPowerSpec[1].iDD2PX;

    memPowerSpec[1].iDD3NX  = (parseUdoubleWithDefault(memspec["mempowerspec"]["idd3n2"], "idd3n2"))
                                                                         - 3*memPowerSpec[1].iDD2PX;

    memPowerSpec[1].iDD4RX  = (parseUdoubleWithDefault(memspec["mempowerspec"]["idd4r2"], "idd4r2"))
                                                                         - 3*memPowerSpec[1].iDD2PX;

    memPowerSpec[1].iDD4WX  = (parseUdoubleWithDefault(memspec["mempowerspec"]["idd4w2"], "idd4w2"))
                                                                         - 3*memPowerSpec[1].iDD2PX;

    memPowerSpec[1].iDD5X   = (parseUdoubleWithDefault(memspec["mempowerspec"]["idd52"], "idd52"))
                                                                       - 3*memPowerSpec[1].iDD2PX;

    memPowerSpec[1].iDD6X   = (parseUdoubleWithDefault(memspec["mempowerspec"]["idd62"], "idd62"))
                                                                       - 3*memPowerSpec[1].iDD2PX;

    memPowerSpec[1].vDDX    = (parseUdoubleWithDefault(memspec["mempowerspec"]["vdd2"], "vdd2"));

    //optional parameters
    memPowerSpec[0].capacitance = (parseUdoubleWithDefault(memspec["mempowerspec"]["capacitance"], "capacitance"));
    memPowerSpec[0].ioPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["ioPower"], "ioPower"));
    memPowerSpec[0].wrOdtPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["wrOdtPower"], "wrOdtPower"));
    memPowerSpec[0].termRdPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["termRdPower"], "termRdPower"));
    memPowerSpec[0].termWrPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["termWrPower"], "termWrPower"));


    json bankWise = memspec["bankwisespec"];
    if (!bankWise.empty()){
        json rho = memspec["bankwisespec"]["factRho"];
        json sigma = memspec["bankwisespec"]["factSigma"];
        if (!rho.empty()) bwParams.bwPowerFactRho = parseUint(memspec["bankwisespec"]["factRho"],"factRho");
        else bwParams.bwPowerFactRho = 100;
        if (!sigma.empty()) bwParams.bwPowerFactSigma = parseUint(memspec["bankwisespec"]["factSigma"],"factSigma");
        else bwParams.bwPowerFactSigma = 100;
    } // end if !bankwise.empty()
    else{
        bwParams.bwPowerFactRho = 100;
        bwParams.bwPowerFactSigma = 100;
    }
}

int64_t MemSpecWideIO::timeToCompletion(DRAMPower::MemCommand::cmds type)
{
    int64_t offset = 0;

    if (type == DRAMPower::MemCommand::RD) {
        offset = memTimingSpec.tRL + memTimingSpec.tDQSCK + 1
                + (burstLength / dataRate);
    } else if (type == DRAMPower::MemCommand::WR) {
        offset = memTimingSpec.tWL +(burstLength / dataRate) + memTimingSpec.tWR;
    } else if (type == DRAMPower::MemCommand::ACT) {
        offset = memTimingSpec.tRCD;
    } else if ((type == DRAMPower::MemCommand::PRE) ||
               (type == DRAMPower::MemCommand::PREA)) {
        offset = memTimingSpec.tRP;
    }
    return offset;
} // MemSpecWideIO::timeToCompletion



