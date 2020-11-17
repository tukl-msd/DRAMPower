/*
 * Copyright (c) 2012-2020, TU Delft
 * Copyright (c) 2012-2020, TU Eindhoven
 * Copyright (c) 2012-2020, TU Kaiserslautern
 * Copyright (c) 2012-2020, Fraunhofer IESE
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

#ifndef COMMAND_TIMINGS_H
#define COMMAND_TIMINGS_H

#include <stdint.h>

#include <vector>
#include <iostream>
#include <deque>
#include <string>

#include "../MemCommand.h"
#include "../memspec/MemSpecDDR3.h"
#include "../DebugManager.h"


namespace DRAMPower {
class Counters {
 public:
  // Power-Down and Self-refresh related memory states
  enum memstate {
    MS_NOT_IN_PD = 0, MS_PDN_F_ACT = 10, MS_PDN_S_ACT = 11, MS_PDN_F_PRE = 12,
    MS_PDN_S_PRE = 13, MS_SREF = 14
  };

  // Returns number of reads, writes, acts, pres and refs in the trace
  Counters(){}

  // Number of activate commands per bank
  std::vector<int64_t> numberofactsBanks;
  // Number of precharge commands per bank
  std::vector<int64_t> numberofpresBanks;
  // Number of reads commands per bank
  std::vector<int64_t> numberofreadsBanks;
  // Number of writes commands per bank
  std::vector<int64_t> numberofwritesBanks;
  // Number of refresh commands
  int64_t numberofrefs;
  // Number of bankwise refresh commands
  std::vector<int64_t> numberofrefbBanks;
  // Number of precharge cycles
  int64_t precycles;
  // Number of active cycles
  int64_t actcycles;
  std::vector<int64_t> actcyclesBanks;
  // Number of Idle cycles in the active state
  int64_t idlecycles_act;
  // Number of Idle cycles in the precharge state
  int64_t idlecycles_pre;
  // Number of fast-exit activate power-downs
  int64_t f_act_pdns;
  // Number of slow-exit activate power-downs
  int64_t s_act_pdns;
  // Number of fast-exit precharged power-downs
  int64_t f_pre_pdns;
  // Number of slow-exit activate power-downs
  int64_t s_pre_pdns;
  // Number of self-refresh commands
  int64_t numberofsrefs;
  // Number of clock cycles in fast-exit activate power-down mode
  int64_t f_act_pdcycles;
  // Number of clock cycles in slow-exit activate power-down mode
  int64_t s_act_pdcycles;
  // Number of clock cycles in fast-exit precharged power-down mode
  int64_t f_pre_pdcycles;
  // Number of clock cycles in slow-exit precharged power-down mode
  int64_t s_pre_pdcycles;
  // Number of clock cycles in self-refresh mode (excludes the initial
  // auto-refresh). During this time the current drawn is IDD6.
  int64_t sref_cycles;
  // Number of clock cycles in activate power-up mode
  int64_t pup_act_cycles;
  // Number of clock cycles in precharged power-up mode
  int64_t pup_pre_cycles;
  // Number of clock cycles in self-refresh power-up mode
  int64_t spup_cycles;

  // Number of active cycles for the initial auto-refresh when entering
  // self-refresh mode.
  int64_t sref_ref_act_cycles;
  // Number of active auto-refresh cycles in self-refresh mode already used to calculate the energy of the previous windows
  int64_t sref_ref_act_cycles_window;
  // Number of precharged auto-refresh cycles in self-refresh mode
  int64_t sref_ref_pre_cycles;
  // Number of precharged auto-refresh cycles in self-refresh mode already used to calculate the energy of the previous window
  int64_t sref_ref_pre_cycles_window;
  // Number of active auto-refresh cycles during self-refresh exit
  int64_t spup_ref_act_cycles;
  // Number of precharged auto-refresh cycles during self-refresh exit
  int64_t spup_ref_pre_cycles;

  static bool commandSorter(const DRAMPower::MemCommand& i, const DRAMPower::MemCommand& j);

  int64_t zero_guard(int64_t cycles_in, const char* warning);


  // function for clearing counters
  void clearCounters(const int64_t timestamp);

  // function for clearing arrays
  void clear();


  virtual void getCommands(std::vector<MemCommand>& /*list*/,
                           bool /*lastupdate*/,
                           int64_t /*timestamp*/);


  // Handlers for commands that are getting processed
  virtual void handleAct(    unsigned bank, int64_t timestamp);
  virtual void handleRd(     unsigned bank, int64_t timestamp);
  virtual void handleWr(     unsigned bank, int64_t timestamp);
  virtual void handleRef(    unsigned /*bank*/, int64_t /*timestamp*/);
  virtual void handleRefB(   unsigned /*bank*/, int64_t /*timestamp*/);
  virtual void handlePre(    unsigned bank, int64_t timestamp);
  virtual void handlePreA(   unsigned bank, int64_t timestamp);
  virtual void handlePdnFAct(unsigned bank, int64_t timestamp);
  virtual void handlePdnFPre(unsigned bank, int64_t timestamp);
  virtual void handlePdnSPre(unsigned bank, int64_t timestamp);
  virtual void handlePupAct( int64_t /*timestamp*/);
  virtual void handlePupPre( int64_t /*timestamp*/);
  virtual void handleSREn(   unsigned bank, int64_t timestamp);
  virtual void handleSREx(   unsigned /*bank*/, int64_t /*timestamp*/);
  virtual void handleNopEnd( int64_t /*timestamp*/);

  protected:
  // Possible bank states are precharged or active
  enum BankState {
    BANK_PRECHARGED = 0,
    BANK_ACTIVE
  };

  int64_t  zero;
  // Cached last read command from the file
  std::vector<MemCommand> cached_cmd;

  // Stores the memory commands for analysis
  std::vector<MemCommand> cmd_list;

  //Stores the memory commands for the next window
  std::vector<MemCommand> next_window_cmd_list;

  // To save states of the different banks, before entering active
  // power-down mode (slow/fast-exit).
  std::vector<BankState> last_bank_state;
  // Bank state vector
  std::vector<BankState> bank_state;

  std::vector<int64_t> activation_cycle;
  // To keep track of the last ACT cycle
  int64_t latest_act_cycle;
  // To keep track of the last PRE cycle
  int64_t latest_pre_cycle;
  // To keep track of the last READ cycle
  int64_t latest_read_cycle;
  // To keep track of the last WRITE cycle
  int64_t latest_write_cycle;

  // To calculate end of READ operation
  int64_t end_read_op;
  // To calculate end of WRITE operation
  int64_t end_write_op;
  // To calculate end of ACT operation
  int64_t end_act_op;

  // Clock cycle when self-refresh was issued
  int64_t sref_cycle;

  // Latest Self-Refresh clock cycle used to calculate the energy of the previous window
  int64_t sref_cycle_window;

  // Clock cycle when the latest power-down was issued
  int64_t pdn_cycle;

  // Memory State
  unsigned mem_state;

  int64_t num_banks;

  // Clock cycle of first activate command when memory state changes to ACT
  int64_t first_act_cycle;
  std::vector<int64_t> first_act_cycle_banks;

  // Clock cycle of last precharge command when memory state changes to PRE
  int64_t last_pre_cycle;


  // To update idle period information whenever active cycles may be idle
  virtual void idle_act_update(int64_t                     /*latest_read_cycle*/,
                               int64_t                     /*latest_write_cycle*/,
                               int64_t                     /*latest_act_cycle*/,
                               int64_t                     /*timestamp*/);

  // To update idle period information whenever precharged cycles may be idle
  virtual void idle_pre_update(int64_t                     /*timestamp*/,
                               int64_t                     /*latest_pre_cycle*/);

  // Returns the number of active banks according to the bank_state vector.
  unsigned get_num_active_banks(void);
  unsigned nActiveBanks(void);

  bool isPrecharged(unsigned bank);

  void printWarningIfActive(const std::string& warning, int type, int64_t timestamp, unsigned bank);
  void printWarningIfNotActive(const std::string& warning, int type, int64_t timestamp, unsigned bank);
  void printWarningIfPoweredDown(const std::string& warning, int type, int64_t timestamp, unsigned bank);
};
}
#endif // ifndef COMMAND_TIMINGS_H
