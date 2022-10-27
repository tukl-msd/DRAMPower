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

#ifndef DRAMPOWER_MEMSPEC_MEMSPECDDR4_H
#define DRAMPOWER_MEMSPEC_MEMSPECDDR4_H

#include "MemSpec.h"



namespace DRAMPower {

class MemSpecDDR4 final : public MemSpec
{
public:

    enum VoltageDomain {
        VDD = 0,
        VPP = 1,
        VDDQ = 2
    };

public:
	MemSpecDDR4() = default;

    MemSpecDDR4(nlohmann::json &memspec);

	~MemSpecDDR4() = default;

    uint64_t timeToCompletion(CmdType type) override;

	unsigned numberOfBankGroups;
	unsigned numberOfRanks;

	// Memspec Variables:
	struct MemTimingSpec 
	{
		double fCKMHz;
		double tCK;
		uint64_t tRAS;
		uint64_t tRCD;
		uint64_t tRL;
		uint64_t tRTP;
		uint64_t tWL;
		uint64_t tWR;
		uint64_t tRFC;
		uint64_t tRP;
		uint64_t tAL;
        uint64_t tBurst;
	};

	// Currents and Voltages:
	struct MemPowerSpec 
	{
		double iXX0;
		double iXX2N;
		double iXX3N;
		double iXX4R;
		double iXX4W;
		double iXX5X;
		double iXX6N;
		double vXX;
		double iXX2P;
		double iXX3P;
        double iBeta;
	};


	struct BankWiseParams 
	{
        // ACT Standby power factor
        double bwPowerFactRho;
	};

    uint64_t refreshMode;
	MemTimingSpec memTimingSpec;
	std::vector<MemPowerSpec> memPowerSpec;
	BankWiseParams bwParams;
};

}
#endif /* DRAMPOWER_MEMSPEC_MEMSPECDDR4_H */
