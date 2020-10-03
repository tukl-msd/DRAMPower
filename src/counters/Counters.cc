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

#include "Counters.h"

using namespace DRAMPower;
using namespace std;



// function to clear counters
void Counters::clearCounters(const int64_t timestamp)
{
  std::fill(numberofactsBanks.begin(), numberofactsBanks.end(), 0);
  std::fill(numberofpresBanks.begin(), numberofpresBanks.end(), 0);
  std::fill(numberofreadsBanks.begin(), numberofreadsBanks.end(), 0);
  std::fill(numberofwritesBanks.begin(), numberofwritesBanks.end(), 0);
  std::fill(actcyclesBanks.begin(), actcyclesBanks.end(), 0);

  numberofrefs        = 0;
  f_act_pdns          = 0;
  s_act_pdns          = 0;
  f_pre_pdns          = 0;
  s_pre_pdns          = 0;
  numberofsrefs       = 0;

  actcycles           = 0;
  precycles           = 0;
  f_act_pdcycles      = 0;
  s_act_pdcycles      = 0;
  f_pre_pdcycles      = 0;
  s_pre_pdcycles      = 0;
  pup_act_cycles      = 0;
  pup_pre_cycles      = 0;
  sref_cycles         = 0;
  spup_cycles         = 0;
  sref_ref_act_cycles = 0;
  sref_ref_pre_cycles = 0;
  spup_ref_act_cycles = 0;
  spup_ref_pre_cycles = 0;
  idlecycles_act      = 0;
  idlecycles_pre      = 0;

  // reset count references to timestamp so that they are moved
  // to start of next stats generation
  std::fill(first_act_cycle_banks.begin(), first_act_cycle_banks.end(), timestamp);
  first_act_cycle     = timestamp;

  pdn_cycle           = timestamp;
  sref_cycle_window   = timestamp;

  end_act_op          = timestamp;
  end_read_op         = timestamp;
  end_write_op        = timestamp;

  latest_read_cycle   = -1;
  latest_write_cycle  = -1;

  if (timestamp == 0) {
    latest_pre_cycle = -1;
    latest_act_cycle = -1;
    sref_cycle = 0;
    last_pre_cycle = 0;
    sref_ref_act_cycles_window = 0;
    sref_ref_pre_cycles_window = 0;
  } else {
    last_pre_cycle = max(timestamp,last_pre_cycle);

    latest_pre_cycle = max(timestamp, latest_pre_cycle);

    if (latest_act_cycle < timestamp)
        latest_act_cycle = -1;
  }
}

// function to clear all arrays
void Counters::clear()
{
  cached_cmd.clear();
  cmd_list.clear();
  last_bank_state.clear();
  bank_state.clear();
}

// Reads through the trace file, identifies the timestamp, command and bank
// If the issued command includes an auto-precharge, adds an explicit
// precharge to a cached command list and computes the precharge offset from the
// issued command timestamp, when the auto-precharge would kick in

void Counters::getCommands(std::vector<MemCommand> &, bool, int64_t)
{
 throw std::invalid_argument("getCommands method must be implemented");
} // Counters::getCommands




// To update idle period information whenever active cycles may be idle
void Counters::idle_act_update(int64_t, int64_t, int64_t, int64_t)
{
    throw std::invalid_argument("idle_act_update method must be implemented");
} // Counters::idle_act_update

// To update idle period information whenever precharged cycles may be idle
void Counters::idle_pre_update(int64_t, int64_t)
{
    throw std::invalid_argument("idle_pre_update method must be implemented");
}


////////////////////HELPERS/////////////////////////

// Returns the number of active banks based on the bank_state vector.
unsigned Counters::get_num_active_banks(void)
{
  return (unsigned)std::count(bank_state.begin(), bank_state.end(), BANK_ACTIVE);
}

// Naming-standard compliant wrapper
unsigned Counters::nActiveBanks(void)
{
  return Counters::get_num_active_banks();
}

bool Counters::isPrecharged(unsigned bank)
{
    return bank_state[bank] == BANK_PRECHARGED;
}

void Counters::printWarningIfActive(const string& warning, int type, int64_t timestamp, unsigned bank)
{
  if (get_num_active_banks() != 0) {
    printWarning(warning, type, timestamp, bank);
  }
}

void Counters::printWarningIfNotActive(const string& warning, int type, int64_t timestamp, unsigned bank)
{
  if (get_num_active_banks() == 0) {
    printWarning(warning, type, timestamp, bank);
  }
}

void Counters::printWarningIfPoweredDown(const string& warning, int type, int64_t timestamp, unsigned bank)
{
  if (mem_state != 0) {
    printWarning(warning, type, timestamp, bank);
  }
}

void Counters::printWarning(const string& warning, int type, int64_t timestamp, unsigned bank)
{
  cerr << "WARNING: " << warning << endl;
  cerr << "Command: " << type << ", Timestamp: " << timestamp <<
    ", Bank: " << bank << endl;
}


////////////////////HANDLERS////////////////////////

int64_t Counters::zero_guard(int64_t cycles_in, const char* warning)
{
  // Calculate max(0, cycles_in)
  int64_t zero = 0;
  if (warning != nullptr && cycles_in < 0) {
    // This line is commented out for now, we will attempt to remove the situations where
    // these warnings trigger later.
    // cerr << "WARNING: " << warning << endl;
  }
  return max(zero, cycles_in);
}

void Counters::handleAct(unsigned bank, int64_t timestamp)
{
  printWarningIfPoweredDown("Command issued while in power-down mode.", MemCommand::ACT, timestamp, bank);
  // If command is ACT - update number of acts, bank state of the
  // target bank, first and latest activation cycle and the memory
  // state. Update the number of precharged/idle-precharged cycles.
  // If the bank is already active ignore the command and generate a
  // warning.
  if (isPrecharged(bank)) {
    numberofactsBanks[bank]++;

    if (nActiveBanks() == 0) {
      // Here a memory state transition to ACT is happening. Save the
      // number of cycles in precharge state (increment the counter).
      first_act_cycle = timestamp;
      precycles += zero_guard(timestamp - last_pre_cycle, "1 last_pre_cycle is in the future.");
      idle_pre_update(timestamp, latest_pre_cycle);
    }

    bank_state[bank] = BANK_ACTIVE;
    latest_act_cycle = timestamp;
  } else {
    printWarning("Bank is already active!", MemCommand::ACT, timestamp, bank);
  }
}

void Counters::handleRd(unsigned bank, int64_t timestamp)
{
  printWarningIfPoweredDown("Command issued while in power-down mode.", MemCommand::RD, timestamp, bank);
  // If command is RD - update number of reads and read cycle. Check
  // for active idle cycles (if any).
  if (isPrecharged(bank)) {
    printWarning("Bank is not active!", MemCommand::RD, timestamp, bank);
  }
  numberofreadsBanks[bank]++;
  idle_act_update(latest_read_cycle, latest_write_cycle, latest_act_cycle, timestamp);
  latest_read_cycle = timestamp;
}

void Counters::handleWr(unsigned bank, int64_t timestamp)
{
  printWarningIfPoweredDown("Command issued while in power-down mode.", MemCommand::WR, timestamp, bank);
  // If command is WR - update number of writes and write cycle. Check
  // for active idle cycles (if any).
  if (isPrecharged(bank)) {
    printWarning("Bank is not active!", MemCommand::WR, timestamp, bank);
  }
  numberofwritesBanks[bank]++;
  idle_act_update(latest_read_cycle, latest_write_cycle, latest_act_cycle, timestamp);
  latest_write_cycle = timestamp;
}

void Counters::handleRef(unsigned /*bank*/, int64_t /*timestamp*/)
{
    throw std::invalid_argument("handleRef method must be implemented");
}

void Counters::handleRefB(unsigned /*bank*/, int64_t /*timestamp*/)
{
    throw std::invalid_argument("handleRefB method must be implemented or does not exist for memory type");
}

void Counters::handlePre(unsigned bank, int64_t timestamp)
{
  printWarningIfPoweredDown("Command issued while in power-down mode.", MemCommand::PRE, timestamp, bank);
  // If command is explicit PRE - update number of precharges, bank
  // state of the target bank and last and latest precharge cycle.
  // Calculate the number of active cycles if the memory was in the
  // active state before, but there is a state transition to PRE now
  // (i.e., this is the last active bank).
  // If the bank is already precharged ignore the command and generate a
  // warning.

  // Precharge only if the target bank is active
  if (bank_state[bank] == BANK_ACTIVE) {
    numberofpresBanks[bank]++;
    actcyclesBanks[bank] += zero_guard(timestamp - first_act_cycle_banks[bank], "first_act_cycle is in the future (bank).");
    // Since we got here, at least one bank is active
    assert(nActiveBanks() != 0);

    if (nActiveBanks() == 1) {
      // This is the last active bank. Therefore, here a memory state
      // transition to PRE is happening. Let's increment the active cycle
      // counter.
      actcycles += zero_guard(timestamp - first_act_cycle, "first_act_cycle is in the future.");
      last_pre_cycle = timestamp;
      idle_act_update(latest_read_cycle, latest_write_cycle, latest_act_cycle, timestamp);
    }

    bank_state[bank] = BANK_PRECHARGED;
    latest_pre_cycle = timestamp;
  } else {
    printWarning("Bank is already precharged!", MemCommand::PRE, timestamp, bank);
  }
}

void Counters::handlePreA(unsigned bank, int64_t timestamp)
{
  printWarningIfPoweredDown("Command issued while in power-down mode.", MemCommand::PREA, timestamp, bank);
  // If command is explicit PREA (precharge all banks) - update
  // number of precharges by the number of active banks, update the bank
  // state of all banks to PRE and set the precharge cycle (the cycle in
  // which the memory state changes from ACT to PRE, aka last_pre_cycle).
  // Calculate the number of active cycles if the memory was in the
  // active state before, but there is a state transition to PRE now.

  if (nActiveBanks() > 0) {
    // Active banks are being precharged
    // At least one bank was active, therefore the current memory state is
    // ACT. Since all banks are being precharged a memory state transition
    // to PRE is happening. Add to the counter the amount of cycles the
    // memory remained in the ACT state.

    actcycles += zero_guard(timestamp - first_act_cycle, "first_act_cycle is in the future.");
    last_pre_cycle = timestamp;

    for (unsigned b = 0; b < num_banks; b++) {
      if (bank_state[b] == BANK_ACTIVE) {
        // Active banks are being precharged
        numberofpresBanks[b] += 1;
        actcyclesBanks[b] += zero_guard(timestamp - first_act_cycle_banks[b], "first_act_cycle is in the future (bank).");
      }
    }

    idle_act_update(latest_read_cycle, latest_write_cycle, latest_act_cycle, timestamp);

    latest_pre_cycle = timestamp;
    // Reset the state for all banks to precharged.
    for (auto& bs : bank_state) {
      bs = BANK_PRECHARGED;
    }
  } else {
    printWarning("All banks are already precharged!", MemCommand::PREA, timestamp, bank);
  }
}

void Counters::handlePdnFAct(unsigned bank, int64_t timestamp)
{
  // If command is fast-exit active power-down - update number of
  // power-downs, set the power-down cycle and the memory mode to
  // fast-exit active power-down. Save states of all the banks from
  // the cycle before entering active power-down, to be returned to
  // after powering-up. Update active and active idle cycles.
  printWarningIfNotActive("All banks are precharged! Incorrect use of Active Power-Down.", MemCommand::PDN_F_ACT, timestamp, bank);
  f_act_pdns++;
  last_bank_state = bank_state;
  pdn_cycle  = timestamp;
  actcycles += zero_guard(timestamp - first_act_cycle, "first_act_cycle is in the future.");
  for (unsigned b = 0; b < num_banks; b++) {
    if (bank_state[b] == BANK_ACTIVE) {
      actcyclesBanks[b] += zero_guard(timestamp - first_act_cycle_banks[b], "first_act_cycle is in the future (bank).");
    }
  }
  idle_act_update(latest_read_cycle, latest_write_cycle, latest_act_cycle, timestamp);
  mem_state  = Counters::MS_PDN_F_ACT;
}


void Counters::handlePdnFPre(unsigned bank, int64_t timestamp)
{
  // If command is fast-exit precharged power-down - update number of
  // power-downs, set the power-down cycle and the memory mode to
  // fast-exit precahrged power-down. Update precharged and precharged
  // idle cycles.
  printWarningIfActive("One or more banks are active! Incorrect use of Precharged Power-Down.", MemCommand::PDN_F_PRE, timestamp, bank);
  f_pre_pdns++;
  pdn_cycle  = timestamp;
  precycles += zero_guard(timestamp - last_pre_cycle, "3 last_pre_cycle is in the future.");
  idle_pre_update(timestamp, latest_pre_cycle);
  mem_state  = Counters::MS_PDN_F_PRE;
}

void Counters::handlePdnSPre(unsigned bank, int64_t timestamp)
{
  // If command is slow-exit precharged power-down - update number of
  // power-downs, set the power-down cycle and the memory mode to
  // slow-exit precahrged power-down. Update precharged and precharged
  // idle cycles.
  printWarningIfActive("One or more banks are active! Incorrect use of Precharged Power-Down.",  MemCommand::PDN_S_PRE, timestamp, bank);
  s_pre_pdns++;
  pdn_cycle  = timestamp;
  precycles += zero_guard(timestamp - last_pre_cycle, "4 last_pre_cycle is in the future.");
  idle_pre_update(timestamp, latest_pre_cycle);
  mem_state  = Counters::MS_PDN_S_PRE;
}

void Counters::handlePupAct(int64_t /*timestamp*/)
{
    throw std::invalid_argument("handlePupAct method must be implemented");
}

void Counters::handlePupPre(int64_t /*timestamp*/)
{
    throw std::invalid_argument("handlePupPre method must be implemented");
}

void Counters::handleSREn(unsigned bank, int64_t timestamp)
{
  // If command is self-refresh - update number of self-refreshes,
  // set memory state to SREF, update precharge and idle precharge
  // cycles and set the self-refresh cycle.
  printWarningIfActive("One or more banks are active! SREF requires all banks to be precharged.", MemCommand::SREN, timestamp, bank);
  numberofsrefs++;
  sref_cycle = timestamp;
  sref_cycle_window = timestamp;
  sref_ref_pre_cycles_window = 0;
  sref_ref_act_cycles_window = 0;
  precycles += zero_guard(timestamp - last_pre_cycle, "5  last_pre_cycle is in the future.");
  idle_pre_update(timestamp, latest_pre_cycle);
  mem_state  = Counters::MS_SREF;
}

void Counters::handleSREx(unsigned /*bank*/, int64_t /*timestamp*/)
{
   throw std::invalid_argument("handleSREx method must be implemented");
}


void Counters::handleNopEnd(int64_t /*timestamp*/)
{
    throw std::invalid_argument("handleNopEnd method must be implemented");
}


