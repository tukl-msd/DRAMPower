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
 * Authors: Karthik Chandrasekar, Matthias Jung, Omar Naji
 *
 */
#include <fstream>
#include <algorithm>
#include <sstream>

#include "CommandAnalysis.h"
#include "CmdScheduler.h"

using namespace Data;
using namespace std;

CommandAnalysis::CommandAnalysis() {
}

CommandAnalysis::CommandAnalysis(ifstream& pwr_trace, const int nbrofBanks,
        Data::MemorySpecification memSpec) {

    //Initializing all counters and variables

    nCommands = 0;
    nCached = 0;
    numberofacts = 0;
    numberofpres = 0;
    numberofreads = 0;
    numberofwrites = 0;
    numberofrefs = 0;
    f_act_pdns = 0;
    s_act_pdns = 0;
    f_pre_pdns = 0;
    s_pre_pdns = 0;
    numberofsrefs = 0;

    pop = 0;
    init = 0;
    zero = 0;

    actcycles = 0;
    precycles = 0;
    f_act_pdcycles = 0;
    s_act_pdcycles = 0;
    f_pre_pdcycles = 0;
    s_pre_pdcycles = 0;
    pup_act_cycles = 0;
    pup_pre_cycles = 0;
    sref_cycles = 0;
    spup_cycles = 0;
    sref_ref_act_cycles = 0;
    sref_ref_pre_cycles = 0;
    spup_ref_act_cycles = 0;
    spup_ref_pre_cycles = 0;
    idlecycles_act = 0;
    idlecycles_pre = 0;

    latest_act_cycle = -1;
    latest_pre_cycle = -1;
    latest_read_cycle = -1;
    latest_write_cycle = -1;
    end_read_op = 0;
    end_write_op = 0;
    end_act_op = 0;

    first_act_cycle = 0;
    last_pre_cycle = 0;

    bankstate.resize(nbrofBanks, 0);
    last_states.resize(nbrofBanks);
    mem_state = 0;

    sref_cycle = 0;
    pdn_cycle = 0;

    type = 0;
    bank = 0;
    timestamp = 0;

    cmd_list.resize(1, MemCommand::PRE);
    full_cmd_list.resize(1, MemCommand::PRE);
    cached_cmd.resize(1, MemCommand::PRE);

    //To get all the commands from the input trace and analyse them
    getCommands(memSpec, nbrofBanks, pwr_trace);
    pwr_trace.close();
    cached_cmd.clear();
    cmd_list.clear();
    full_cmd_list.clear();
    last_states.clear();
    bankstate.clear();
}


// Constructor for libDRAMPower
CommandAnalysis::CommandAnalysis(std::vector<MemCommand>& list, const int nbrofBanks, Data::MemorySpecification memSpec) {

    //Initializing all counters and variables

    nCommands = 0;
    nCached = 0;

    numberofacts = 0;
    numberofpres = 0;
    numberofreads = 0;
    numberofwrites = 0;
    numberofrefs = 0;
    f_act_pdns = 0;
    s_act_pdns = 0;
    f_pre_pdns = 0;
    s_pre_pdns = 0;
    numberofsrefs = 0;

    pop = 0;
    init = 0;
    zero = 0;

    actcycles = 0;
    precycles = 0;
    f_act_pdcycles = 0;
    s_act_pdcycles = 0;
    f_pre_pdcycles = 0;
    s_pre_pdcycles = 0;
    pup_act_cycles = 0;
    pup_pre_cycles = 0;
    sref_cycles = 0;
    spup_cycles = 0;
    sref_ref_act_cycles = 0;
    sref_ref_pre_cycles = 0;
    spup_ref_act_cycles = 0;
    spup_ref_pre_cycles = 0;
    idlecycles_act = 0;
    idlecycles_pre = 0;

    latest_act_cycle = -1;
    latest_pre_cycle = -1;
    latest_read_cycle = -1;
    latest_write_cycle = -1;
    end_read_op = 0;
    end_write_op = 0;
    end_act_op = 0;

    first_act_cycle = 0;
    last_pre_cycle = 0;

    bankstate.resize(nbrofBanks, 0);
    last_states.resize(nbrofBanks);
    mem_state = 0;

    sref_cycle = 0;
    pdn_cycle = 0;

    type = 0;
    bank = 0;
    timestamp = 0;

    cmd_list.resize(1, MemCommand::PRE);
    full_cmd_list.resize(1, MemCommand::PRE);
    cached_cmd.resize(1, MemCommand::PRE);
    //to get commands from cmd_list vector and analyze them
    getCommands_lib(memSpec, nbrofBanks, list);
	cached_cmd.clear();
    cmd_list.clear();
	list.clear();
    full_cmd_list.clear();
    last_states.clear();
    bankstate.clear();
}
//getCommand function for DRAMPower library
void CommandAnalysis::getCommands_lib(const Data::MemorySpecification& memSpec,
        const int nbrofBanks, std::vector<MemCommand>& list) {
	std::vector<double> activation_cycle(nbrofBanks, 0);
	for (int i = 0 ; i < list.size(); i++) {
		cmd_list.resize(cmd_list.size() + 1, MemCommand::PRE);
		cmd_list[i] = list[i]; 
			if (cmd_list[nCommands].getType() == MemCommand::ACT) {
                activation_cycle[cmd_list[nCommands].getBank()] =
                                cmd_list[nCommands].getTime();
            } else if (cmd_list[nCommands].getType() == MemCommand::RDA) {
                cmd_list[nCommands].setType(MemCommand::RD);
                cached_cmd.resize(cached_cmd.size() + 1, MemCommand::PRE);
                cached_cmd[nCached].setType(MemCommand::PRE);
                cached_cmd[nCached].setTime(max(cmd_list[nCommands].getTime() +
                    cmd_list[nCommands].getPrechargeOffset(memSpec, MemCommand::RDA),
                    activation_cycle[cmd_list[nCommands].getBank()] +
                                                        memSpec.memTimingSpec.RAS));
                cached_cmd[nCached].setBank(cmd_list[nCommands].getBank());
                nCached++;
            } else if (cmd_list[nCommands].getType() == MemCommand::WRA) {
                cmd_list[nCommands].setType(MemCommand::WR);
                cached_cmd.resize(cached_cmd.size() + 1, MemCommand::PRE);
                cached_cmd[nCached].setType(MemCommand::PRE);
                cached_cmd[nCached].setTime(max(cmd_list[nCommands].getTime() +
                    cmd_list[nCommands].getPrechargeOffset(memSpec, MemCommand::WRA),
                    activation_cycle[cmd_list[nCommands].getBank()] +
                                                        memSpec.memTimingSpec.RAS));
                cached_cmd[nCached].setBank(cmd_list[nCommands].getBank());
                nCached++;
            }
		nCommands++;
		 //Based on the analysis windows length defined in CommandAnalysis.h
		 //calls the analyse and evaluate functions to analyse the expanded
		 //trace (including the auto-precharges) and to update the relevant
		 //counters and state changes
			if (nCommands % ANALYSIS_WINDOW == 0) {
            	pop = 0;
            	analyse_commands(nbrofBanks, memSpec, nCommands, nCached);
            	nCommands = 0;
            	nCached = 0;
            	cmd_list.resize(1, MemCommand::PRE);
            	cached_cmd.resize(1, MemCommand::PRE);
        	}
	}
	//For any remaining commands that have not yet been analysed
	pop = 0;
	nCommands++;
	analyse_commands(nbrofBanks, memSpec, nCommands, nCached);        	
}

//Reads through the trace file, identifies the timestamp, command and bank
//If the issued command includes an auto-precharge, adds an explicit
//precharge to a cached command list and computes the precharge offset from the
//issued command timestamp, when the auto-precharge would kick in

void CommandAnalysis::getCommands(const Data::MemorySpecification& memSpec,
        const int nbrofBanks, ifstream& pwr_trace) {
    std::vector<double> activation_cycle(nbrofBanks, 0);
    std::string line;
    MemCommand::cmds type;
    while (getline(pwr_trace, line)) {
        istringstream linestream(line);
        string item;
        double item_val;
        unsigned itemnum = 0;
        cmd_list.resize(cmd_list.size() + 1, MemCommand::PRE);
        while (getline(linestream, item, ',')) {
            if (itemnum == 0) {
                stringstream timestamp(item);
                timestamp >> item_val;
                cmd_list[nCommands].setTime(item_val);
            } else if (itemnum == 1) {
                item_val = MemCommand::getTypeFromName(item);
                cmd_list[nCommands].setType(static_cast<MemCommand::cmds> (item_val));
            } else if (itemnum == 2) {
                stringstream bank(item);
                bank >> item_val;
	            cmd_list[nCommands].setType(type);
                cmd_list[nCommands].setBank(static_cast<unsigned>(item_val));
            }
	        type = cmd_list[nCommands].getType();
            itemnum++;
        }
            if (cmd_list[nCommands].getType() == MemCommand::ACT) {
                activation_cycle[cmd_list[nCommands].getBank()] =
                                cmd_list[nCommands].getTime();
            } else if (cmd_list[nCommands].getType() == MemCommand::RDA) {
                cmd_list[nCommands].setType(MemCommand::RD);
                cached_cmd.resize(cached_cmd.size() + 1, MemCommand::PRE);
                cached_cmd[nCached].setType(MemCommand::PRE);
                cached_cmd[nCached].setTime(max(cmd_list[nCommands].getTime() + 
                    cmd_list[nCommands].getPrechargeOffset(memSpec, MemCommand::RDA),
                    activation_cycle[cmd_list[nCommands].getBank()] +
														memSpec.memTimingSpec.RAS));
                cached_cmd[nCached].setBank(cmd_list[nCommands].getBank());
                nCached++;
            } else if (cmd_list[nCommands].getType() == MemCommand::WRA) {
                cmd_list[nCommands].setType(MemCommand::WR);
                cached_cmd.resize(cached_cmd.size() + 1, MemCommand::PRE);
                cached_cmd[nCached].setType(MemCommand::PRE);
                cached_cmd[nCached].setTime(max(cmd_list[nCommands].getTime() +
                    cmd_list[nCommands].getPrechargeOffset(memSpec, MemCommand::WRA),
                    activation_cycle[cmd_list[nCommands].getBank()] +
														memSpec.memTimingSpec.RAS));
                cached_cmd[nCached].setBank(cmd_list[nCommands].getBank());
                nCached++;
            }
        nCommands++;

        //Based on the analysis windows length defined in CommandAnalysis.h
        //calls the analyse and evaluate functions to analyse the expanded
        //trace (including the auto-precharges) and to update the relevant
        //counters and state changes
        if (nCommands % ANALYSIS_WINDOW == 0) {
            pop = 0;
            analyse_commands(nbrofBanks, memSpec, nCommands, nCached);
            nCommands = 0;
            nCached = 0;
            cmd_list.resize(1, MemCommand::PRE);
            cached_cmd.resize(1, MemCommand::PRE);
        }
    }

    //For any remaining commands that have not yet been analysed
    pop = 0;
	analyse_commands(nbrofBanks, memSpec, nCommands, nCached);
}

//Checks the auto-precharge cached command list and inserts the explicit
//precharges with the appropriate timestamp in the original command list
//(by merging) based on their offset from the issuing command. Calls the
//evaluate function to analyse this expanded list of commands.

void CommandAnalysis::analyse_commands(const int nbrofBanks,
        Data::MemorySpecification memSpec, double nCommands, double nCached) {
    full_cmd_list.resize(1, MemCommand::PRE);
    unsigned mCommands = 0;
    unsigned mCached = 0;
    for (unsigned i = 0; i < nCommands + nCached + 1; i++) {
		if (cached_cmd.size() > 1) {
            if (cmd_list[mCommands].getTime() > 1 && init == 0) {
                full_cmd_list[i].setType(MemCommand::PREA);
                init = 1;
                pop = 1;
            } else {
                init = 1;
                if (cached_cmd[mCached].getTime() > 0 && cmd_list.
                        at(mCommands).getTime() < cached_cmd[mCached].
                        getTime() && (cmd_list[mCommands].getTime() > 0 ||
                        (cmd_list[mCommands].getTime() == 0 && cmd_list[mCommands].
                        getType() != MemCommand::PRE))) {
                    full_cmd_list[i] = cmd_list[mCommands];
                    mCommands++;
                } else if (cached_cmd[mCached].getTime() > 0 && cmd_list[mCommands].
                        getTime() >= cached_cmd[mCached].getTime()) {
                    full_cmd_list[i] = cached_cmd[mCached];
                    mCached++;
                } else if (cached_cmd[mCached].getTime() == 0) {
                    if (cmd_list[mCommands].getTime() > 0 || (cmd_list[mCommands].
                            getTime() == 0 && cmd_list[mCommands].
                            getType() != MemCommand::PRE)) {
                        full_cmd_list[i] = cmd_list[mCommands];
                        mCommands++;
                    }
                } else if (cmd_list[mCommands].getTime() == 0) {
                    full_cmd_list[i] = cached_cmd[mCached];
                    mCached++;
                }
            }
        } else {
            if (cmd_list[mCommands].getTime() > 1 && init == 0) {
                full_cmd_list[i].setType(MemCommand::PREA);
                init = 1;
                pop = 1;
            } else {
                init = 1;
                if (cmd_list[mCommands].getTime() > 0 || (cmd_list.
                        at(mCommands).getTime() == 0 && cmd_list[mCommands].
                        getType() != MemCommand::PRE)) {
                    full_cmd_list[i] = cmd_list[mCommands];
                    mCommands++;
                }
            }
        }
        full_cmd_list.resize(full_cmd_list.size() + 1, MemCommand::PRE);
    }

    full_cmd_list.pop_back();
    if (pop == 0) {
        full_cmd_list.pop_back();
    }

    if(nCommands < ANALYSIS_WINDOW)
    {
        full_cmd_list.resize(full_cmd_list.size() + 1, MemCommand::NOP);
        full_cmd_list[full_cmd_list.size() - 1].setTime(full_cmd_list
		[full_cmd_list.size() - 2].getTime() + timeToCompletion(memSpec, 
		full_cmd_list[full_cmd_list.size() -2].getType()) - 1);
    }
    evaluate(memSpec, full_cmd_list, nbrofBanks);
}

//To get the time of completion of the issued command
//Derived based on JEDEC specifications

int CommandAnalysis::timeToCompletion(const MemorySpecification&
        memSpec, MemCommand::cmds type) {

    int offset = 0;
    const MemTimingSpec& memTimingSpec = memSpec.memTimingSpec;
    const MemArchitectureSpec& memArchSpec = memSpec.memArchSpec;

    if (type == MemCommand::RD) {
        offset = static_cast<int> (memTimingSpec.RL +
                memTimingSpec.DQSCK + 1 + (memArchSpec.burstLength /
                memArchSpec.dataRate));
    } else if (type == MemCommand::WR) {
        offset = static_cast<int> (memTimingSpec.WL +
                (memArchSpec.burstLength / memArchSpec.dataRate) +
                memTimingSpec.WR);
    } else if (type == MemCommand::ACT) {
        offset = static_cast<int> (memTimingSpec.RCD);
    } else if (type == MemCommand::PRE || type == MemCommand::PREA) {
        offset = static_cast<int> (memTimingSpec.RP);
    }
    return offset;
}

//Used to analyse a given list of commands and identify command timings
//and memory state transitions
void CommandAnalysis::evaluate(const MemorySpecification& memSpec,
        vector<MemCommand>& cmd_list, int nbrofBanks) {
	//for each command identify timestamp, type and bank
    for (unsigned cmd_list_counter = 0; cmd_list_counter < cmd_list.size();
            cmd_list_counter++) {
        type = cmd_list[cmd_list_counter].getType();
        bank = cmd_list[cmd_list_counter].getBank();
        timestamp = cmd_list[cmd_list_counter].getTime();

        if (type == MemCommand::ACT) {
            //If command is ACT - update number of acts, bank state of the
            //target bank, first and latest activation cycle and the memory
            //state. Update the number of precharged/idle-precharged cycles.
            numberofacts++;
	    	if(bankstate[bank] == 1)
		    {
				cout << "Bank is already active!" << endl;
				cout << "Command :" << type << "Timestamp: " << timestamp << 
														", Bank: " << bank << endl;
		    }
            bankstate[bank] = 1;
            if (mem_state == 0) {
                first_act_cycle = timestamp;
                precycles += max(zero, timestamp - last_pre_cycle);
                idle_pre_update(memSpec, timestamp, latest_pre_cycle);
            }
            latest_act_cycle = timestamp;
            mem_state++;
        } else if (type == MemCommand::RD) {
            //If command is RD - update number of reads and read cycle. Check
            //for active idle cycles (if any).
		    if(bankstate[bank] == 0)
		    {
				cout << "Bank is not active!" << endl;
				cout << "Command :" << type << "Timestamp: " << timestamp << 
														", Bank: " << bank << endl;
	    	}
            numberofreads++;
            idle_act_update(memSpec, latest_read_cycle, latest_write_cycle,
                    latest_act_cycle, timestamp);
            latest_read_cycle = timestamp;
        } else if (type == MemCommand::WR) {
            //If command is WR - update number of writes and write cycle. Check
            //for active idle cycles (if any).
		    if(bankstate[bank] == 0)
		    {
				cout << "Bank is not active!" << endl;
				cout << "Command :" << type << "Timestamp: " << timestamp << 
														", Bank: " << bank << endl;
		    }
            numberofwrites++;
            idle_act_update(memSpec, latest_read_cycle, latest_write_cycle,
                    latest_act_cycle, timestamp);
            latest_write_cycle = timestamp;
        } else if (type == MemCommand::REF) {
            //If command is REF - update number of refreshes, set bank state of
            //all banks to ACT, set the last PRE cycles at RFC-RP cycles from
            //timestamp, set the number of active cycles to RFC-RP and check
            //for active and precharged cycles and idle active and idle
            //precharged cycles before refresh. Change memory state to 0.
		    if(mem_state != 0)
		    {
				cout << "One or more banks are active! REF requires all banks to be "
															<< "precharged." << endl;
				cout << "Command :" << type << "Timestamp: " << timestamp << 
														", Bank: " << bank << endl;
		    }
            numberofrefs++;
            idle_pre_update(memSpec, timestamp, latest_pre_cycle);
            first_act_cycle = timestamp;
            precycles += max(zero, timestamp - last_pre_cycle);
            last_pre_cycle = timestamp + memSpec.memTimingSpec.RFC -
                                                        memSpec.memTimingSpec.RP;
            latest_pre_cycle = last_pre_cycle;
            actcycles += memSpec.memTimingSpec.RFC - memSpec.memTimingSpec.RP;
            mem_state = 0;
            for (int j = 0; j < nbrofBanks; j++) {
                bankstate[j] = 0;
            }
        } else if (type == MemCommand::PRE) {
            //If command is explicit PRE - update number of precharges, bank
            //state of the target bank and last and latest precharge cycle.
            //Calculate the number of active cycles if the memory was in the
            //active state before, but there is a state transition to PRE now.
            //If not, update the number of precharged cycles and idle cycles.
            //Update memory state if needed.
            if (bankstate[bank] == 1) {
                numberofpres++;
            }
            bankstate[bank] = 0;

            if (mem_state == 1) {
                actcycles += max(zero, timestamp - first_act_cycle);
                last_pre_cycle = timestamp;
                idle_act_update(memSpec, latest_read_cycle, latest_write_cycle,
                        latest_act_cycle, timestamp);
            } else if (mem_state == 0) {
                precycles += max(zero, timestamp - last_pre_cycle);
                idle_pre_update(memSpec, timestamp, latest_pre_cycle);
                last_pre_cycle = timestamp;
            }
            latest_pre_cycle = timestamp;
            if (mem_state > 0) {
                mem_state--;
            } else {
                mem_state = 0;
            }
        } else if (type == MemCommand::PREA) {
            //If command is explicit PREA (precharge all banks) - update
            //number of precharges by the number of banks, update the bank
            //state of all banks to PRE and set the precharge cycle.
            //Calculate the number of active cycles if the memory was in the
            //active state before, but there is a state transition to PRE now.
            //If not, update the number of precharged cycles and idle cycles.
            if (timestamp == 0) {
                numberofpres += 0;
            } else {
                numberofpres += mem_state;
            }

            if (mem_state > 0) {
                actcycles += max(zero, timestamp - first_act_cycle);
                idle_act_update(memSpec, latest_read_cycle, latest_write_cycle,
                        latest_act_cycle, timestamp);
            } else if (mem_state == 0) {
                precycles += max(zero, timestamp - last_pre_cycle);
                idle_pre_update(memSpec, timestamp, latest_pre_cycle);
            }

            latest_pre_cycle = timestamp;
            last_pre_cycle = timestamp;

            mem_state = 0;

            for (int j = 0; j < nbrofBanks; j++) {
                bankstate[j] = 0;
            }
        } else if (type == MemCommand::PDN_F_ACT) {
            //If command is fast-exit active power-down - update number of
            //power-downs, set the power-down cycle and the memory mode to
            //fast-exit active power-down. Save states of all the banks from
            //the cycle before entering active power-down, to be returned to
            //after powering-up. Update active and active idle cycles.
		    if(mem_state == 0)
		    {
				cout << "All banks are precharged! Incorrect use of Active "
															"Power-Down." << endl;
				cout << "Command :" << type << "Timestamp: " << timestamp << 
														", Bank: " << bank << endl;
		    }
            f_act_pdns++;
            for (int j = 0; j < nbrofBanks; j++) {
                last_states[j] = bankstate[j];
            }
            pdn_cycle = timestamp;
            actcycles += max(zero, timestamp - first_act_cycle);
            idle_act_update(memSpec, latest_read_cycle, latest_write_cycle,
                    latest_act_cycle, timestamp);
            mem_state = CommandAnalysis::MS_PDN_F_ACT;
        } else if (type == MemCommand::PDN_S_ACT) {
            //If command is slow-exit active power-down - update number of
            //power-downs, set the power-down cycle and the memory mode to
            //slow-exit active power-down. Save states of all the banks from
            //the cycle before entering active power-down, to be returned to
            //after powering-up. Update active and active idle cycles.
		    if(mem_state == 0)
		    {
				cout << "All banks are precharged! Incorrect use of Active "
															"Power-Down." << endl;
				cout << "Command :" << type << "Timestamp: " << timestamp << 
														", Bank: " << bank << endl;
		    }
            s_act_pdns++;
            for (int j = 0; j < nbrofBanks; j++) {
                last_states[j] = bankstate[j];
            }
            pdn_cycle = timestamp;
            actcycles += max(zero, timestamp - first_act_cycle);
            idle_act_update(memSpec, latest_read_cycle, latest_write_cycle,
                    latest_act_cycle, timestamp);
            mem_state = CommandAnalysis::MS_PDN_S_ACT;
        } else if (type == MemCommand::PDN_F_PRE) {
            //If command is fast-exit precharged power-down - update number of
            //power-downs, set the power-down cycle and the memory mode to
            //fast-exit precahrged power-down. Update precharged and precharged
            //idle cycles.
		    if(mem_state != 0)
		    {
				cout << "One or more banks are active! Incorrect use of Precharged "
														  << "Power-Down." << endl;
				cout << "Command :" << type << "Timestamp: " << timestamp << 
														", Bank: " << bank << endl;
		    }
            f_pre_pdns++;
            pdn_cycle = timestamp;
            precycles += max(zero, timestamp - last_pre_cycle);
            idle_pre_update(memSpec, timestamp, latest_pre_cycle);
            mem_state = CommandAnalysis::MS_PDN_F_PRE;
        } else if (type == MemCommand::PDN_S_PRE) {
            //If command is slow-exit precharged power-down - update number of
            //power-downs, set the power-down cycle and the memory mode to
            //slow-exit precahrged power-down. Update precharged and precharged
            //idle cycles.
		    if(mem_state != 0)
		    {
				cout << "One or more banks are active! Incorrect use of Precharged "
														  << "Power-Down." << endl;
				cout << "Command :" << type << "Timestamp: " << timestamp << 
														", Bank: " << bank << endl;
		    }
            s_pre_pdns++;
            pdn_cycle = timestamp;
            precycles += max(zero, timestamp - last_pre_cycle);
            idle_pre_update(memSpec, timestamp, latest_pre_cycle);
            mem_state = CommandAnalysis::MS_PDN_S_PRE;
        } else if (type == MemCommand::PUP_ACT) {
            //If command is power-up in the active mode - check the power-down
            //exit-mode employed (fast or slow), update the number of power-down
            //and power-up cycles and the latest and first act cycle. Also, reset
            //all the individual bank states to the respective saved states
            //before entering power-down.
            if (mem_state == CommandAnalysis::MS_PDN_F_ACT) {
                f_act_pdcycles += max(zero, timestamp - pdn_cycle);
                pup_act_cycles += memSpec.memTimingSpec.XP;
                latest_act_cycle = max(timestamp, timestamp +
                        memSpec.memTimingSpec.XP - memSpec.memTimingSpec.RCD);
            } else if (mem_state == CommandAnalysis::MS_PDN_S_ACT) {
                s_act_pdcycles += max(zero, timestamp - pdn_cycle);
                if (memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR")
				|| memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR2")
				|| memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR3")
				|| memSpec.memoryType == memSpec.getMemoryTypeFromName("WIDEIO_SDR")){
                    pup_act_cycles += memSpec.memTimingSpec.XP;
                    latest_act_cycle = max(timestamp, timestamp +
                            memSpec.memTimingSpec.XP - memSpec.memTimingSpec.RCD);
                } else {
                    pup_act_cycles += memSpec.memTimingSpec.XPDLL -
                            memSpec.memTimingSpec.RCD;
                    latest_act_cycle = max(timestamp, timestamp +
                            memSpec.memTimingSpec.XPDLL -
                            (2 * memSpec.memTimingSpec.RCD));
                }
            } else if (mem_state != CommandAnalysis::MS_PDN_S_ACT || mem_state != 
													CommandAnalysis::MS_PDN_F_ACT) {
				cout << "Incorrect use of Active Power-Up!" << endl;	    	
	    }
            mem_state = 0;
            for (int j = 0; j < nbrofBanks; j++) {
                bankstate[j] = last_states[j];
                mem_state += last_states[j];
            }
            first_act_cycle = timestamp;
        } else if (type == MemCommand::PUP_PRE) {
            //If command is power-up in the precharged mode - check the power-down
            //exit-mode employed (fast or slow), update the number of power-down
            //and power-up cycles and the latest and last pre cycle.
            if (mem_state == CommandAnalysis::MS_PDN_F_PRE) {
                f_pre_pdcycles += max(zero, timestamp - pdn_cycle);
                pup_pre_cycles += memSpec.memTimingSpec.XP;
                latest_pre_cycle = max(timestamp, timestamp +
                        memSpec.memTimingSpec.XP - memSpec.memTimingSpec.RP);
            } else if (mem_state == CommandAnalysis::MS_PDN_S_PRE) {
                s_pre_pdcycles += max(zero, timestamp - pdn_cycle);
                if (memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR")
				|| memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR2")
				|| memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR3")
				|| memSpec.memoryType == memSpec.getMemoryTypeFromName("WIDEIO_SDR")){
                    pup_pre_cycles += memSpec.memTimingSpec.XP;
                    latest_pre_cycle = max(timestamp, timestamp +
                            memSpec.memTimingSpec.XP - memSpec.memTimingSpec.RP);
                } else {
                    pup_pre_cycles += memSpec.memTimingSpec.XPDLL -
                            memSpec.memTimingSpec.RCD;
                    latest_pre_cycle = max(timestamp, timestamp +
                            memSpec.memTimingSpec.XPDLL - memSpec.memTimingSpec.RCD -
                            memSpec.memTimingSpec.RP);
                }
            } else if (mem_state != CommandAnalysis::MS_PDN_S_PRE || mem_state != 
								CommandAnalysis::MS_PDN_F_PRE) {
				cout << "Incorrect use of Precharged Power-Up!" << endl;	    	
	    	}
            mem_state = 0;
            last_pre_cycle = timestamp;
        } else if (type == MemCommand::SREN) {
            //If command is self-refresh - update number of self-refreshes,
            //set memory state to SREF, update precharge and idle precharge
            //cycles and set the self-refresh cycle.
	    	if(mem_state != 0)
		    {
				cout << "One or more banks are active! SREF requires all banks to be"
														<< " precharged." << endl;
				cout << "Command :" << type << "Timestamp: " << timestamp << 
														", Bank: " << bank << endl;
		    }
            numberofsrefs++;
            sref_cycle = timestamp;
            precycles += max(zero, timestamp - last_pre_cycle);
            idle_pre_update(memSpec, timestamp, latest_pre_cycle);
            mem_state = CommandAnalysis::MS_SREF;
        } else if (type == MemCommand::SREX) {
            //If command is self-refresh exit - update the number of self-refresh
            //clock cycles, number of active and precharged auto-refresh clock
            //cycles during self-refresh and self-refresh exit based on the number
            //of cycles in the self-refresh mode and auto-refresh duration (RFC).
            //Set the last and latest precharge cycle accordingly and set the
            //memory state to 0.
		    if (mem_state != CommandAnalysis::MS_SREF) {
				cout << "Incorrect use of Self-Refresh Power-Up!" << endl;	    	
		    }
            if (max(zero, timestamp - sref_cycle) >= memSpec.memTimingSpec.RFC) {
                sref_cycles += max(zero, timestamp - sref_cycle
                                                        - memSpec.memTimingSpec.RFC);
                sref_ref_act_cycles += memSpec.memTimingSpec.RFC -
                        memSpec.memTimingSpec.RP;
                sref_ref_pre_cycles += memSpec.memTimingSpec.RP;
                last_pre_cycle = timestamp;
                if (memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR")
				|| memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR2")
				|| memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR3")
				|| memSpec.memoryType == memSpec.getMemoryTypeFromName("WIDEIO_SDR")){
                    spup_cycles += memSpec.memTimingSpec.XS;
                    latest_pre_cycle = max(timestamp, timestamp +
                            memSpec.memTimingSpec.XS - memSpec.memTimingSpec.RP);
                } else {
                    spup_cycles += memSpec.memTimingSpec.XSDLL -
                            memSpec.memTimingSpec.RCD;
                    latest_pre_cycle = max(timestamp, timestamp +
                            memSpec.memTimingSpec.XSDLL - memSpec.memTimingSpec.RCD
                            - memSpec.memTimingSpec.RP);
                }
            } else {
                double sref_diff = memSpec.memTimingSpec.RFC-memSpec.memTimingSpec.RP;
                double sref_pre = max(zero, timestamp - sref_cycle - sref_diff);
                double spup_pre = memSpec.memTimingSpec.RP - sref_pre;
                double sref_act = max(zero, timestamp - sref_cycle);
                double spup_act = memSpec.memTimingSpec.RFC - sref_act;

                if (max(zero, timestamp - sref_cycle) >= sref_diff) {
                    sref_ref_act_cycles += sref_diff;
                    sref_ref_pre_cycles += sref_pre;
                    spup_ref_pre_cycles += spup_pre;
                    last_pre_cycle = timestamp + spup_pre;
                    if (memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR")
					|| memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR2")
					|| memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR3")
					|| memSpec.memoryType == 
										memSpec.getMemoryTypeFromName("WIDEIO_SDR")) {
                        spup_cycles += memSpec.memTimingSpec.XS - spup_pre;
                        latest_pre_cycle = max(timestamp, timestamp +
                                memSpec.memTimingSpec.XS - spup_pre -
                                memSpec.memTimingSpec.RP);
                    } else {
                        spup_cycles += memSpec.memTimingSpec.XSDLL -
											memSpec.memTimingSpec.RCD - spup_pre;
                        latest_pre_cycle = max(timestamp, timestamp +
							memSpec.memTimingSpec.XSDLL - memSpec.memTimingSpec.RCD -
											spup_pre - memSpec.memTimingSpec.RP);
                    }
                } else {
                    sref_ref_act_cycles += sref_act;
                    spup_ref_act_cycles += spup_act;
                    spup_ref_pre_cycles += memSpec.memTimingSpec.RP;
                    last_pre_cycle = timestamp + spup_act + memSpec.memTimingSpec.RP;
                    if (memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR")
					|| memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR2")
					|| memSpec.memoryType == memSpec.getMemoryTypeFromName("LPDDR3")
					|| memSpec.memoryType ==
										memSpec.getMemoryTypeFromName("WIDEIO_SDR")) {
                        spup_cycles += memSpec.memTimingSpec.XS - spup_act -
                                memSpec.memTimingSpec.RP;
                        latest_pre_cycle = max(timestamp, timestamp +
                                memSpec.memTimingSpec.XS - spup_act -
                                (2 * memSpec.memTimingSpec.RP));
                    } else {
                        spup_cycles += memSpec.memTimingSpec.XSDLL -
                                memSpec.memTimingSpec.RCD - spup_act -
                                memSpec.memTimingSpec.RP;
                        latest_pre_cycle = max(timestamp, timestamp +
							memSpec.memTimingSpec.XSDLL - memSpec.memTimingSpec.RCD -
                                spup_act - (2 * memSpec.memTimingSpec.RP));
                    }
                }
            }
            mem_state = 0;
        } else if (type == MemCommand::END || type == MemCommand::NOP) {
            //May be optionally used at the end of memory trace for better accuracy
            //Update all counters based on completion of operations.
            if (mem_state > 0 && mem_state < 9) {
                actcycles += max(zero, timestamp - first_act_cycle);
                idle_act_update(memSpec, latest_read_cycle, latest_write_cycle,
                        latest_act_cycle, timestamp);
            } else if (mem_state == 0) {
                precycles += max(zero, timestamp - last_pre_cycle);
                idle_pre_update(memSpec, timestamp, latest_pre_cycle);
            } else if (mem_state == CommandAnalysis::MS_PDN_F_ACT) {
                f_act_pdcycles += max(zero, timestamp - pdn_cycle);
            } else if (mem_state == CommandAnalysis::MS_PDN_S_ACT) {
                s_act_pdcycles += max(zero, timestamp - pdn_cycle);
            } else if (mem_state == CommandAnalysis::MS_PDN_F_PRE) {
                f_pre_pdcycles += max(zero, timestamp - pdn_cycle);
            } else if (mem_state == CommandAnalysis::MS_PDN_S_PRE) {
                s_pre_pdcycles += max(zero, timestamp - pdn_cycle);
            } else if (mem_state == CommandAnalysis::MS_SREF) {
                sref_cycles += max(zero, timestamp - sref_cycle);
            }
        }
    }
}

//To update idle period information whenever active cycles may be idle
void CommandAnalysis::idle_act_update(const MemorySpecification& memSpec,
        double latest_read_cycle, double latest_write_cycle,
        double latest_act_cycle, double timestamp) {

    if (latest_read_cycle >= 0) {
        end_read_op = latest_read_cycle + timeToCompletion(memSpec,
                MemCommand::RD) - 1;
    }

    if (latest_write_cycle >= 0) {
        end_write_op = latest_write_cycle + timeToCompletion(memSpec,
                MemCommand::WR) - 1;
    }

    if (latest_act_cycle >= 0) {
        end_act_op = latest_act_cycle + timeToCompletion(memSpec,
                MemCommand::ACT) - 1;
    }

    idlecycles_act += max(zero, timestamp - max(max(end_read_op, end_write_op), 
																		end_act_op));
}

//To update idle period information whenever precharged cycles may be idle
void CommandAnalysis::idle_pre_update(const MemorySpecification& memSpec,
        double timestamp, double latest_pre_cycle) {

    if (latest_pre_cycle > 0) {
        idlecycles_pre += max(zero, timestamp - latest_pre_cycle - 
														memSpec.memTimingSpec.RP);
    } else if (latest_pre_cycle == 0) {
        idlecycles_pre += max(zero, timestamp - latest_pre_cycle);
    }
}

