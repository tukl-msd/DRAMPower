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

#include "MemSpecLPDDR4.h"
using namespace DRAMPower;
using json = nlohmann::json;


MemSpecLPDDR4::MemSpecLPDDR4(nlohmann::json &memspec)
    : MemSpec(memspec)
{
    numberOfRanks          = parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks");
    numberOfChannels       = parseUint(memspec["memarchitecturespec"]["nbrOfChannels"],"nbrOfChannels");
    memTimingSpec.fCKMHz   = (parseUdouble(memspec["memtimingspec"]["clkMhz"], "clkMhz"));
    memTimingSpec.tCK      = (1000.0 / memTimingSpec.fCKMHz); //clock period in nano seconds
    memTimingSpec.tCKE     = (parseUint(memspec["memtimingspec"]["CKE"], "CKE"));
    memTimingSpec.tDQSCK   = (parseUint(memspec["memtimingspec"]["DQSCK"], "DQSCK"));
    memTimingSpec.tPPD     = (parseUint(memspec["memtimingspec"]["PPD"], "PPD"));
    memTimingSpec.tRAS     = (parseUint(memspec["memtimingspec"]["RAS"], "RAS"));
    memTimingSpec.tRC    = (parseUint(memspec["memtimingspec"]["RC"], "RC"));
    memTimingSpec.tRCD     = (parseUint(memspec["memtimingspec"]["RCD"], "RCD"));
    memTimingSpec.tRL      = (parseUint(memspec["memtimingspec"]["RL"], "RL"));
    memTimingSpec.tRTP     = (parseUint(memspec["memtimingspec"]["RTP"], "RTP"));
    memTimingSpec.tWL      = (parseUint(memspec["memtimingspec"]["WL"], "WL"));
    memTimingSpec.tWR      = (parseUint(memspec["memtimingspec"]["WR"], "WR"));
    memTimingSpec.tXP      = (parseUint(memspec["memtimingspec"]["XP"], "XP"));
    memTimingSpec.tSR      = (parseUint(memspec["memtimingspec"]["SR"], "SR"));
    memTimingSpec.tXSR     = (parseUint(memspec["memtimingspec"]["XSR"], "XSR"));
    memTimingSpec.tESCKE   = (parseUint(memspec["memtimingspec"]["ESCKE"], "ESCKE"));
    memTimingSpec.tCCDMW   = (parseUint(memspec["memtimingspec"]["CCDMW"], "CCDMW"));
    memTimingSpec.tCCD     = (parseUint(memspec["memtimingspec"]["CCD"], "CCD"));
    memTimingSpec.tRPab    = (parseUint(memspec["memtimingspec"]["RPAB"], "RPAB"));
    memTimingSpec.tRPpb    = (parseUint(memspec["memtimingspec"]["RPPB"], "RPPB"));
    memTimingSpec.tFAW     = (parseUint(memspec["memtimingspec"]["FAW"], "FAW"));
    memTimingSpec.tRFCab   = (parseUint(memspec["memtimingspec"]["RFCAB"], "RFCAB"));
    memTimingSpec.tRFCpb   = (parseUint(memspec["memtimingspec"]["RFCPB"], "RFCPB"));
    memTimingSpec.tREFIab  = (parseUint(memspec["memtimingspec"]["REFIAB"], "REFIAB"));
    memTimingSpec.tREFIpb  = (parseUint(memspec["memtimingspec"]["REFIPB"], "REFIPB"));
    memTimingSpec.tRRD     = (parseUint(memspec["memtimingspec"]["RRD"], "RRD"));
    memTimingSpec.tWTR     = (parseUint(memspec["memtimingspec"]["WTR"], "WTR"));
    memTimingSpec.tXP      = (parseUint(memspec["memtimingspec"]["XP"], "XP"));
    memTimingSpec.tDQSS    = (parseUint(memspec["memtimingspec"]["DQSS"], "DQSS"));
    memTimingSpec.tDQS2DQ  = (parseUint(memspec["memtimingspec"]["DQS2DQ"], "DQS2DQ"));

    prechargeOffsetRD      =  memTimingSpec.tRTP + ((burstLength)/(dataRate)) - 6;
    prechargeOffsetWR      =  memTimingSpec.tWL + ((burstLength)/(dataRate)) + memTimingSpec.tWR + 3;

    //Push back new subject created with default constructor.
    memPowerSpec.push_back(MemPowerSpec());

    memPowerSpec[0].iDD0X            = (parseUdouble(memspec["mempowerspec"]["idd0"  ], "idd0"  ));
    memPowerSpec[0].iDD2PX           = (parseUdouble(memspec["mempowerspec"]["idd2p" ], "idd2p" ));
    memPowerSpec[0].iDD2PSX          = (parseUdouble(memspec["mempowerspec"]["idd2ps"], "idd2ps"));
    memPowerSpec[0].iDD2NX           = (parseUdouble(memspec["mempowerspec"]["idd2n" ], "idd2n" ));
    memPowerSpec[0].iDD2NSX          = (parseUdouble(memspec["mempowerspec"]["idd2ns"], "idd2ns"));
    memPowerSpec[0].iDD3PX           = (parseUdouble(memspec["mempowerspec"]["idd3p" ], "idd3p" ));
    memPowerSpec[0].iDD3PSX          = (parseUdouble(memspec["mempowerspec"]["idd3ps"], "idd3ps"));
    memPowerSpec[0].iDD3NX           = (parseUdouble(memspec["mempowerspec"]["idd3n" ], "idd3n" ));
    memPowerSpec[0].iDD3NSX          = (parseUdouble(memspec["mempowerspec"]["idd3ns"], "idd3ns"));
    memPowerSpec[0].iDD4RX           = (parseUdouble(memspec["mempowerspec"]["idd4r" ], "idd4r" ));
    memPowerSpec[0].iDD4WX           = (parseUdouble(memspec["mempowerspec"]["idd4w" ], "idd4w" ));
    memPowerSpec[0].iDD5X            = (parseUdouble(memspec["mempowerspec"]["idd5"  ], "idd5"  ));
    memPowerSpec[0].iDD5ABX          = (parseUdouble(memspec["mempowerspec"]["idd5ab"], "idd5ap"));
    memPowerSpec[0].iDD5PBX          = (parseUdouble(memspec["mempowerspec"]["idd5pb"], "idd5ap"));
    memPowerSpec[0].iDD5PBX_BST      = 0.0; //TODO: decide which current to use
    memPowerSpec[0].iDD6X            = (parseUdouble(memspec["mempowerspec"]["idd6"  ], "idd6"  ));
    memPowerSpec[0].vDDX             = (parseUdouble(memspec["mempowerspec"]["vdd"], "vdd"));

    memPowerSpec.push_back(MemPowerSpec());
    memPowerSpec[1].iDD0X            = (parseUdouble(memspec["mempowerspec"]["idd02"  ], "idd02"  ));
    memPowerSpec[1].iDD2PX           = (parseUdouble(memspec["mempowerspec"]["idd2p2" ], "idd2p2" ));
    memPowerSpec[1].iDD2PSX          = (parseUdouble(memspec["mempowerspec"]["idd2ps2"], "idd2ps2"));
    memPowerSpec[1].iDD2NX           = (parseUdouble(memspec["mempowerspec"]["idd2n2" ], "idd2n2" ));
    memPowerSpec[1].iDD2NSX          = (parseUdouble(memspec["mempowerspec"]["idd2ns2"], "idd2ns2"));
    memPowerSpec[1].iDD3PX           = (parseUdouble(memspec["mempowerspec"]["idd3p2" ], "idd3p2" ));
    memPowerSpec[1].iDD3PSX          = (parseUdouble(memspec["mempowerspec"]["idd3ps2"], "idd3ps2"));
    memPowerSpec[1].iDD3NX           = (parseUdouble(memspec["mempowerspec"]["idd3n2" ], "idd3n2" ));
    memPowerSpec[1].iDD3NSX          = (parseUdouble(memspec["mempowerspec"]["idd3ns2"], "idd3ns2"));
    memPowerSpec[1].iDD4RX           = (parseUdouble(memspec["mempowerspec"]["idd4r2" ], "idd4r2" ));
    memPowerSpec[1].iDD4WX           = (parseUdouble(memspec["mempowerspec"]["idd4w2" ], "idd4w2" ));
    memPowerSpec[1].iDD5X            = (parseUdouble(memspec["mempowerspec"]["idd52"  ], "idd52"  ));
    memPowerSpec[1].iDD5ABX          = (parseUdouble(memspec["mempowerspec"]["idd5ab2"], "idd5ap2"));
    memPowerSpec[1].iDD5PBX          = (parseUdouble(memspec["mempowerspec"]["idd5pb2"], "idd5ap2"));
    memPowerSpec[1].iDD5PBX_BST      = 0.0; //TODO: decide which current to use
    memPowerSpec[1].iDD6X            = (parseUdouble(memspec["mempowerspec"]["idd62"  ], "idd62"  ));
    memPowerSpec[1].vDDX             = (parseUdouble(memspec["mempowerspec"]["vdd2"], "vdd2"));

    memPowerSpec.push_back(MemPowerSpec());
    memPowerSpec[2].iDD0X            = (parseUdouble(memspec["mempowerspec"]["idd0q"  ], "idd0q"  ));
    memPowerSpec[2].iDD2PX           = (parseUdouble(memspec["mempowerspec"]["idd2pq" ], "idd2pq" ));
    memPowerSpec[2].iDD2PSX          = (parseUdouble(memspec["mempowerspec"]["idd2psq"], "idd2psq"));
    memPowerSpec[2].iDD2NX           = (parseUdouble(memspec["mempowerspec"]["idd2nq" ], "idd2nq" ));
    memPowerSpec[2].iDD2NSX          = (parseUdouble(memspec["mempowerspec"]["idd2nsq"], "idd2nsq"));
    memPowerSpec[2].iDD3PX           = (parseUdouble(memspec["mempowerspec"]["idd3pq" ], "idd3pq" ));
    memPowerSpec[2].iDD3PSX          = (parseUdouble(memspec["mempowerspec"]["idd3psq"], "idd3psq"));
    memPowerSpec[2].iDD3NX           = (parseUdouble(memspec["mempowerspec"]["idd3nq" ], "idd3nq" ));
    memPowerSpec[2].iDD3NSX          = (parseUdouble(memspec["mempowerspec"]["idd3nsq"], "idd3nsq"));
    memPowerSpec[2].iDD4RX           = (parseUdouble(memspec["mempowerspec"]["idd4rq" ], "idd4rq" ));
    memPowerSpec[2].iDD4WX           = (parseUdouble(memspec["mempowerspec"]["idd4wq" ], "idd4wq" ));
    memPowerSpec[2].iDD5X            = (parseUdouble(memspec["mempowerspec"]["idd5q"  ], "idd5q"  ));
    memPowerSpec[2].iDD5ABX          = (parseUdouble(memspec["mempowerspec"]["idd5abq"], "idd5apq"));
    memPowerSpec[2].iDD5PBX          = (parseUdouble(memspec["mempowerspec"]["idd5pbq"], "idd5apq"));
    memPowerSpec[2].iDD5PBX_BST      = 0.0; //TODO: decide which current to use
    memPowerSpec[2].iDD6X            = (parseUdouble(memspec["mempowerspec"]["idd6q"  ], "idd6q"  ));
    memPowerSpec[2].vDDX             = (parseUdouble(memspec["mempowerspec"]["vddq"], "vddq"));


    //optional parameters
    ioPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["ioPower"], "ioPower"));
    capacitance = (parseUdoubleWithDefault(memspec["mempowerspec"]["capacitance"], "capacitance"));
    wrOdtPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["wrOdtPower"], "wrOdtPower"));
    termRdPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["termRdPower"], "termRdPower"));
    termWrPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["termWrPower"], "termWrPower"));


    json bankWise = memspec["bankwisespec"];
    if (!bankWise.empty()) {
        json rho = memspec["bankwisespec"]["factRho"];
        json sigma = memspec["bankwisespec"]["factSigma"];
        if (!rho.empty()) bwParams.bwPowerFactRho = parseUint(memspec["bankwisespec"]["factRho"],"factRho");
        else bwParams.bwPowerFactRho = 100;
        if (!sigma.empty()) bwParams.bwPowerFactSigma = parseUint(memspec["bankwisespec"]["factSigma"],"factSigma");
        else bwParams.bwPowerFactSigma = 100;
    } // end if !bankwise.empty()
    else {
        bwParams.bwPowerFactRho = 100;
        bwParams.bwPowerFactSigma = 100;
    }
}


int64_t MemSpecLPDDR4::timeToCompletion(DRAMPower::MemCommand::cmds type)
{
    int64_t offset = 0;

    if (type == DRAMPower::MemCommand::ACT)
        offset = memTimingSpec.tRCD + 3;
    else if (type == DRAMPower::MemCommand::RD)
        offset = memTimingSpec.tRL + memTimingSpec.tDQSCK + ((burstLength)/(dataRate)) + 3;
    else if (type == DRAMPower::MemCommand::WR)
        offset = memTimingSpec.tWL + memTimingSpec.tDQSS + memTimingSpec.tDQS2DQ + ((burstLength)/(dataRate)) + 3;
    else if (type == MemCommand::REF)
        offset = memTimingSpec.tRFCab + 1;
    else if (type == MemCommand::REFB)
        offset = memTimingSpec.tRFCpb + 1;
    else if (type == MemCommand::PRE)
        return memTimingSpec.tRPpb + 1;
    else if (type == MemCommand::PREA)
        return memTimingSpec.tRPab + 1;
    else
        PRINTDEBUGMESSAGE("timeToCompletion not available for given Command Type", 0, type, 0);
    return offset;

} // MemSpecLPDDR4::timeToCompletion



