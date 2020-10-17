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

#ifndef MEMSPECDDR3_H
#define MEMSPECDDR3_H

#include "MemSpec.h"

class MemSpecDDR3 final : public MemSpec
{
public:
    MemSpecDDR3(json &memspec, const bool debug __attribute__((unused))=false,
                const bool writeToConsole __attribute__((unused))=false,
                const bool writeToFile __attribute__((unused))=false,
                const std::string &traceName __attribute__((unused))="");
    ~MemSpecDDR3() {}
    int64_t timeToCompletion(DRAMPower::MemCommand::cmds type);

    // Memspec Variables:
    struct MemTimingSpec{
     double fCKMHz;
     double tCK;
     int64_t tCKE;
     int64_t tPD;
     int64_t tCKESR;
     int64_t tRAS;
     int64_t tRC;
     int64_t tRCD;
     int64_t tRL;
     int64_t tRTP;
     int64_t tWL;
     int64_t tWR;
     int64_t tXP;
     int64_t tXS;
     int64_t tREFI;
     int64_t tRFC;
     int64_t tRP;
     int64_t tDQSCK;
     int64_t tCCD;
     int64_t tFAW;
     int64_t tRRD;
     int64_t tWTR;
     int64_t tXPDLL;
     int64_t tXSDLL;
     int64_t tAL;
     int64_t tACTPDEN;
     int64_t tPRPDEN;
     int64_t tREFPDEN;
     int64_t tRTRS;
    };

    // Currents and Voltages:
    struct MemPowerSpec{
     double iDD0;
     double iDD2N;
     double iDD3N;
     double iDD4R;
     double iDD4W;
     double iDD5;
     double iDD6;
     double vDD;
     double iDD2P0;
     double iDD2P1;
     double iDD3P0;
     double iDD3P1;

     double capacitance;
     double ioPower;
     double wrOdtPower;
     double termRdPower;
     double termWrPower;
    };

    struct BankWiseParams{
        // Set of possible PASR modes
        enum pasrModes{
          PASR_0,
          PASR_1,
          PASR_2,
          PASR_3,
          PASR_4,
          PASR_5,
          PASR_6,
          PASR_7
        };
        // List of active banks under the specified PASR mode
        std::vector<unsigned> activeBanks;
        // ACT Standby power factor
        int64_t bwPowerFactRho;
        // Self-Refresh power factor( true : Bankwise mode)
        int64_t bwPowerFactSigma;
        // Wherther PASR is enabled ( true : enabled )
        bool flgPASR;
        // PASR mode utilized (int 0-7)
        int64_t pasrMode;
        // Wheather bank is active in PASR
        bool isBankActiveInPasr(const unsigned bankIdx) const;

    };

    MemTimingSpec memTimingSpec;
    std::vector<MemPowerSpec> memPowerSpec;
    BankWiseParams bwParams;

    int64_t getExitSREFtime();


};

#endif // MEMSPECDDR3_H
