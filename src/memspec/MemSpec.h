/*
 * Copyright (c) 2015, University of Kaiserslautern
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
 *    Janik Schlemminger
 *    Matthias Jung
 *    Lukas Steiner
 */

#ifndef MEMSPEC_H
#define MEMSPEC_H

#include <vector>
#include <algorithm>  // for max

#include "./jsonparser/JSONParser.h"
#include "./MemCommand.h"

class MemSpec
{
public:
    struct MemArchitectureSpec {
    unsigned numberOfChannels;
    unsigned numberOfRanks;
    unsigned banksPerRank;
    unsigned groupsPerRank;
    unsigned banksPerGroup;
    unsigned numberOfBanks;
    unsigned numberOfBankGroups;
    unsigned numberOfDevicesOnDIMM;
    unsigned numberOfRows;
    unsigned numberOfColumns;
    unsigned burstLength;
    unsigned dataRate;
    unsigned bitWidth;
    };


    std::string memoryId;
    std::string memoryType;


    int64_t prechargeOffsetRD;
    int64_t prechargeOffsetWR;

    MemArchitectureSpec memArchSpec;
    virtual ~MemSpec();
    virtual int64_t timeToCompletion(DRAMPower::MemCommand::cmds type) = 0;

    virtual int64_t getRAS(); //rename
    virtual int64_t getRP();
    virtual int64_t getRCD();
    virtual int64_t getRFC();
    virtual int64_t getXP();
    virtual int64_t getXPDLL();
    virtual int64_t getCKESR();
    virtual int64_t getExitSREFtime();

protected:
    MemSpec(nlohmann::json &memspec);

    bool parseBool(nlohmann::json &obj, std::string name);
    unsigned parseUint(nlohmann::json &obj, std::string name);
    double parseUdouble(nlohmann::json &obj, std::string name);
    double parseUdoubleWithDefault(nlohmann::json &obj, std::string name);
    std::string parseString(nlohmann::json &obj, std::string name);

};

#endif // MEMSPEC_H

