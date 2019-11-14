/*
 * Copyright (c) 2012-2014, TU Delft
 * Copyright (c) 2012-2014, TU Eindhoven
 * Copyright (c) 2012-2014, TU Kaiserslautern
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
 *
 */

#include "LibDRAMPower.h"

using namespace DRAMPower;

libDRAMPower::libDRAMPower(const MemorySpecification& memSpec, bool includeIoAndTermination) :
  memSpec(memSpec),
  counters(memSpec),
  includeIoAndTermination(includeIoAndTermination),
  mpm(MemoryPowerModel())
{
    MemBankWiseParams p (100,100,false,0,false,static_cast<unsigned>(memSpec.memArchSpec.nbrOfBanks));
    libDRAMPower DRAMPower = libDRAMPower(memSpec, 0, p);
}

libDRAMPower::libDRAMPower(const MemorySpecification& memSpec, bool includeIoAndTermination, const DRAMPower::MemBankWiseParams& bwPowerParams) :
  memSpec(memSpec),
  counters(CommandAnalysis(memSpec)),
  includeIoAndTermination(includeIoAndTermination),
  bwPowerParams(bwPowerParams)
{
}

libDRAMPower::~libDRAMPower()
{
}

void libDRAMPower::doCommand(MemCommand::cmds type, int bank, int64_t timestamp)
{
  MemCommand cmd(type, static_cast<unsigned>(bank), timestamp);
  cmdList.push_back(cmd);
}

void libDRAMPower::updateCounters(bool lastUpdate, int64_t timestamp)
{
  counters.getCommands(cmdList, lastUpdate, timestamp);
  cmdList.clear();
}

void libDRAMPower::calcEnergy()
{
  updateCounters(true);
  mpm.power_calc(memSpec, counters, includeIoAndTermination, bwPowerParams);
}

void libDRAMPower::calcWindowEnergy(int64_t timestamp)
{
  doCommand(MemCommand::NOP, 0, timestamp);
  updateCounters(false, timestamp);
  mpm.power_calc(memSpec, counters, includeIoAndTermination, bwPowerParams);
  clearCounters(timestamp);
}


void libDRAMPower::clearState()
{
  counters.clear();
}

void libDRAMPower::clearCounters(int64_t timestamp)
{
  counters.clearStats(timestamp);
}

const DRAMPower::MemoryPowerModel::Energy& libDRAMPower::getEnergy() const
{
  return mpm.energy;
}

const DRAMPower::MemoryPowerModel::Power& libDRAMPower::getPower() const
{
  return mpm.power;
}
