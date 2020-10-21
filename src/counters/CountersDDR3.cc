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
 * Authors: Karthik Chandrasekar,
 *          Matthias Jung,
 *          Omar Naji,
 *          Sven Goossens,
 *          Éder F. Zulian
 *          Subash Kannoth
 *          Felipe S. Prado
 *          Luiza Correa
 */

#include <fstream>
#include <algorithm> //max
#include <sstream>

#include "CountersDDR3.h"

using namespace DRAMPower;
using namespace std;


CountersDDR3::CountersDDR3(MemSpecDDR3& memspec) :
memSpec(memspec)
{
  auto &nBanks = memSpec.memArchSpec.numberOfBanks;
  // Initializing all counters and variables
  numberofactsBanks.assign(static_cast<size_t>(nBanks), 0);
  numberofpresBanks.assign(static_cast<size_t>(nBanks), 0);
  numberofreadsBanks.assign(static_cast<size_t>(nBanks), 0);
  numberofwritesBanks.assign(static_cast<size_t>(nBanks), 0);
  actcyclesBanks.assign(static_cast<size_t>(nBanks), 0);
  numberofrefbBanks.assign(static_cast<size_t>(nBanks), 0);

  first_act_cycle_banks.resize(static_cast<size_t>(nBanks), 0);

  clearCounters(0);
  zero = 0;

  bank_state.resize(static_cast<size_t>(nBanks), BANK_PRECHARGED);
  last_bank_state.resize(static_cast<size_t>(nBanks), BANK_PRECHARGED);
  mem_state  = MS_NOT_IN_PD;

  cmd_list.clear();
  cached_cmd.clear();
  activation_cycle.resize(static_cast<size_t>(nBanks), 0);
  num_banks = nBanks;
}


void CountersDDR3::getCommands(std::vector<MemCommand>& list, bool lastupdate, int64_t timestamp)
{
  int64_t prechargeOffset;
  if (!next_window_cmd_list.empty()) {
    list.insert(list.begin(), next_window_cmd_list.begin(), next_window_cmd_list.end());
    next_window_cmd_list.clear();
  }
  cout << list.size();
  for (size_t i = 0; i < list.size(); ++i) {
//    int64_t prechargeOffset;
    MemCommand& cmd = list[i];
    MemCommand::cmds cmdType = cmd.getType();
    if (cmdType == MemCommand::ACT) {
      activation_cycle[cmd.getBank()] = cmd.getTimeInt64();
    } else if (cmdType == MemCommand::RDA || cmdType == MemCommand::WRA) {
      // Remove auto-precharge flag from command
      cmd.setType(cmd.typeWithoutAutoPrechargeFlag());
      if (cmdType == MemCommand::RDA) prechargeOffset=memSpec.prechargeOffsetRD;
      else prechargeOffset=memSpec.prechargeOffsetWR;
      //Add the auto precharge to the list of cached_cmds
      int64_t preTime = max(cmd.getTimeInt64() + prechargeOffset,
                           activation_cycle[cmd.getBank()] + memSpec.memTimingSpec.tRAS); ///problemm!

      list.push_back(MemCommand(MemCommand::PRE, cmd.getBank(), preTime));
    }

    if ((!lastupdate)  && (timestamp > 0)) {
      if(cmd.getTimeInt64() > timestamp)
      {
          MemCommand nextWindowCmd = list[i];
          next_window_cmd_list.push_back(nextWindowCmd);
          list.erase(find(list.begin(), list.end(), cmd));
      }
    }
  }
  sort(list.begin(), list.end(), commandSorter);
  if (lastupdate && list.empty() == false) {
    // Add cycles at the end of the list
    int64_t t =  memSpec.timeToCompletion(list.back().getType()) + list.back().getTimeInt64() - 1;
    list.push_back(MemCommand(MemCommand::NOP, 0, t));
  }
} // CountersDDR3::getCommands




// To update idle period information whenever active cycles may be idle
void CountersDDR3::idle_act_update(int64_t latest_read_cycle, int64_t latest_write_cycle,
                                      int64_t latest_act_cycle, int64_t timestamp)
{
  if (latest_read_cycle >= 0) {
    end_read_op = latest_read_cycle +  memSpec.timeToCompletion(MemCommand::RD) - 1;
  }

  if (latest_write_cycle >= 0) {
    end_write_op = latest_write_cycle +  memSpec.timeToCompletion(MemCommand::WR) - 1;
  }

  if (latest_act_cycle >= 0) {
    end_act_op = latest_act_cycle +  memSpec.timeToCompletion(MemCommand::ACT) - 1;
  }

  idlecycles_act += max(zero, timestamp - max(max(end_read_op, end_write_op),
                                              end_act_op));
} // CountersDDR3::idle_act_update

// To update idle period information whenever precharged cycles may be idle
void CountersDDR3::idle_pre_update(int64_t timestamp, int64_t latest_pre_cycle)
{
  if (latest_pre_cycle > 0) {
    idlecycles_pre += max(zero, timestamp - latest_pre_cycle -
                          memSpec.memTimingSpec.tRP);
  } else if (latest_pre_cycle == 0) {
    idlecycles_pre += max(zero, timestamp - latest_pre_cycle);
  }
}



////////////////////HANDLERS////////////////////////


void CountersDDR3::handleRef(unsigned bank, int64_t timestamp)
{
  printWarningIfPoweredDown("Command issued while in power-down mode.", MemCommand::REF, timestamp, bank);
  // If command is REF - update number of refreshes, set bank state of
  // all banks to ACT, set the last PRE cycles at RFC-RP cycles from
  // timestamp, set the number of active cycles to RFC-RP and check
  // for active and precharged cycles and idle active and idle
  // precharged cycles before refresh. Change memory state to 0.
  printWarningIfActive("One or more banks are active! REF requires all banks to be precharged.", MemCommand::REF, timestamp, bank);
  numberofrefs++;
  idle_pre_update(timestamp, latest_pre_cycle);
  first_act_cycle  = timestamp;
  std::fill(first_act_cycle_banks.begin(), first_act_cycle_banks.end(), timestamp);
  precycles       += zero_guard(timestamp - last_pre_cycle, "2 last_pre_cycle is in the future.");
  last_pre_cycle   = timestamp + memSpec.memTimingSpec.tRFC - memSpec.memTimingSpec.tRP;
  latest_pre_cycle = last_pre_cycle;
  actcycles       += memSpec.memTimingSpec.tRFC - memSpec.memTimingSpec.tRP;
  for (auto &e : actcyclesBanks) {
    e += memSpec.memTimingSpec.tRFC - memSpec.memTimingSpec.tRP;
  }
  for (auto& bs : bank_state) {
    bs = BANK_PRECHARGED;
  }
}



void CountersDDR3::handlePupAct(int64_t timestamp)
{
  // Command power-up in the active mode is always fast.

  if ((mem_state == Counters::MS_PDN_F_ACT) /*| (mem_state == Counters::MS_PDN_S_ACT)*/) {
    f_act_pdcycles  += zero_guard(timestamp - pdn_cycle, "pdn_cycle is in the future.");
    pup_act_cycles  += memSpec.memTimingSpec.tXP;
    latest_act_cycle = timestamp;
  }
  else {
    cerr << "Incorrect use of Active Power-Up!" << endl;
  }
  mem_state = MS_NOT_IN_PD;
  bank_state = last_bank_state;
  first_act_cycle = timestamp;
  std::fill(first_act_cycle_banks.begin(), first_act_cycle_banks.end(), timestamp);
}

void CountersDDR3::handlePupPre(int64_t timestamp)
{
  // If command is power-up in the precharged mode - check the power-down
  // exit-mode employed (fast or slow), update the number of power-down
  // and power-up cycles and the latest and last pre cycle.

  if (mem_state == Counters::MS_PDN_F_PRE) {
    f_pre_pdcycles  += zero_guard(timestamp - pdn_cycle, "pdn_cycle is in the future.");
    pup_pre_cycles  += memSpec.memTimingSpec.tXP;
    latest_pre_cycle = timestamp;
  } else if (mem_state == Counters::MS_PDN_S_PRE) { //Only memories with DLL can have slow exit
    s_pre_pdcycles += zero_guard(timestamp - pdn_cycle, "pdn_cycle is in the future.");
    pup_pre_cycles  += memSpec.getExitSREFtime();
    latest_pre_cycle = timestamp + zero_guard(memSpec.getExitSREFtime() - memSpec.memTimingSpec.tRP, "tXPDLL - tRCD - tRP");
  } else {
    cerr << "Incorrect use of Precharged Power-Up!" << endl;
  }
  mem_state      = MS_NOT_IN_PD;
  last_pre_cycle = timestamp;
}


void CountersDDR3::handleSREx(unsigned bank, int64_t timestamp)
{
  // If command is self-refresh exit - update the number of self-refresh
  // clock cycles, number of active and precharged auto-refresh clock
  // cycles during self-refresh and self-refresh exit based on the number
  // of cycles in the self-refresh mode and auto-refresh duration (RFC).
  // Set the last and latest precharge cycle accordingly and set the
  // memory state to 0.
  if (mem_state != Counters::MS_SREF) {
    cerr << "Incorrect use of Self-Refresh Power-Up!" << endl;
  }
  // The total duration of self-refresh is given by the difference between
  // the current clock cycle and the clock cycle of entering self-refresh.
  int64_t sref_duration = timestamp - sref_cycle;

  // Negative or zero duration should never happen.
  if (sref_duration <= 0) {
    PRINTDEBUGMESSAGE("Invalid Self-Refresh duration", timestamp, MemCommand::SREX, bank);
    sref_duration = 0;
  }

  // The minimum time that the DRAM must remain in Self-Refresh is CKESR.
  if (sref_duration < memSpec.memTimingSpec.tCKESR) {
    PRINTDEBUGMESSAGE("Self-Refresh duration < CKESR!", timestamp, MemCommand::SREX, bank);
  }

  if (sref_duration >= memSpec.memTimingSpec.tRFC) {
    /*
     * Self-refresh Exit Context 1 (tSREF >= tRFC):
     * The memory remained in self-refresh for a certain number of clock
     * cycles greater than a refresh cycle time (RFC). Consequently, the
     * initial auto-refresh accomplished.
     *
     *
     *  SREN                                #              SREX
     *  |                                   #                ^
     *  |                                   #                |
     *  |<------------------------- tSREF ----------...----->|
     *  |                                   #                |
     *  |      Initial Auto-Refresh         #                |
     *  v                                   #                |
     *  ------------------------------------#-------...-----------------> t
     *                                      #
     *   <------------- tRFC -------------->#
     *   <---- (tRFC - tRP) ----><-- tRP -->#
     *               |                |
     *               v                v
     *     sref_ref_act_cycles     sref_ref_pre_cycles
     *
     *
     * Summary:
     * sref_cycles += tSREF – tRFC
     * sref_ref_act_cycles += tRFC - tRP
     * sref_ref_pre_cycles += tRP
     * spup_ref_act_cycles += 0
     * spup_ref_pre_cycles += 0
     *
     */

    // The initial auto-refresh consumes (IDD5 − IDD3N) over one refresh
    // period (RFC) from the start of the self-refresh.
    sref_ref_act_cycles += memSpec.memTimingSpec.tRFC -
                           memSpec.memTimingSpec.tRP - sref_ref_act_cycles_window;
    sref_ref_pre_cycles += memSpec.memTimingSpec.tRP - sref_ref_pre_cycles_window;
    last_pre_cycle       = timestamp;

    // The IDD6 current is consumed for the time period spent in the
    // self-refresh mode, which excludes the time spent in finishing the
    // initial auto-refresh.
    if (sref_cycle_window > sref_cycle + memSpec.memTimingSpec.tRFC) {
        sref_cycles += zero_guard(timestamp - sref_cycle_window, "sref_cycle_window is in the future.");
    } else {
        sref_cycles += zero_guard(timestamp - sref_cycle - memSpec.memTimingSpec.tRFC, "sref_cycle - tRFC < 0");
    }

    // IDD2N current is consumed when exiting the self-refresh state.
      spup_cycles     += memSpec.getExitSREFtime();
      latest_pre_cycle = timestamp + zero_guard(memSpec.getExitSREFtime() - memSpec.memTimingSpec.tRP, "exitSREFtime - tRP < 0");


  } else {
    // Self-refresh Exit Context 2 (tSREF < tRFC):
    // Exit self-refresh before the completion of the initial
    // auto-refresh.

    // Number of active cycles needed by an auto-refresh.
    int64_t ref_act_cycles = memSpec.memTimingSpec.tRFC - memSpec.memTimingSpec.tRP;

    if (sref_duration >= ref_act_cycles) {
      /*
       * Self-refresh Exit Context 2A (tSREF < tRFC && tSREF >= tRFC - tRP):
       * The duration of self-refresh is equal or greater than the number
       * of active cycles needed by the initial auto-refresh.
       *
       *
       *  SREN                                           SREX
       *  |                                                ^         #
       *  |                                                |         #
       *  |<------------------ tSREF --------------------->|         #
       *  |                                                |         #
       *  |                                  Initial Auto-Refresh    #
       *  v                                                |         #
       *  -----------------------------------------------------------#--> t
       *                                                             #
       *   <------------------------ tRFC -------------------------->#
       *   <------------- (tRFC - tRP)--------------><----- tRP ---->#
       *           |                                 <-----><------->
       *           v                                  |         |
       *     sref_ref_act_cycles                      v         v
       *                             sref_ref_pre_cycles spup_ref_pre_cycles
       *
       *
       * Summary:
       * sref_cycles += 0
       * sref_ref_act_cycles += tRFC - tRP
       * sref_ref_pre_cycles += tSREF – (tRFC – tRP)
       * spup_ref_act_cycles += 0
       * spup_ref_pre_cycles += tRP – sref_ref_pre_cycles
       *
       */

      // Number of precharged cycles (zero <= pre_cycles < RP)
      int64_t pre_cycles = sref_duration - ref_act_cycles - sref_ref_pre_cycles_window;

      sref_ref_act_cycles += ref_act_cycles - sref_ref_act_cycles_window;
      sref_ref_pre_cycles += pre_cycles;

      // Number of precharged cycles during the self-refresh power-up. It
      // is at maximum tRP (if pre_cycles is zero).
      int64_t spup_pre = memSpec.memTimingSpec.tRP - pre_cycles;

      spup_ref_pre_cycles += spup_pre;

      last_pre_cycle       = timestamp + spup_pre;

        spup_cycles     += memSpec.getExitSREFtime() - spup_pre;
        latest_pre_cycle = timestamp + zero_guard(memSpec.getExitSREFtime() - spup_pre - memSpec.memTimingSpec.tRP,
                                                  "exitSREFtime - spup_pre - tRP < 0");
    } else {
      /*
       * Self-refresh Exit Context 2B (tSREF < tRFC - tRP):
       * self-refresh duration is shorter than the number of active cycles
       * needed by the initial auto-refresh.
       *
       *
       *  SREN                             SREX
       *  |                                  ^                        #
       *  |                                  |                        #
       *  |<-------------- tSREF ----------->|                        #
       *  |                                  |                        #
       *  |                       Initial Auto-Refresh                #
       *  v                                  |                        #
       *  ------------------------------------------------------------#--> t
       *                                                              #
       *   <------------------------ tRFC --------------------------->#
       *   <-------------- (tRFC - tRP)-------------><------ tRP ---->#
       *   <--------------------------------><------><--------------->
       *               |                        |             |
       *               v                        v             v
       *     sref_ref_act_cycles    spup_ref_act_cycles spup_ref_pre_cycles
       *
       *
       * Summary:
       * sref_cycles += 0
       * sref_ref_act_cycles += tSREF
       * sref_ref_pre_cycles += 0
       * spup_ref_act_cycles += (tRFC – tRP) - tSREF
       * spup_ref_pre_cycles += tRP
       *
       */

      sref_ref_act_cycles += sref_duration - sref_ref_act_cycles_window;

      int64_t spup_act = (memSpec.memTimingSpec.tRFC - memSpec.memTimingSpec.tRP) - sref_duration;

      spup_ref_act_cycles += spup_act;
      spup_ref_pre_cycles += memSpec.memTimingSpec.tRP;

      last_pre_cycle       = timestamp + spup_act + memSpec.memTimingSpec.tRP;

      spup_cycles     += memSpec.getExitSREFtime() - spup_act - memSpec.memTimingSpec.tRP;
      latest_pre_cycle = timestamp + zero_guard(memSpec.getExitSREFtime() - spup_act - (2 * memSpec.memTimingSpec.tRP),
                                                  "memSpec.exitSREFtime - spup_act - (2 * tRP) < 0");
    }
  }
  mem_state = MS_NOT_IN_PD;
}


void CountersDDR3::handleNopEnd(int64_t timestamp)
{
  // May be optionally used at the end of memory trace for better accuracy
  // Update all counters based on completion of operations.

  for (unsigned b = 0; b < num_banks; b++) {
    if (bank_state[b] == BANK_ACTIVE) {
      actcyclesBanks[b] += zero_guard(timestamp - first_act_cycle_banks[b], "first_act_cycle is in the future (bank)");
    }
  }

  if (nActiveBanks() > 0 && mem_state == MS_NOT_IN_PD) {
    actcycles += zero_guard(timestamp - first_act_cycle, "first_act_cycle is in the future");
    idle_act_update(latest_read_cycle, latest_write_cycle,
                    latest_act_cycle, timestamp);
  } else if (nActiveBanks() == 0 && mem_state == MS_NOT_IN_PD) {
    precycles += zero_guard(timestamp - last_pre_cycle, "6 last_pre_cycle is in the future");
    idle_pre_update(timestamp, latest_pre_cycle);
  } else if (mem_state == Counters::MS_PDN_F_ACT) {
    f_act_pdcycles += zero_guard(timestamp - pdn_cycle, "pdn_cycle is in the future");
  } else if (mem_state == Counters::MS_PDN_S_ACT) {
    s_act_pdcycles += zero_guard(timestamp - pdn_cycle, "pdn_cycle is in the future");
  } else if (mem_state == Counters::MS_PDN_F_PRE) {
    f_pre_pdcycles += zero_guard(timestamp - pdn_cycle, "pdn_cycle is in the future");
  } else if (mem_state == Counters::MS_PDN_S_PRE) {
    s_pre_pdcycles += zero_guard(timestamp - pdn_cycle, "pdn_cycle is in the future");
  } else if (mem_state == Counters::MS_SREF) {
    auto rfc_minus_rp = (memSpec.memTimingSpec.tRFC - memSpec.memTimingSpec.tRP);

    if (timestamp > sref_cycle + memSpec.memTimingSpec.tRFC) {
      if (sref_cycle_window <= sref_cycle + rfc_minus_rp) {
        sref_ref_act_cycles += rfc_minus_rp - sref_ref_act_cycles_window;
        sref_ref_act_cycles_window = rfc_minus_rp;
        sref_cycle_window = sref_cycle + rfc_minus_rp;
      }
      if (sref_cycle_window <= sref_cycle + memSpec.memTimingSpec.tRFC) {
        sref_ref_pre_cycles += memSpec.memTimingSpec.tRP - sref_ref_pre_cycles_window;
        sref_ref_pre_cycles_window = memSpec.memTimingSpec.tRP;
        sref_cycle_window = sref_cycle + memSpec.memTimingSpec.tRFC;
      }
      sref_cycles += zero_guard(timestamp - sref_cycle_window, "sref_cycle_window is in the future");
    } else if (timestamp > sref_cycle + rfc_minus_rp) {

      if (sref_cycle_window <= sref_cycle + rfc_minus_rp) {
        sref_ref_act_cycles += rfc_minus_rp - sref_ref_act_cycles_window;
        sref_ref_act_cycles_window = rfc_minus_rp;
        sref_cycle_window = sref_cycle + rfc_minus_rp;
      }
      sref_ref_pre_cycles_window += timestamp - sref_cycle_window;
      sref_ref_pre_cycles += timestamp - sref_cycle_window;
    } else {
      sref_ref_act_cycles_window += timestamp - sref_cycle_window;
      sref_ref_act_cycles += timestamp - sref_cycle_window;
    }
  }
}


