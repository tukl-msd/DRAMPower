/*
 * Copyright (c) 2012, TU Delft, TU Eindhoven and TU Kaiserslautern 
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
 * Authors: Karthik Chandrasekar
 *
 */

#ifndef COMMAND_TIMINGS_H
#define COMMAND_TIMINGS_H

#include <vector>
#include <iostream>
#include <deque>

#include "MemCommand.h"
#include "MemorySpecification.h"
#include "Utils.h"

namespace Data {

    class CommandAnalysis {
    public:

	//Power-Down and Self-refresh related memory states
        enum memstate {
            MS_PDN_F_ACT = 10, MS_PDN_S_ACT = 11, MS_PDN_F_PRE = 12,
            MS_PDN_S_PRE = 13, MS_SREF = 14
        };

        CommandAnalysis();

        //Returns number of reads, writes, acts, pres and refs in the trace
        CommandAnalysis(std::ifstream& pwr_trace, const int nbrofBanks,
                Data::MemorySpecification memSpec);

        //Number of commands to be considered in a single power estimation time window
        const static int ANALYSIS_WINDOW = MILLION;

        unsigned init;
        double zero;
        unsigned pop;

        //Number of activate commands
        double numberofacts;
        //Number of precharge commands
        double numberofpres;
        //Number of reads commands
        double numberofreads;
        //Number of writes commands
        double numberofwrites;
        //Number of refresh commands
        double numberofrefs;
        //Number of precharge cycles
        double precycles;
        //Number of active cycles
        double actcycles;
        //Number of Idle cycles in the active state
        double idlecycles_act;
        //Number of Idle cycles in the precharge state
        double idlecycles_pre;
        //Number of fast-exit activate power-downs
        double f_act_pdns;
        //Number of slow-exit activate power-downs
        double s_act_pdns;
        //Number of fast-exit precharged power-downs
        double f_pre_pdns;
        //Number of slow-exit activate power-downs
        double s_pre_pdns;
        //Number of self-refresh commands
        double numberofsrefs;
        //Number of clock cycles in fast-exit activate power-down mode
        double f_act_pdcycles;
        //Number of clock cycles in slow-exit activate power-down mode
        double s_act_pdcycles;
        //Number of clock cycles in fast-exit precharged power-down mode
        double f_pre_pdcycles;
        //Number of clock cycles in slow-exit precharged power-down mode
        double s_pre_pdcycles;
        //Number of clock cycles in self-refresh mode
        double sref_cycles;
        //Number of clock cycles in activate power-up mode
        double pup_act_cycles;
        //Number of clock cycles in precharged power-up mode
        double pup_pre_cycles;
        //Number of clock cycles in self-refresh power-up mode
        double spup_cycles;

        //Number of active auto-refresh cycles in self-refresh mode
        double sref_ref_act_cycles;
        //Number of precharged auto-refresh cycles in self-refresh mode
        double sref_ref_pre_cycles;
        //Number of active auto-refresh cycles during self-refresh exit
        double spup_ref_act_cycles;
        //Number of precharged auto-refresh cycles during self-refresh exit
        double spup_ref_pre_cycles;

        //Cached last read command from the file
        std::vector<MemCommand> cached_cmd;

        //Stores the memory commands for analysis
        std::vector<MemCommand> cmd_list;

        //Stores all memory commands for analysis
        std::vector<MemCommand> full_cmd_list;

        //To save states of the different banks, before entering active
        //power-down mode (slow/fast-exit).
        std::vector<int> last_states;

        //Bank state vector
        std::vector<int> bankstate;

        //To keep track of the last ACT cycle
        double latest_act_cycle;
        //To keep track of the last PRE cycle
        double latest_pre_cycle;
        //To keep track of the last READ cycle
        double latest_read_cycle;
        //To keep track of the last WRITE cycle
        double latest_write_cycle;

        //To calculate end of READ operation
        double end_read_op;
        //To calculate end of WRITE operation
        double end_write_op;
        //To calculate end of ACT operation
        double end_act_op;

        //Clock cycle when self-refresh was issued
        double sref_cycle;

	//Clock cycle when the latest power-down was issued
        double pdn_cycle;

        //Memory State
        unsigned mem_state;

        //For command type
        int type;

        //For command bank
        int bank;

	//Command Issue timestamp in clock cycles (cc)
        double timestamp;

	//Clock cycle of first activate command when memory state changes to ACT
        double first_act_cycle;

	//Clock cycle of last precharge command when memory state changes to PRE
        double last_pre_cycle;

	//Data structures to list explicit commands
        unsigned nCommands;

	//Data structure to keep track of auto-precharges
        unsigned nCached;

	//To collect and analyse all commands including auto-precharges
        void analyse_commands(const int nbrofBanks, Data::MemorySpecification
        memSpec, double nCommands, double nCached);

	//To identify auto-precharges
        void getCommands(const MemorySpecification& memSpec, const int
                nbrofBanks, std::ifstream& pwr_trace);

	//To perform timing analysis of a given set of commands and update command counters
        void evaluate(const MemorySpecification& memSpec,
                std::vector<MemCommand>&cmd_list, int nbrofBanks);

        //To calculate time of completion of any issued command
        int timeToCompletion(const MemorySpecification& memSpec,
                MemCommand::cmds type);

	//To update idle period information whenever active cycles may be idle
        void idle_act_update(const MemorySpecification& memSpec,
                double latest_read_cycle, double latest_write_cycle,
                double latest_act_cycle, double timestamp);

	//To update idle period information whenever precharged cycles may be idle
        void idle_pre_update(const MemorySpecification& memSpec,
                double timestamp, double latest_pre_cycle);

    };
}
#endif
