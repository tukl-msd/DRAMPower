/*
 * Copyright (c) 2014, TU Delft, TU Eindhoven and TU Kaiserslautern 
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
 * Authors: Matthias Jung, Omar Naji
 *
 */


#ifndef LIB_DRAM_POWER_H
#define LIB_DRAM_POWER_H

#include <string>
#include <xercesc/sax2/DefaultHandler.hpp>
#include "CommandAnalysis.h"
#include "MemoryPowerModel.h"
#include "MemCommand.h"

using namespace std;
using namespace Data;

class libDRAMPower
{
    public:
    libDRAMPower();
    ~libDRAMPower();

    void doCommand(MemCommand::cmds type, unsigned bank, double timestamp);
    //takes as parameter an object of Memory Specification. The user of the
    //library is required to write a parser to set the parameters id,
    //memorytype, memArchSpec, memTimingSpec and memPowerSpec
    void getEnergy(const MemorySpecification& memSpec);
    std::vector<MemCommand> cmd_list;
    private:    
};

#endif
