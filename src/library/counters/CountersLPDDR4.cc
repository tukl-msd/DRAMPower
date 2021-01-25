/*
 * Copyright (c) 2012-2014, TU Delft
 * Copyright (c) 2012-2014, TU Eindhoven
 * Copyright (c) 2012-2020, TU Kaiserslautern
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

#include "CountersLPDDR4.h"

using namespace DRAMPower;
using namespace std;

CountersLPDDR4::CountersLPDDR4(MemSpecLPDDR4& memspec) :
    memSpec(memspec)
{
    auto &nBanks = memSpec.numberOfBanks;
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


void CountersLPDDR4::getCommands(std::vector<MemCommand>& list, bool lastupdate, int64_t timestamp)
{
    int64_t prechargeOffset;
    if (!next_window_cmd_list.empty()) {
        list.insert(list.begin(), next_window_cmd_list.begin(), next_window_cmd_list.end());
        next_window_cmd_list.clear();
    }
    cout << list.size();
    for (size_t i = 0; i < list.size(); ++i) {
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
                                  activation_cycle[cmd.getBank()] + memSpec.memTimingSpec.tRAS);
            list.push_back(MemCommand(preTime, MemCommand::PRE, 0,  cmd.getBank()));
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
        list.push_back(MemCommand(t, MemCommand::NOP, 0, 0));
    }
} // CountersLPDDR4::getCommands




// To update idle period information whenever active cycles may be idle
void CountersLPDDR4::idle_act_update(int64_t latest_read_cycle, int64_t latest_write_cycle,
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
} // CountersLPDDR4::idle_act_update

// To update idle period information whenever precharged cycles may be idle
void CountersLPDDR4::idle_pre_update(int64_t timestamp, int64_t latest_pre_cycle)
{
    if (latest_pre_cycle > 0) {
        idlecycles_pre += max(zero, timestamp - latest_pre_cycle -
                              memSpec.memTimingSpec.tRPab);
    } else if (latest_pre_cycle == 0) {
        idlecycles_pre += max(zero, timestamp - latest_pre_cycle);
    }
}

void CountersLPDDR4::idle_pre_update_pb(int64_t timestamp, int64_t latest_pre_cycle)
{
    if (latest_pre_cycle > 0) {
        idlecycles_pre += max(zero, timestamp - latest_pre_cycle -
                              memSpec.memTimingSpec.tRPpb);
    } else if (latest_pre_cycle == 0) {
        idlecycles_pre += max(zero, timestamp - latest_pre_cycle);
    }
}

////////////////////HANDLERS////////////////////////

void CountersLPDDR4::handlePreA(unsigned bank, int64_t timestamp)
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
        // Active banks are being precharged
        numberofpreA += 1;
        for (unsigned b = 0; b < num_banks; b++) {
            if (bank_state[b] == BANK_ACTIVE) {
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
        PRINTDEBUGMESSAGE("All banks are already precharged!", timestamp, MemCommand::PREA, bank);
    }
}


void CountersLPDDR4::handleRef(unsigned bank, int64_t timestamp)
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
    last_pre_cycle   = timestamp + memSpec.memTimingSpec.tRFCab - memSpec.memTimingSpec.tRPab;
    latest_pre_cycle = last_pre_cycle;
    actcycles       += memSpec.memTimingSpec.tRFCab - memSpec.memTimingSpec.tRPab;
    for (auto &e : actcyclesBanks) {
        e += memSpec.memTimingSpec.tRFCab - memSpec.memTimingSpec.tRPab;
    }
    for (auto& bs : bank_state) {
        bs = BANK_PRECHARGED;
    }
}

void CountersLPDDR4::handleRefB(unsigned bank, int64_t timestamp)
{
  // A REFB command requires a previous PRE command.
    printWarningIfBankActive("Bank Active! REFB requires target bank to be precharged", MemCommand::REFB, timestamp, bank);
    numberofrefbBanks[bank]++;

    if (nActiveBanks() !=0) { //All banks precharged
        idle_pre_update_pb(timestamp, latest_pre_cycle);
        first_act_cycle  = timestamp;
        first_act_cycle_banks[bank] = timestamp;
        precycles       += zero_guard(timestamp - last_pre_cycle, "2 last_pre_cycle is in the future.");
        last_pre_cycle   = timestamp + memSpec.memTimingSpec.tRFCab - memSpec.memTimingSpec.tRPab;
        latest_pre_cycle = last_pre_cycle;
        actcycles       += memSpec.memTimingSpec.tRFCpb - memSpec.memTimingSpec.tRPpb;
        actcyclesBanks[bank] += memSpec.memTimingSpec.tRFCab - memSpec.memTimingSpec.tRPab;
        bank_state [bank] = BANK_PRECHARGED;
    }
    else { //At least one bank active.
        // Memory never goes to precharged state so "precycles" isn't updated
        first_act_cycle_banks[bank] = timestamp;
        //Bank active during tRFC-tRP and then back to precharged
        actcyclesBanks[bank] += memSpec.memTimingSpec.tRFCpb - memSpec.memTimingSpec.tRPpb;
        bank_state [bank] = BANK_PRECHARGED;
    }
}

void CountersLPDDR4::handlePupAct(int64_t timestamp)
{
    if (mem_state == Counters::MS_PDN_F_ACT) {
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

void CountersLPDDR4::handlePupPre(int64_t timestamp)
{
    // LPDDR4 always has fast exit
    if (mem_state == Counters::MS_PDN_F_PRE) {
        f_pre_pdcycles  += zero_guard(timestamp - pdn_cycle, "pdn_cycle is in the future.");
        pup_pre_cycles  += memSpec.memTimingSpec.tXP;
        latest_pre_cycle = timestamp;
    } else {
        cerr << "Incorrect use of Precharged Power-Up!" << endl;
    }
    mem_state      = MS_NOT_IN_PD;
    last_pre_cycle = timestamp;
}


void CountersLPDDR4::handleSREx(unsigned bank, int64_t timestamp)
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

    // The minimum time that the DRAM must remain in Self-Refresh is tSR.
    if (sref_duration < memSpec.memTimingSpec.tSR) {
        PRINTDEBUGMESSAGE("Self-Refresh duration < tSR!", timestamp, MemCommand::SREX, bank);
    }

    if (sref_duration >= memSpec.memTimingSpec.tRFCab) {
        /*
     * Self-refresh Exit Context 1 (tSREF >= tRFCab):
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
        sref_ref_act_cycles += memSpec.memTimingSpec.tRFCab -
                memSpec.memTimingSpec.tRPab - sref_ref_act_cycles_window;
        sref_ref_pre_cycles += memSpec.memTimingSpec.tRPab - sref_ref_pre_cycles_window;
        last_pre_cycle       = timestamp;

        // The IDD6 current is consumed for the time period spent in the
        // self-refresh mode, which excludes the time spent in finishing the
        // initial auto-refresh.
        if (sref_cycle_window > sref_cycle + memSpec.memTimingSpec.tRFCab) {
            sref_cycles += zero_guard(timestamp - sref_cycle_window, "sref_cycle_window is in the future.");
        } else {
            sref_cycles += zero_guard(timestamp - sref_cycle - memSpec.memTimingSpec.tRFCab, "sref_cycle - tRFC < 0");
        }

        // IDD2N current is consumed when exiting the self-refresh state.
        spup_cycles     += memSpec.getExitSREFtime();
        latest_pre_cycle = timestamp + zero_guard(memSpec.getExitSREFtime() - memSpec.memTimingSpec.tRPab, "exitSREFtime - tRP < 0");


    } else {
        // Self-refresh Exit Context 2 (tSREF < tRFC):
        // Exit self-refresh before the completion of the initial
        // auto-refresh.

        // Number of active cycles needed by an auto-refresh.
        int64_t ref_act_cycles = memSpec.memTimingSpec.tRFCab - memSpec.memTimingSpec.tRPab;

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
            int64_t spup_pre = memSpec.memTimingSpec.tRPab - pre_cycles;

            spup_ref_pre_cycles += spup_pre;

            last_pre_cycle       = timestamp + spup_pre;

            spup_cycles     += memSpec.getExitSREFtime() - spup_pre;
            latest_pre_cycle = timestamp + zero_guard(memSpec.getExitSREFtime() - spup_pre - memSpec.memTimingSpec.tRPab,
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

            int64_t spup_act = (memSpec.memTimingSpec.tRFCab - memSpec.memTimingSpec.tRPab) - sref_duration;

            spup_ref_act_cycles += spup_act;
            spup_ref_pre_cycles += memSpec.memTimingSpec.tRPab;

            last_pre_cycle       = timestamp + spup_act + memSpec.memTimingSpec.tRPab;

            spup_cycles     += memSpec.getExitSREFtime() - spup_act - memSpec.memTimingSpec.tRPab;
            latest_pre_cycle = timestamp + zero_guard(memSpec.getExitSREFtime() - spup_act - (2 * memSpec.memTimingSpec.tRPab),
                                                      "memSpec.exitSREFtime - spup_act - (2 * tRP) < 0");
        }
    }
    mem_state = MS_NOT_IN_PD;
}


void CountersLPDDR4::handleNopEnd(int64_t timestamp)
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
    } else if (mem_state == Counters::MS_PDN_F_PRE) {
        f_pre_pdcycles += zero_guard(timestamp - pdn_cycle, "pdn_cycle is in the future");
    } else if (mem_state == Counters::MS_SREF) {
        auto rfc_minus_rp = (memSpec.memTimingSpec.tRFCab - memSpec.memTimingSpec.tRPab);

        if (timestamp > sref_cycle + memSpec.memTimingSpec.tRFCab) {
            if (sref_cycle_window <= sref_cycle + rfc_minus_rp) {
                sref_ref_act_cycles += rfc_minus_rp - sref_ref_act_cycles_window;
                sref_ref_act_cycles_window = rfc_minus_rp;
                sref_cycle_window = sref_cycle + rfc_minus_rp;
            }
            if (sref_cycle_window <= sref_cycle + memSpec.memTimingSpec.tRFCab) {
                sref_ref_pre_cycles += memSpec.memTimingSpec.tRPab - sref_ref_pre_cycles_window;
                sref_ref_pre_cycles_window = memSpec.memTimingSpec.tRPab;
                sref_cycle_window = sref_cycle + memSpec.memTimingSpec.tRFCab;
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


