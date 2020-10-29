/*
 * Copyright (c) 2019, University of Kaiserslautern
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
 */

#ifndef MemSpecWideIO_H
#define MemSpecWideIO_H

#include "MemSpec.h"

class MemSpecWideIO final : public MemSpec
{
public:
    MemSpecWideIO(json &memspec, const bool debug __attribute__((unused))=false,
                const bool writeToConsole __attribute__((unused))=false,
                const bool writeToFile __attribute__((unused))=false,
                const std::string &traceName __attribute__((unused))="");
    ~MemSpecWideIO() {}
    int64_t timeToCompletion(DRAMPower::MemCommand::cmds type);

    //ranks?
    unsigned numberOfChannels;

    // Memspec Variables:
    struct MemTimingSpec{
     double fCKMHz;
     double tCK;
     int64_t tCKE;
     int64_t tCKESR;
     int64_t tRAS;
     int64_t tRC;
     int64_t tRCD;
     int64_t tRL;
     int64_t tWL;
     int64_t tWR;
     int64_t tXP;
     int64_t tXSR;
     int64_t tREFI;
     int64_t tRFC;
     int64_t tRP;
     int64_t tDQSCK;
     int64_t tAC;
     int64_t tCCD_R;
     int64_t tCCD_W;
     int64_t tRRD;
     int64_t tTAW;
     int64_t tWTR;
     int64_t tRTRS;
     };

    // Currents and Voltages:
    struct MemPowerSpec{
     double iDD0X;
     double iDD2PX;
     double iDD2NX;
     double iDD3PX;
     double iDD3nX;
     double iDD4RX;
     double iDD4WX;
     double iDD5X;
     double iDD6X;
     double vDDX;

     double capacitance;
     double ioPower;
     double wrOdtPower;
     double termRdPower;
     double termWrPower;
    };

    struct BankWiseParams{
        // ACT Standby power factor
        int64_t bwPowerFactRho;
        // Self-Refresh power factor( true : Bankwise mode)
        int64_t bwPowerFactSigma;
    };

    MemTimingSpec memTimingSpec;
    std::vector<MemPowerSpec> memPowerSpec;
    BankWiseParams bwParams;

    int64_t getExitSREFtime();


};

#endif // MemSpecWideIO_H
