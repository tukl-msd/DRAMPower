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

#ifndef MEMSPECLPDDR4_H
#define MEMSPECLPDDR4_H

#include "MemSpec.h"

namespace DRAMPower {
class MemSpecLPDDR4 final : public MemSpec
{
public:
    MemSpecLPDDR4(nlohmann::json &memspec);

    int64_t timeToCompletion(DRAMPower::MemCommand::cmds type) override;

    unsigned numberOfRanks;
    unsigned numberOfChannels;
    // Memspec Variables:
    struct MemTimingSpec{
        double fCKMHz;
        double tCK;
        int64_t tREFIab;
        int64_t tREFIpb;
        int64_t tRFCab;
        int64_t tRFCpb;
        int64_t tRAS;
        int64_t tRPab;
        int64_t tRPpb;
        int64_t tRC;
        int64_t tPPD;
        int64_t tRCD;
        int64_t tFAW;
        int64_t tRRD;
        int64_t tCCD;
        int64_t tCCDMW;
        int64_t tRL;
        int64_t tDQSCK;
        int64_t tRTP;
        int64_t tWL;
        int64_t tWR;
        int64_t tWTR;
        int64_t tXP;
        int64_t tSR;
        int64_t tXSR;
        int64_t tESCKE;
        int64_t tCKE;
    };
    // Currents and Voltages:
    // Currents and Voltages:
    struct MemPowerSpec{
     //The "X" corresponds to the different voltage domains
     //Vdd1,Vdd2 or VddQ
     double iDD0X;
     double iDD2PX;
     double iDD2PSX;
     double iDD2NX;
     double iDD2NSX;
     double iDD3PX;
     double iDD3PSX;
     double iDD3NX;
     double iDD3NSX;
     double iDD4RX;
     double iDD4WX;
     double iDD5X;
     double iDD5ABX;
     double iDD5PBX;
     double iDD5PBX_BST;
     double iDD6X;
     double vDDX;
    };

    double capacitance;
    double ioPower;
    double wrOdtPower;
    double termRdPower;
    double termWrPower;

    struct BankWiseParams{
        // ACT Standby power factor
        int64_t bwPowerFactRho;
        // Self-Refresh power factor
        int64_t bwPowerFactSigma;
    };

    //TODO: PASR - Bank Masking and Segment Masking
    MemTimingSpec memTimingSpec;
    std::vector<MemPowerSpec> memPowerSpec;
    BankWiseParams bwParams;
};
}
#endif // MEMSPECLPDDR4_H

