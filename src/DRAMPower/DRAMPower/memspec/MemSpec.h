/*
 * Copyright (c) 2015-2020, University of Kaiserslautern
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
 *    Janik Schlemminger
 *    Matthias Jung
 *    Lukas Steiner
 *    Luiza Correa
 */

#ifndef DRAMPOWER_MEMSPEC_MEMSPEC_H
#define DRAMPOWER_MEMSPEC_MEMSPEC_H

#include <DRAMPower/command/CmdType.h>

#include <DRAMUtils/util/json.h>
#include <DRAMUtils/memspec/MemSpec.h>

#include <vector>
#include <algorithm>


namespace DRAMPower {

template <typename T>
class MemSpec
{
    T rawmemspec;
public:
    uint64_t numberOfBanks;
	uint64_t numberOfRows;
	uint64_t numberOfColumns;
	uint64_t numberOfDevices;   // Number of devices per rank
	uint64_t burstLength;
	uint64_t dataRate;
	uint64_t bitWidth;

    /* MEMSPEC specific parameters
    unsigned numberOfBankGroups;
    unsigned numberOfDevicesOnDIMM;
    unsigned numberOfRanks;
    unsigned banksPerRank;
    unsigned groupsPerRank;
    unsigned banksPerGroup;
    unsigned numberOfChannels;
    */

    std::string memoryId;
    std::string memoryType;


    uint64_t prechargeOffsetRD;
    uint64_t prechargeOffsetWR;
    MemSpec() = delete;
    virtual ~MemSpec() = default;
    virtual uint64_t timeToCompletion(DRAMPower::CmdType type) = 0;

	// MemSpec() = default;
    MemSpec(const T &memspec) : rawmemspec(memspec)
    {
        numberOfBanks = memspec.memarchitecturespec.nbrOfBanks;
        numberOfRows = memspec.memarchitecturespec.nbrOfRows;
        numberOfColumns = memspec.memarchitecturespec.nbrOfColumns;
        numberOfDevices = memspec.memarchitecturespec.nbrOfDevices;
        if (numberOfDevices < 1)
        {
            numberOfDevices = 1;
        }
        burstLength = memspec.memarchitecturespec.burstLength;
        dataRate = memspec.memarchitecturespec.dataRate;
        bitWidth = memspec.memarchitecturespec.width;
        memoryId = memspec.memoryId;
        memoryType = memspec.id;
    }

};

} // namespace DRAMPower

#endif /* DRAMPOWER_MEMSPEC_MEMSPEC_H */
