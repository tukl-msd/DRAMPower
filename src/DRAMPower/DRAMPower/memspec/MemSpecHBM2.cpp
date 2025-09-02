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

#include "MemSpecHBM2.h"
using namespace DRAMPower;


MemSpecHBM2::MemSpecHBM2(const DRAMUtils::MemSpec::MemSpecHBM2 &memspec)
    : MemSpec(memspec)
{

    numberOfBankGroups     = memspec.memarchitecturespec.nbrOfBankGroups;
    numberOfStacks          = memspec.memarchitecturespec.nbrOfStacks;
    numberOfPseudoChannels = memspec.memarchitecturespec.nbrOfPseudoChannels;

    memTimingSpec.tCK      = memspec.memtimingspec.tCK;
    memTimingSpec.tRAS     = memspec.memtimingspec.RAS;
    memTimingSpec.tRTP     = memspec.memtimingspec.RTP;
    memTimingSpec.tWL      = memspec.memtimingspec.WL;
    memTimingSpec.tWR      = memspec.memtimingspec.WR;
    memTimingSpec.tRP      = memspec.memtimingspec.RP;


    auto VDD = VoltageDomain::VDD;
    auto VPP = VoltageDomain::VPP;

    memPowerSpec.push_back(MemPowerSpec());

    memPowerSpec[VDD].vXX       = memspec.mempowerspec.vdd;
    memPowerSpec[VDD].iXX0      = memspec.mempowerspec.idd0;
    memPowerSpec[VDD].iXX2N     = memspec.mempowerspec.idd2n;
    memPowerSpec[VDD].iXX3N     = memspec.mempowerspec.idd3n;
    memPowerSpec[VDD].iXX4R     = memspec.mempowerspec.idd4r;
    memPowerSpec[VDD].iXX4W     = memspec.mempowerspec.idd4w;
    memPowerSpec[VDD].iXX6N     = memspec.mempowerspec.idd6n;
    memPowerSpec[VDD].iXX2P     = memspec.mempowerspec.idd2p;
    memPowerSpec[VDD].iXX3P     = memspec.mempowerspec.idd3p;

    memPowerSpec.push_back(MemPowerSpec());

    memPowerSpec[VPP].vXX       = memspec.mempowerspec.vpp;
    memPowerSpec[VPP].iXX0      = memspec.mempowerspec.ipp0;
    memPowerSpec[VPP].iXX2N     = memspec.mempowerspec.ipp2n;
    memPowerSpec[VPP].iXX3N     = memspec.mempowerspec.ipp3n;
    memPowerSpec[VPP].iXX4R     = memspec.mempowerspec.ipp4r;
    memPowerSpec[VPP].iXX4W     = memspec.mempowerspec.ipp4w;
    memPowerSpec[VPP].iXX6N     = memspec.mempowerspec.ipp6n;
    memPowerSpec[VPP].iXX2P     = memspec.mempowerspec.ipp2p;
    memPowerSpec[VPP].iXX3P     = memspec.mempowerspec.ipp3p;

    vddq = memspec.mempowerspec.vddq;

    memTimingSpec.tRFC = memspec.memtimingspec.RFC;
    memTimingSpec.tRFCSB = memspec.memtimingspec.RFCSB;
    memPowerSpec[VDD].iBeta = memspec.mempowerspec.iBeta_vdd.value_or(memspec.mempowerspec.idd0);
    memPowerSpec[VPP].iBeta = memspec.mempowerspec.iBeta_vpp.value_or(memspec.mempowerspec.ipp0);

    if(memspec.bankwisespec.has_value())
    {
        bwParams.bwPowerFactRho = memspec.bankwisespec.value().factRho.value_or(1);
    }
    else
    {
        bwParams.bwPowerFactRho = 1;
    }

    memTimingSpec.tBurst = burstLength/dataRate;
    prechargeOffsetRD      =  memTimingSpec.tRTP; // TODO: verify tAL removed
    prechargeOffsetWR      =  memTimingSpec.tBurst + memTimingSpec.tWL + memTimingSpec.tWR;

    parseImpedanceSpec(memspec);
}

void MemSpecHBM2::parseImpedanceSpec(const DRAMUtils::MemSpec::MemSpecHBM2 &memspec) {
    memImpedanceSpec = memspec.memimpedancespec;
}

MemSpecHBM2 MemSpecHBM2::from_memspec(const DRAMUtils::MemSpec::MemSpecVariant& memSpec)
{
    return std::get<DRAMUtils::MemSpec::MemSpecHBM2>(memSpec.getVariant());
}

