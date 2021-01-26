/*
 * Copyright (c) 2012-2014, TU Delft
 * Copyright (c) 2012-2014, TU Eindhoven
 * Copyright (c) 2012-2020, TU Kaiserslautern
 * Copyright (c) 2012-2021, Fraunhofer IESE
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Matthias Jung
 *          Omar Naji
 *          Subash Kannoth
 *          Éder F. Zulian
 *          Felipe S. Prado
 *          Luiza Correa
 *
 */

#ifndef DRAM_POWER_IF_H
#define DRAM_POWER_IF_H

#include <stdint.h>
#include <vector>

#include "../MemCommand.h"
#include "../DebugManager.h"

namespace DRAMPower{
class DRAMPowerIF{
public:
    virtual ~DRAMPowerIF(){}

    virtual void doCommand(int64_t                timestamp,
                           DRAMPower::MemCommand::cmds type,
                           int                    rank,
                           int                    bank)=0;

    virtual void setupDebugManager(const bool debug __attribute__((unused))=false,
                                   const bool writeToConsole __attribute__((unused))=false,
                                   const bool writeToFile __attribute__((unused))=false,
                                   const std::string &traceName __attribute__((unused))="")=0;

    virtual void calcEnergy() = 0;

    virtual void calcWindowEnergy(int64_t timestamp) = 0;

    virtual double getEnergy() = 0;

    virtual  double getPower() = 0;

    virtual void powerPrint() = 0;

    virtual void clearCountersWrapper() = 0;

    // list of all commands
    std::vector<DRAMPower::MemCommand> cmdList;
};
}
#endif // ifndef LIB_DRAM_POWER_H
