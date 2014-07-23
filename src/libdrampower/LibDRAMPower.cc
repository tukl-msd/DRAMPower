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


#include "LibDRAMPower.h"

#include <iostream>
#include <string>

using namespace std;

libDRAMPower::libDRAMPower(MemorySpecification memSpec, int grouping, int interleaving, int burst,
                           int term, int powerdown){
    MemSpec = memSpec;
    Grouping = grouping;
    Interleaving = interleaving;
    Burst = burst;
    Term = term;        
    Powerdown = powerdown;
    counters = CommandAnalysis(memSpec.memArchSpec.nbrOfBanks, memSpec);
}

libDRAMPower::~libDRAMPower(){
    
}

void libDRAMPower::doCommand(MemCommand::cmds type, unsigned bank, double timestamp){
    MemCommand cmd(type, bank, timestamp);
    list.push_back(cmd);
}

void libDRAMPower::updateCounters(bool lastupdate){
    
    counters.getCommands(MemSpec, MemSpec.memArchSpec.nbrOfBanks, list, lastupdate);
    list.clear();
}

void libDRAMPower::getEnergy(){
    
    counters.clear();
    mpm.power_calc(MemSpec, counters, Grouping, Interleaving, Burst, Term, Powerdown);

}
