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

#include "MemoryPowerModel.h"
#include "CmdScheduler.h"
#include <math.h>

using namespace std;
using namespace Data;

//Calculate energy and average power consumption for the DRAMPowerlib
void MemoryPowerModel::lib_power(MemorySpecification memSpec, std::vector<MemCommand>& cmd_list,
				int grouping, int interleaving, int burst, int term,
				int powerdown){
	MemTimingSpec& memTimingSpec = memSpec.memTimingSpec;
    MemArchitectureSpec& memArchSpec = memSpec.memArchSpec;
    MemPowerSpec& memPowerSpec = memSpec.memPowerSpec;
    CommandAnalysis timings;
	//creating timings
	time_t startnow = time(0);
	tm* startpm = localtime(&startnow);
	cout << "* Power Computation Start time: " << asctime(startpm);
	//create CommandAnalysis object using cmd_list
	timings = CommandAnalysis(cmd_list, memArchSpec.nbrOfBanks, memSpec);
	 //To obtain command timings from the trace
	act_energy = 0.0;
	pre_energy = 0.0;
	read_energy = 0.0;
	write_energy = 0.0;
	ref_energy = 0.0;
	act_stdby_energy = 0.0;
	pre_stdby_energy = 0.0;
	idle_energy_act = 0.0;
	idle_energy_pre = 0.0;
	total_energy = 0.0;
	f_act_pd_energy = 0.0;
	f_pre_pd_energy = 0.0;
	s_act_pd_energy = 0.0;
	s_pre_pd_energy = 0.0;
	sref_energy = 0.0;
	sref_ref_energy = 0.0;
	sref_ref_act_energy = 0.0;
	sref_ref_pre_energy = 0.0;
	spup_energy = 0.0;
	spup_ref_energy = 0.0;
	spup_ref_act_energy = 0.0;
	spup_ref_pre_energy = 0.0;
	pup_act_energy = 0.0;
	pup_pre_energy = 0.0;	
	IO_power = 0.0;
	WR_ODT_power = 0.0;
	TermRD_power = 0.0;
	TermWR_power = 0.0;	
	read_io_energy = 0.0;
	write_term_energy = 0.0;
	read_oterm_energy = 0.0;
	write_oterm_energy = 0.0;
	io_term_energy = 0.0;
	//IO and Termination Power measures are included, if required.
	if(term)
	{
		io_term_power(memSpec);
		
		//Read IO power is consumed by each DQ (data) and DQS (data strobe) pin
		//Width represents data pins
		//1 DQS pin is associated with every data byte
		read_io_energy = (IO_power * timings.numberofreads * memArchSpec.burstLength 
										* (memArchSpec.width + memArchSpec.width/8));	
		
		//Write ODT power is consumed by each DQ (data), DQS (data strobe) and DM 
		//(data mask) pin. 
		//Width represents data pins
		//1 DQS and 1 DM pin is associated with every data byte
		write_term_energy = (WR_ODT_power * timings.numberofwrites * 
				memArchSpec.burstLength * (memArchSpec.width + memArchSpec.width/4));
		
		if(memArchSpec.nbrOfRanks > 1)
		{
			//Termination power consumed in the idle rank during reads on the active
			//rank by each DQ (data) and DQS (data strobe) pin. 
			//Width represents data pins
			//1 DQS pin is associated with every data byte
			read_oterm_energy = (TermRD_power * timings.numberofreads * 
				memArchSpec.burstLength * (memArchSpec.width + memArchSpec.width/8));
			
			//Termination power consumed in the idle rank during writes on the active
			//rank by each DQ (data), DQS (data strobe) and DM (data mask) pin. 
			//Width represents data pins
			//1 DQS and 1 DM pin is associated with every data byte
			write_oterm_energy = (TermWR_power * timings.numberofwrites * 
				memArchSpec.burstLength * (memArchSpec.width + memArchSpec.width/4));
		}
		
		//Sum of all IO and termination energy
		io_term_energy = read_io_energy + write_term_energy + read_oterm_energy + 
																  write_oterm_energy;		
	}	
	
	
	MemorySpecification::MemoryType memoryType;
    memoryType = memSpec.memoryType;

    total_cycles = timings.actcycles + timings.precycles +
          timings.f_act_pdcycles + timings.f_pre_pdcycles +
          timings.s_act_pdcycles + timings.s_pre_pdcycles + timings.sref_cycles
          + timings.sref_ref_act_cycles + timings.sref_ref_pre_cycles +
          timings.spup_ref_act_cycles + timings.spup_ref_pre_cycles;

    act_energy = timings.numberofacts * engy_act(memPowerSpec.idd3n,
            memPowerSpec.idd0, memPowerSpec.vdd, memTimingSpec.RAS,
            memTimingSpec.clkPeriod);

    pre_energy = timings.numberofpres * engy_pre(memPowerSpec.idd2n,
            memPowerSpec.idd0, memPowerSpec.vdd, memTimingSpec.RAS,
            memTimingSpec.RC, memTimingSpec.clkPeriod);

    read_energy = timings.numberofreads * engy_read_cmd(memPowerSpec.idd3n,
            memPowerSpec.idd4r, memPowerSpec.vdd, memArchSpec.burstLength /
            memArchSpec.dataRate, memTimingSpec.clkPeriod);

    write_energy = timings.numberofwrites * engy_write_cmd(memPowerSpec.idd3n,
            memPowerSpec.idd4w, memPowerSpec.vdd, memArchSpec.burstLength /
            memArchSpec.dataRate, memTimingSpec.clkPeriod);

    ref_energy = timings.numberofrefs * engy_ref(memPowerSpec.idd3n,
            memPowerSpec.idd5, memPowerSpec.vdd, memTimingSpec.RFC,
            memTimingSpec.clkPeriod);

    pre_stdby_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
            timings.precycles, memTimingSpec.clkPeriod);

    act_stdby_energy = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
            timings.actcycles, memTimingSpec.clkPeriod);

    //Idle energy in the active standby clock cycles
    idle_energy_act = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
            timings.idlecycles_act, memTimingSpec.clkPeriod);

    //Idle energy in the precharge standby clock cycles
    idle_energy_pre = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
            timings.idlecycles_pre, memTimingSpec.clkPeriod);

    //fast-exit active power-down cycles energy
    f_act_pd_energy = engy_f_act_pd(memPowerSpec.idd3p1, memPowerSpec.vdd,
            timings.f_act_pdcycles, memTimingSpec.clkPeriod);

    //fast-exit precharged power-down cycles energy
    f_pre_pd_energy = engy_f_pre_pd(memPowerSpec.idd2p1, memPowerSpec.vdd,
            timings.f_pre_pdcycles, memTimingSpec.clkPeriod);

    //slow-exit active power-down cycles energy
    s_act_pd_energy = engy_s_act_pd(memPowerSpec.idd3p0, memPowerSpec.vdd,
            timings.s_act_pdcycles, memTimingSpec.clkPeriod);

    //slow-exit precharged power-down cycles energy
    s_pre_pd_energy = engy_s_pre_pd(memPowerSpec.idd2p0, memPowerSpec.vdd,
            timings.s_pre_pdcycles, memTimingSpec.clkPeriod);

    //self-refresh cycles energy including a refresh per self-refresh entry
    sref_energy = engy_sref(memPowerSpec.idd6, memPowerSpec.idd3n,
            memPowerSpec.idd5, memPowerSpec.vdd,
            timings.sref_cycles, timings.sref_ref_act_cycles,
            timings.sref_ref_pre_cycles, timings.spup_ref_act_cycles,
            timings.spup_ref_pre_cycles, memTimingSpec.clkPeriod);

    //background energy during active auto-refresh cycles in self-refresh
    sref_ref_act_energy = engy_s_act_pd(memPowerSpec.idd3p0, memPowerSpec.vdd,
            timings.sref_ref_act_cycles, memTimingSpec.clkPeriod);

    //background energy during precharged auto-refresh cycles in self-refresh
    sref_ref_pre_energy = engy_s_pre_pd(memPowerSpec.idd2p0, memPowerSpec.vdd,
            timings.sref_ref_pre_cycles, memTimingSpec.clkPeriod);

    //background energy during active auto-refresh cycles in self-refresh exit
    spup_ref_act_energy = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
            timings.spup_ref_act_cycles, memTimingSpec.clkPeriod);

    //background energy during precharged auto-refresh cycles in self-refresh exit
    spup_ref_pre_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
            timings.spup_ref_pre_cycles, memTimingSpec.clkPeriod);

    //self-refresh power-up cycles energy -- included
    spup_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
            timings.spup_cycles, memTimingSpec.clkPeriod);

    //active power-up cycles energy - same as active standby -- included
    pup_act_energy = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
            timings.pup_act_cycles, memTimingSpec.clkPeriod);

    //precharged power-up cycles energy - same as precharged standby -- included
    pup_pre_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
            timings.pup_pre_cycles, memTimingSpec.clkPeriod);

    //similar equations as before to support multiple voltage domains in LPDDR2
    //and WIDEIO memories
    if (memoryType == MemorySpecification::getMemoryTypeFromName("LPDDR2")||
        memoryType == MemorySpecification::getMemoryTypeFromName("LPDDR3")||
        memoryType == MemorySpecification::getMemoryTypeFromName("DDR4")||
        memoryType == MemorySpecification::getMemoryTypeFromName("WIDEIO_SDR")){
        act_energy += timings.numberofacts * engy_act(memPowerSpec.idd3n2,
                memPowerSpec.idd02, memPowerSpec.vdd2, memTimingSpec.RAS,
                memTimingSpec.clkPeriod);

        pre_energy += timings.numberofpres * engy_pre(memPowerSpec.idd2n2,
                memPowerSpec.idd02, memPowerSpec.vdd2, memTimingSpec.RAS,
                memTimingSpec.RC, memTimingSpec.clkPeriod);

        read_energy += timings.numberofreads * engy_read_cmd(memPowerSpec.idd3n2,
                memPowerSpec.idd4r2, memPowerSpec.vdd2, memArchSpec.burstLength/
                memArchSpec.dataRate, memTimingSpec.clkPeriod);

        write_energy += timings.numberofwrites *
                engy_write_cmd(memPowerSpec.idd3n2, memPowerSpec.idd4w2,
                memPowerSpec.vdd2, memArchSpec.burstLength /
                memArchSpec.dataRate, memTimingSpec.clkPeriod);

        ref_energy += timings.numberofrefs * engy_ref(memPowerSpec.idd3n2,
                memPowerSpec.idd52, memPowerSpec.vdd2, memTimingSpec.RFC,
                memTimingSpec.clkPeriod);

        pre_stdby_energy += engy_pre_stdby(memPowerSpec.idd2n2,memPowerSpec.vdd2,
                timings.precycles, memTimingSpec.clkPeriod);

        act_stdby_energy += engy_act_stdby(memPowerSpec.idd3n2,memPowerSpec.vdd2,
                timings.actcycles, memTimingSpec.clkPeriod);

        idle_energy_act += engy_act_stdby(memPowerSpec.idd3n2, memPowerSpec.vdd2,
                timings.idlecycles_act, memTimingSpec.clkPeriod);

        idle_energy_pre += engy_pre_stdby(memPowerSpec.idd2n2, memPowerSpec.vdd2,
                timings.idlecycles_pre, memTimingSpec.clkPeriod);

        f_act_pd_energy += engy_f_act_pd(memPowerSpec.idd3p12, memPowerSpec.vdd2,
                timings.f_act_pdcycles, memTimingSpec.clkPeriod);

        f_pre_pd_energy += engy_f_pre_pd(memPowerSpec.idd2p12, memPowerSpec.vdd2,
                timings.f_pre_pdcycles, memTimingSpec.clkPeriod);

        s_act_pd_energy += engy_s_act_pd(memPowerSpec.idd3p02, memPowerSpec.vdd2,
                timings.s_act_pdcycles, memTimingSpec.clkPeriod);

        s_pre_pd_energy += engy_s_pre_pd(memPowerSpec.idd2p02, memPowerSpec.vdd2,
                timings.s_pre_pdcycles, memTimingSpec.clkPeriod);

        sref_energy += engy_sref(memPowerSpec.idd62, memPowerSpec.idd3n2,
                memPowerSpec.idd52, memPowerSpec.vdd2,
                timings.sref_cycles, timings.sref_ref_act_cycles,
                timings.sref_ref_pre_cycles, timings.spup_ref_act_cycles,
                timings.spup_ref_pre_cycles, memTimingSpec.clkPeriod);

        sref_ref_act_energy += engy_s_act_pd(memPowerSpec.idd3p02,
                memPowerSpec.vdd2, timings.sref_ref_act_cycles,
                                                       memTimingSpec.clkPeriod);

        sref_ref_pre_energy += engy_s_pre_pd(memPowerSpec.idd2p02,
                memPowerSpec.vdd2, timings.sref_ref_pre_cycles,
                                                       memTimingSpec.clkPeriod);

        spup_ref_act_energy += engy_act_stdby(memPowerSpec.idd3n2,
                memPowerSpec.vdd2, timings.spup_ref_act_cycles,
                                                       memTimingSpec.clkPeriod);

        spup_ref_pre_energy += engy_pre_stdby(memPowerSpec.idd2n2,
                memPowerSpec.vdd2, timings.spup_ref_pre_cycles,
                                                       memTimingSpec.clkPeriod);

        spup_energy += engy_pre_stdby(memPowerSpec.idd2n2, memPowerSpec.vdd2,
                timings.spup_cycles, memTimingSpec.clkPeriod);

        pup_act_energy += engy_act_stdby(memPowerSpec.idd3n2, memPowerSpec.vdd2,
                timings.pup_act_cycles, memTimingSpec.clkPeriod);

        pup_pre_energy += engy_pre_stdby(memPowerSpec.idd2n2, memPowerSpec.vdd2,
                timings.pup_pre_cycles, memTimingSpec.clkPeriod);

    }

    //auto-refresh energy during self-refresh cycles
    sref_ref_energy = sref_ref_act_energy + sref_ref_pre_energy;

    //auto-refresh energy during self-refresh exit cycles
    spup_ref_energy = spup_ref_act_energy + spup_ref_pre_energy;

	//adding all energy components for the active rank and all background and idle
	//energy components for both ranks (in a dual-rank system)
    total_energy = act_energy + pre_energy + read_energy + write_energy +
            ref_energy + io_term_energy + memArchSpec.nbrOfRanks * 
			(act_stdby_energy + pre_stdby_energy + sref_energy +
			f_act_pd_energy + f_pre_pd_energy + s_act_pd_energy + s_pre_pd_energy + 
			sref_ref_energy + spup_ref_energy);

    cout.precision(0);
    cout << "* Trace Details:" << endl;
    cout << "Number of Activates: " << fixed << timings.numberofacts << endl;
    cout << "Number of Reads: " << timings.numberofreads << endl;
    cout << "Number of Writes: " << timings.numberofwrites << endl;
    cout << "Number of Precharges: " << timings.numberofpres << endl;
    cout << "Number of Refreshes: " << timings.numberofrefs << endl;
    cout << "Number of Active Cycles: " << timings.actcycles << endl;
    cout << "  Number of Active Idle Cycles: " << timings.idlecycles_act<< endl;
    cout << "  Number of Active Power-Up Cycles: " << timings.pup_act_cycles << endl;
    cout << "    Number of Auto-Refresh Active cycles during Self-Refresh " <<
								 "Power-Up: " << timings.spup_ref_act_cycles << endl;
    cout << "Number of Precharged Cycles: " << timings.precycles << endl;
    cout << "  Number of Precharged Idle Cycles: " << timings.idlecycles_pre << endl;
    cout << "  Number of Precharged Power-Up Cycles: " << timings.pup_pre_cycles 
																			 << endl;
    cout << "    Number of Auto-Refresh Precharged cycles during Self-Refresh"
							 << " Power-Up: " << timings.spup_ref_pre_cycles << endl;
    cout << "  Number of Self-Refresh Power-Up Cycles: " << timings.spup_cycles
																			 << endl;
    cout << "Total Idle Cycles (Active + Precharged): " <<
							 timings.idlecycles_act + timings.idlecycles_pre << endl;
    cout << "Number of Power-Downs: " << timings.f_act_pdns +
				timings.s_act_pdns + timings.f_pre_pdns + timings.s_pre_pdns << endl;
    cout << "  Number of Active Fast-exit Power-Downs: " << timings.f_act_pdns
																			 << endl;
    cout << "  Number of Active Slow-exit Power-Downs: " << timings.s_act_pdns
																			 << endl;
    cout << "  Number of Precharged Fast-exit Power-Downs: " <<
														  timings.f_pre_pdns << endl;
    cout << "  Number of Precharged Slow-exit Power-Downs: " <<
														  timings.s_pre_pdns << endl;
    cout << "Number of Power-Down Cycles: " << timings.f_act_pdcycles +
	timings.s_act_pdcycles + timings.f_pre_pdcycles + timings.s_pre_pdcycles << endl;
    cout << "  Number of Active Fast-exit Power-Down Cycles: " <<
													  timings.f_act_pdcycles << endl;
    cout << "  Number of Active Slow-exit Power-Down Cycles: " <<
													  timings.s_act_pdcycles << endl;
    cout << "    Number of Auto-Refresh Active cycles during Self-Refresh: " <<
												 timings.sref_ref_act_cycles << endl;
    cout << "  Number of Precharged Fast-exit Power-Down Cycles: " <<
													  timings.f_pre_pdcycles << endl;
    cout << "  Number of Precharged Slow-exit Power-Down Cycles: " <<
													  timings.s_pre_pdcycles << endl;
    cout << "    Number of Auto-Refresh Precharged cycles during Self-Refresh: " <<
												 timings.sref_ref_pre_cycles << endl;
    cout << "Number of Auto-Refresh Cycles: " << timings.numberofrefs *
														   memTimingSpec.RFC << endl;
    cout << "Number of Self-Refreshes: " << timings.numberofsrefs << endl;
    cout << "Number of Self-Refresh Cycles: " << timings.sref_cycles << endl;
    cout << "----------------------------------------" << endl;
    cout << "Total Trace Length (clock cycles): " << total_cycles << endl;
    cout << "----------------------------------------" << endl;
    cout.precision(2);

    cout << "\n* Trace Power and Energy Estimates:" << endl;
    cout << "ACT Cmd Energy: " << act_energy << " pJ" << endl;
    cout << "PRE Cmd Energy: " << pre_energy << " pJ" << endl;
    cout << "RD Cmd Energy: " << read_energy << " pJ" << endl;
    cout << "WR Cmd Energy: " << write_energy << " pJ" << endl;
	if(term)
	{
		cout << "RD I/O Energy: " << read_io_energy << " pJ" << endl;
		//No Termination for LPDDR/2/3 and DDR memories
		if(memSpec.memoryType == MemorySpecification::getMemoryTypeFromName("DDR2") 
		|| memSpec.memoryType == MemorySpecification::getMemoryTypeFromName("DDR3") 
		|| memSpec.memoryType == MemorySpecification::getMemoryTypeFromName("DDR4"))
		{
			cout << "WR Termination Energy: " << write_term_energy << " pJ" << endl;
		}	
		
		if(memArchSpec.nbrOfRanks > 1 && (memSpec.memoryType == 
		MemorySpecification::getMemoryTypeFromName("DDR2") || memSpec.memoryType == 
		MemorySpecification::getMemoryTypeFromName("DDR3") || memSpec.memoryType == 
								 MemorySpecification::getMemoryTypeFromName("DDR4")))
		{
			cout << "RD Termination Energy (Idle rank): " << read_oterm_energy 
																	<< " pJ" << endl;
			cout << "WR Termination Energy (Idle rank): " << write_oterm_energy 
																	<< " pJ" << endl;		
		}
	}
	cout << "ACT Stdby Energy: " << memArchSpec.nbrOfRanks * act_stdby_energy << 
																		" pJ" << endl;
    cout << "  Active Idle Energy: " << memArchSpec.nbrOfRanks * idle_energy_act <<  
																		" pJ" << endl;
    cout << "  Active Power-Up Energy: " << memArchSpec.nbrOfRanks * pup_act_energy << 
																	    " pJ" << endl;
    cout << "    Active Stdby Energy during Auto-Refresh cycles in Self-Refresh"
				   << " Power-Up: " << memArchSpec.nbrOfRanks * spup_ref_act_energy << 
																	    " pJ" << endl;
    cout << "PRE Stdby Energy: " << memArchSpec.nbrOfRanks * pre_stdby_energy << 
																		" pJ" << endl;
    cout << "  Precharge Idle Energy: " << memArchSpec.nbrOfRanks * idle_energy_pre << 
																		" pJ" << endl;
    cout << "  Precharged Power-Up Energy: " << memArchSpec.nbrOfRanks * pup_pre_energy << 
																		" pJ" << endl;
    cout << "    Precharge Stdby Energy during Auto-Refresh cycles " <<
	   "in Self-Refresh Power-Up: " << memArchSpec.nbrOfRanks * spup_ref_pre_energy << 
																		" pJ" << endl;
    cout << "  Self-Refresh Power-Up Energy: " << memArchSpec.nbrOfRanks * spup_energy << 
																		" pJ" << endl;
    cout << "Total Idle Energy (Active + Precharged): " << memArchSpec.nbrOfRanks * 
								 (idle_energy_act + idle_energy_pre) << " pJ" << endl;
    cout << "Total Power-Down Energy: " << memArchSpec.nbrOfRanks * (f_act_pd_energy + 
			    f_pre_pd_energy + s_act_pd_energy + s_pre_pd_energy) << " pJ" << endl;
    cout << "  Fast-Exit Active Power-Down Energy: " << memArchSpec.nbrOfRanks * 
													 f_act_pd_energy << " pJ" << endl;
    cout << "  Slow-Exit Active Power-Down Energy: " << memArchSpec.nbrOfRanks * 
													 s_act_pd_energy << " pJ" << endl;
    cout << "    Slow-Exit Active Power-Down Energy during Auto-Refresh cycles "
                << "in Self-Refresh: " << memArchSpec.nbrOfRanks * sref_ref_act_energy << 
																		" pJ" << endl;
    cout << "  Fast-Exit Precharged Power-Down Energy: " << memArchSpec.nbrOfRanks * 
													 f_pre_pd_energy << " pJ" << endl;
    cout << "  Slow-Exit Precharged Power-Down Energy: " << memArchSpec.nbrOfRanks * 
													 s_pre_pd_energy << " pJ" << endl;
    cout << "    Slow-Exit Precharged Power-Down Energy during Auto-Refresh " <<
			 "cycles in Self-Refresh: " << memArchSpec.nbrOfRanks * sref_ref_pre_energy <<
																		" pJ" << endl;
    cout << "Auto-Refresh Energy: " << ref_energy << " pJ" << endl;
    cout << "Self-Refresh Energy: " << memArchSpec.nbrOfRanks * sref_energy << 
																		" pJ" << endl;
    cout << "----------------------------------------" << endl;
    cout << "Total Trace Energy: " << total_energy << " pJ" << endl;
    cout << "Average Power: " << total_energy / (total_cycles * 
											memTimingSpec.clkPeriod) << " mW" << endl;
    cout << "----------------------------------------" << endl;
	
}  
//Calculate energy and average power consumption for the given command trace

void MemoryPowerModel::trace_power(MemorySpecification memSpec,
        ifstream& trace,int trans, int grouping, int interleaving, int burst,
			      int term, int powerdown) {

    MemTimingSpec& memTimingSpec = memSpec.memTimingSpec;
    MemArchitectureSpec& memArchSpec = memSpec.memArchSpec;
    MemPowerSpec& memPowerSpec = memSpec.memPowerSpec;
    CommandAnalysis timings;

    ifstream pwr_trace;

    if(trans){
        cmdScheduler cmdsched;
        cmdsched.transTranslation(memSpec, trace, grouping, interleaving, burst,
																	  	powerdown);
        time_t start = time(0);
        tm* starttm = localtime(&start);
        cout << "* Analysis End Time: " << asctime(starttm);
        pwr_trace.open("commands.trace", ifstream::in);
        cout << "* Power Computation Start time: " << asctime(starttm);
        timings = CommandAnalysis(pwr_trace, memArchSpec.nbrOfBanks, memSpec);
    }
    else {
        time_t startnow = time(0);
        tm* startpm = localtime(&startnow);
        cout << "* Power Computation Start time: " << asctime(startpm);
        timings = CommandAnalysis(trace, memArchSpec.nbrOfBanks, memSpec);
    }

    //To obtain command timings from the trace

    act_energy = 0.0;
    pre_energy = 0.0;
    read_energy = 0.0;
    write_energy = 0.0;
    ref_energy = 0.0;
    act_stdby_energy = 0.0;
    pre_stdby_energy = 0.0;
    idle_energy_act = 0.0;
    idle_energy_pre = 0.0;
    total_energy = 0.0;
    f_act_pd_energy = 0.0;
    f_pre_pd_energy = 0.0;
    s_act_pd_energy = 0.0;
    s_pre_pd_energy = 0.0;
    sref_energy = 0.0;
    sref_ref_energy = 0.0;
    sref_ref_act_energy = 0.0;
    sref_ref_pre_energy = 0.0;
    spup_energy = 0.0;
    spup_ref_energy = 0.0;
    spup_ref_act_energy = 0.0;
    spup_ref_pre_energy = 0.0;
    pup_act_energy = 0.0;
    pup_pre_energy = 0.0;
	
    IO_power = 0.0;
    WR_ODT_power = 0.0;
    TermRD_power = 0.0;
    TermWR_power = 0.0;
	
	read_io_energy = 0.0;
	write_term_energy = 0.0;
	read_oterm_energy = 0.0;
	write_oterm_energy = 0.0;
	io_term_energy = 0.0;

	//IO and Termination Power measures are included, if required.
	if(term)
	{
		io_term_power(memSpec);
		
		//Read IO power is consumed by each DQ (data) and DQS (data strobe) pin
		//Width represents data pins
		//1 DQS pin is associated with every data byte
		read_io_energy = (IO_power * timings.numberofreads * memArchSpec.burstLength 
										* (memArchSpec.width + memArchSpec.width/8));	
		
		//Write ODT power is consumed by each DQ (data), DQS (data strobe) and DM 
		//(data mask) pin. 
		//Width represents data pins
		//1 DQS and 1 DM pin is associated with every data byte
		write_term_energy = (WR_ODT_power * timings.numberofwrites * 
				memArchSpec.burstLength * (memArchSpec.width + memArchSpec.width/4));
		
		if(memArchSpec.nbrOfRanks > 1)
		{
			//Termination power consumed in the idle rank during reads on the active
			//rank by each DQ (data) and DQS (data strobe) pin. 
			//Width represents data pins
			//1 DQS pin is associated with every data byte
			read_oterm_energy = (TermRD_power * timings.numberofreads * 
				memArchSpec.burstLength * (memArchSpec.width + memArchSpec.width/8));
			
			//Termination power consumed in the idle rank during writes on the active
			//rank by each DQ (data), DQS (data strobe) and DM (data mask) pin. 
			//Width represents data pins
			//1 DQS and 1 DM pin is associated with every data byte
			write_oterm_energy = (TermWR_power * timings.numberofwrites * 
				memArchSpec.burstLength * (memArchSpec.width + memArchSpec.width/4));
		}
		
		//Sum of all IO and termination energy
		io_term_energy = read_io_energy + write_term_energy + read_oterm_energy + 
																  write_oterm_energy;		
	}	
	
	
	MemorySpecification::MemoryType memoryType;
    memoryType = memSpec.memoryType;

    total_cycles = timings.actcycles + timings.precycles +
          timings.f_act_pdcycles + timings.f_pre_pdcycles +
          timings.s_act_pdcycles + timings.s_pre_pdcycles + timings.sref_cycles
          + timings.sref_ref_act_cycles + timings.sref_ref_pre_cycles +
          timings.spup_ref_act_cycles + timings.spup_ref_pre_cycles;

    act_energy = timings.numberofacts * engy_act(memPowerSpec.idd3n,
            memPowerSpec.idd0, memPowerSpec.vdd, memTimingSpec.RAS,
            memTimingSpec.clkPeriod);

    pre_energy = timings.numberofpres * engy_pre(memPowerSpec.idd2n,
            memPowerSpec.idd0, memPowerSpec.vdd, memTimingSpec.RAS,
            memTimingSpec.RC, memTimingSpec.clkPeriod);

    read_energy = timings.numberofreads * engy_read_cmd(memPowerSpec.idd3n,
            memPowerSpec.idd4r, memPowerSpec.vdd, memArchSpec.burstLength /
            memArchSpec.dataRate, memTimingSpec.clkPeriod);

    write_energy = timings.numberofwrites * engy_write_cmd(memPowerSpec.idd3n,
            memPowerSpec.idd4w, memPowerSpec.vdd, memArchSpec.burstLength /
            memArchSpec.dataRate, memTimingSpec.clkPeriod);

    ref_energy = timings.numberofrefs * engy_ref(memPowerSpec.idd3n,
            memPowerSpec.idd5, memPowerSpec.vdd, memTimingSpec.RFC,
            memTimingSpec.clkPeriod);

    pre_stdby_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
            timings.precycles, memTimingSpec.clkPeriod);

    act_stdby_energy = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
            timings.actcycles, memTimingSpec.clkPeriod);

    //Idle energy in the active standby clock cycles
    idle_energy_act = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
            timings.idlecycles_act, memTimingSpec.clkPeriod);

    //Idle energy in the precharge standby clock cycles
    idle_energy_pre = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
            timings.idlecycles_pre, memTimingSpec.clkPeriod);

    //fast-exit active power-down cycles energy
    f_act_pd_energy = engy_f_act_pd(memPowerSpec.idd3p1, memPowerSpec.vdd,
            timings.f_act_pdcycles, memTimingSpec.clkPeriod);

    //fast-exit precharged power-down cycles energy
    f_pre_pd_energy = engy_f_pre_pd(memPowerSpec.idd2p1, memPowerSpec.vdd,
            timings.f_pre_pdcycles, memTimingSpec.clkPeriod);

    //slow-exit active power-down cycles energy
    s_act_pd_energy = engy_s_act_pd(memPowerSpec.idd3p0, memPowerSpec.vdd,
            timings.s_act_pdcycles, memTimingSpec.clkPeriod);

    //slow-exit precharged power-down cycles energy
    s_pre_pd_energy = engy_s_pre_pd(memPowerSpec.idd2p0, memPowerSpec.vdd,
            timings.s_pre_pdcycles, memTimingSpec.clkPeriod);

    //self-refresh cycles energy including a refresh per self-refresh entry
    sref_energy = engy_sref(memPowerSpec.idd6, memPowerSpec.idd3n,
            memPowerSpec.idd5, memPowerSpec.vdd,
            timings.sref_cycles, timings.sref_ref_act_cycles,
            timings.sref_ref_pre_cycles, timings.spup_ref_act_cycles,
            timings.spup_ref_pre_cycles, memTimingSpec.clkPeriod);

    //background energy during active auto-refresh cycles in self-refresh
    sref_ref_act_energy = engy_s_act_pd(memPowerSpec.idd3p0, memPowerSpec.vdd,
            timings.sref_ref_act_cycles, memTimingSpec.clkPeriod);

    //background energy during precharged auto-refresh cycles in self-refresh
    sref_ref_pre_energy = engy_s_pre_pd(memPowerSpec.idd2p0, memPowerSpec.vdd,
            timings.sref_ref_pre_cycles, memTimingSpec.clkPeriod);

    //background energy during active auto-refresh cycles in self-refresh exit
    spup_ref_act_energy = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
            timings.spup_ref_act_cycles, memTimingSpec.clkPeriod);

    //background energy during precharged auto-refresh cycles in self-refresh exit
    spup_ref_pre_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
            timings.spup_ref_pre_cycles, memTimingSpec.clkPeriod);

    //self-refresh power-up cycles energy -- included
    spup_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
            timings.spup_cycles, memTimingSpec.clkPeriod);

    //active power-up cycles energy - same as active standby -- included
    pup_act_energy = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
            timings.pup_act_cycles, memTimingSpec.clkPeriod);

    //precharged power-up cycles energy - same as precharged standby -- included
    pup_pre_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
            timings.pup_pre_cycles, memTimingSpec.clkPeriod);

    //similar equations as before to support multiple voltage domains in LPDDR2
    //and WIDEIO memories
    if (memoryType == MemorySpecification::getMemoryTypeFromName("LPDDR2")||
        memoryType == MemorySpecification::getMemoryTypeFromName("LPDDR3")||
        memoryType == MemorySpecification::getMemoryTypeFromName("DDR4")||
        memoryType == MemorySpecification::getMemoryTypeFromName("WIDEIO_SDR")){
        act_energy += timings.numberofacts * engy_act(memPowerSpec.idd3n2,
                memPowerSpec.idd02, memPowerSpec.vdd2, memTimingSpec.RAS,
                memTimingSpec.clkPeriod);

        pre_energy += timings.numberofpres * engy_pre(memPowerSpec.idd2n2,
                memPowerSpec.idd02, memPowerSpec.vdd2, memTimingSpec.RAS,
                memTimingSpec.RC, memTimingSpec.clkPeriod);

        read_energy += timings.numberofreads * engy_read_cmd(memPowerSpec.idd3n2,
                memPowerSpec.idd4r2, memPowerSpec.vdd2, memArchSpec.burstLength/
                memArchSpec.dataRate, memTimingSpec.clkPeriod);

        write_energy += timings.numberofwrites *
                engy_write_cmd(memPowerSpec.idd3n2, memPowerSpec.idd4w2,
                memPowerSpec.vdd2, memArchSpec.burstLength /
                memArchSpec.dataRate, memTimingSpec.clkPeriod);

        ref_energy += timings.numberofrefs * engy_ref(memPowerSpec.idd3n2,
                memPowerSpec.idd52, memPowerSpec.vdd2, memTimingSpec.RFC,
                memTimingSpec.clkPeriod);

        pre_stdby_energy += engy_pre_stdby(memPowerSpec.idd2n2,memPowerSpec.vdd2,
                timings.precycles, memTimingSpec.clkPeriod);

        act_stdby_energy += engy_act_stdby(memPowerSpec.idd3n2,memPowerSpec.vdd2,
                timings.actcycles, memTimingSpec.clkPeriod);

        idle_energy_act += engy_act_stdby(memPowerSpec.idd3n2, memPowerSpec.vdd2,
                timings.idlecycles_act, memTimingSpec.clkPeriod);

        idle_energy_pre += engy_pre_stdby(memPowerSpec.idd2n2, memPowerSpec.vdd2,
                timings.idlecycles_pre, memTimingSpec.clkPeriod);

        f_act_pd_energy += engy_f_act_pd(memPowerSpec.idd3p12, memPowerSpec.vdd2,
                timings.f_act_pdcycles, memTimingSpec.clkPeriod);

        f_pre_pd_energy += engy_f_pre_pd(memPowerSpec.idd2p12, memPowerSpec.vdd2,
                timings.f_pre_pdcycles, memTimingSpec.clkPeriod);

        s_act_pd_energy += engy_s_act_pd(memPowerSpec.idd3p02, memPowerSpec.vdd2,
                timings.s_act_pdcycles, memTimingSpec.clkPeriod);

        s_pre_pd_energy += engy_s_pre_pd(memPowerSpec.idd2p02, memPowerSpec.vdd2,
                timings.s_pre_pdcycles, memTimingSpec.clkPeriod);

        sref_energy += engy_sref(memPowerSpec.idd62, memPowerSpec.idd3n2,
                memPowerSpec.idd52, memPowerSpec.vdd2,
                timings.sref_cycles, timings.sref_ref_act_cycles,
                timings.sref_ref_pre_cycles, timings.spup_ref_act_cycles,
                timings.spup_ref_pre_cycles, memTimingSpec.clkPeriod);

        sref_ref_act_energy += engy_s_act_pd(memPowerSpec.idd3p02,
                memPowerSpec.vdd2, timings.sref_ref_act_cycles,
                                                       memTimingSpec.clkPeriod);

        sref_ref_pre_energy += engy_s_pre_pd(memPowerSpec.idd2p02,
                memPowerSpec.vdd2, timings.sref_ref_pre_cycles,
                                                       memTimingSpec.clkPeriod);

        spup_ref_act_energy += engy_act_stdby(memPowerSpec.idd3n2,
                memPowerSpec.vdd2, timings.spup_ref_act_cycles,
                                                       memTimingSpec.clkPeriod);

        spup_ref_pre_energy += engy_pre_stdby(memPowerSpec.idd2n2,
                memPowerSpec.vdd2, timings.spup_ref_pre_cycles,
                                                       memTimingSpec.clkPeriod);

        spup_energy += engy_pre_stdby(memPowerSpec.idd2n2, memPowerSpec.vdd2,
                timings.spup_cycles, memTimingSpec.clkPeriod);

        pup_act_energy += engy_act_stdby(memPowerSpec.idd3n2, memPowerSpec.vdd2,
                timings.pup_act_cycles, memTimingSpec.clkPeriod);

        pup_pre_energy += engy_pre_stdby(memPowerSpec.idd2n2, memPowerSpec.vdd2,
                timings.pup_pre_cycles, memTimingSpec.clkPeriod);

    }

    //auto-refresh energy during self-refresh cycles
    sref_ref_energy = sref_ref_act_energy + sref_ref_pre_energy;

    //auto-refresh energy during self-refresh exit cycles
    spup_ref_energy = spup_ref_act_energy + spup_ref_pre_energy;

	//adding all energy components for the active rank and all background and idle
	//energy components for both ranks (in a dual-rank system)
    total_energy = act_energy + pre_energy + read_energy + write_energy +
            ref_energy + io_term_energy + memArchSpec.nbrOfRanks * 
			(act_stdby_energy + pre_stdby_energy + sref_energy +
			f_act_pd_energy + f_pre_pd_energy + s_act_pd_energy + s_pre_pd_energy + 
			sref_ref_energy + spup_ref_energy);

    cout.precision(0);
    cout << "* Trace Details:" << endl;
    cout << "Number of Activates: " << fixed << timings.numberofacts << endl;
    cout << "Number of Reads: " << timings.numberofreads << endl;
    cout << "Number of Writes: " << timings.numberofwrites << endl;
    cout << "Number of Precharges: " << timings.numberofpres << endl;
    cout << "Number of Refreshes: " << timings.numberofrefs << endl;
    cout << "Number of Active Cycles: " << timings.actcycles << endl;
    cout << "  Number of Active Idle Cycles: " << timings.idlecycles_act<< endl;
    cout << "  Number of Active Power-Up Cycles: " << timings.pup_act_cycles << endl;
    cout << "    Number of Auto-Refresh Active cycles during Self-Refresh " <<
								 "Power-Up: " << timings.spup_ref_act_cycles << endl;
    cout << "Number of Precharged Cycles: " << timings.precycles << endl;
    cout << "  Number of Precharged Idle Cycles: " << timings.idlecycles_pre << endl;
    cout << "  Number of Precharged Power-Up Cycles: " << timings.pup_pre_cycles 
																			 << endl;
    cout << "    Number of Auto-Refresh Precharged cycles during Self-Refresh"
							 << " Power-Up: " << timings.spup_ref_pre_cycles << endl;
    cout << "  Number of Self-Refresh Power-Up Cycles: " << timings.spup_cycles
																			 << endl;
    cout << "Total Idle Cycles (Active + Precharged): " <<
							 timings.idlecycles_act + timings.idlecycles_pre << endl;
    cout << "Number of Power-Downs: " << timings.f_act_pdns +
				timings.s_act_pdns + timings.f_pre_pdns + timings.s_pre_pdns << endl;
    cout << "  Number of Active Fast-exit Power-Downs: " << timings.f_act_pdns
																			 << endl;
    cout << "  Number of Active Slow-exit Power-Downs: " << timings.s_act_pdns
																			 << endl;
    cout << "  Number of Precharged Fast-exit Power-Downs: " <<
														  timings.f_pre_pdns << endl;
    cout << "  Number of Precharged Slow-exit Power-Downs: " <<
														  timings.s_pre_pdns << endl;
    cout << "Number of Power-Down Cycles: " << timings.f_act_pdcycles +
	timings.s_act_pdcycles + timings.f_pre_pdcycles + timings.s_pre_pdcycles << endl;
    cout << "  Number of Active Fast-exit Power-Down Cycles: " <<
													  timings.f_act_pdcycles << endl;
    cout << "  Number of Active Slow-exit Power-Down Cycles: " <<
													  timings.s_act_pdcycles << endl;
    cout << "    Number of Auto-Refresh Active cycles during Self-Refresh: " <<
												 timings.sref_ref_act_cycles << endl;
    cout << "  Number of Precharged Fast-exit Power-Down Cycles: " <<
													  timings.f_pre_pdcycles << endl;
    cout << "  Number of Precharged Slow-exit Power-Down Cycles: " <<
													  timings.s_pre_pdcycles << endl;
    cout << "    Number of Auto-Refresh Precharged cycles during Self-Refresh: " <<
												 timings.sref_ref_pre_cycles << endl;
    cout << "Number of Auto-Refresh Cycles: " << timings.numberofrefs *
														   memTimingSpec.RFC << endl;
    cout << "Number of Self-Refreshes: " << timings.numberofsrefs << endl;
    cout << "Number of Self-Refresh Cycles: " << timings.sref_cycles << endl;
    cout << "----------------------------------------" << endl;
    cout << "Total Trace Length (clock cycles): " << total_cycles << endl;
    cout << "----------------------------------------" << endl;
    cout.precision(2);

    cout << "\n* Trace Power and Energy Estimates:" << endl;
    cout << "ACT Cmd Energy: " << act_energy << " pJ" << endl;
    cout << "PRE Cmd Energy: " << pre_energy << " pJ" << endl;
    cout << "RD Cmd Energy: " << read_energy << " pJ" << endl;
    cout << "WR Cmd Energy: " << write_energy << " pJ" << endl;
	if(term)
	{
		cout << "RD I/O Energy: " << read_io_energy << " pJ" << endl;
		//No Termination for LPDDR/2/3 and DDR memories
		if(memSpec.memoryType == MemorySpecification::getMemoryTypeFromName("DDR2") 
		|| memSpec.memoryType == MemorySpecification::getMemoryTypeFromName("DDR3") 
		|| memSpec.memoryType == MemorySpecification::getMemoryTypeFromName("DDR4"))
		{
			cout << "WR Termination Energy: " << write_term_energy << " pJ" << endl;
		}	
		
		if(memArchSpec.nbrOfRanks > 1 && (memSpec.memoryType == 
		MemorySpecification::getMemoryTypeFromName("DDR2") || memSpec.memoryType == 
		MemorySpecification::getMemoryTypeFromName("DDR3") || memSpec.memoryType == 
								 MemorySpecification::getMemoryTypeFromName("DDR4")))
		{
			cout << "RD Termination Energy (Idle rank): " << read_oterm_energy 
																	<< " pJ" << endl;
			cout << "WR Termination Energy (Idle rank): " << write_oterm_energy 
																	<< " pJ" << endl;		
		}
	}
	cout << "ACT Stdby Energy: " << memArchSpec.nbrOfRanks * act_stdby_energy << 
																		" pJ" << endl;
    cout << "  Active Idle Energy: " << memArchSpec.nbrOfRanks * idle_energy_act <<  
																		" pJ" << endl;
    cout << "  Active Power-Up Energy: " << memArchSpec.nbrOfRanks * pup_act_energy << 
																	    " pJ" << endl;
    cout << "    Active Stdby Energy during Auto-Refresh cycles in Self-Refresh"
				   << " Power-Up: " << memArchSpec.nbrOfRanks * spup_ref_act_energy << 
																	    " pJ" << endl;
    cout << "PRE Stdby Energy: " << memArchSpec.nbrOfRanks * pre_stdby_energy << 
																		" pJ" << endl;
    cout << "  Precharge Idle Energy: " << memArchSpec.nbrOfRanks * idle_energy_pre << 
																		" pJ" << endl;
    cout << "  Precharged Power-Up Energy: " << memArchSpec.nbrOfRanks * pup_pre_energy << 
																		" pJ" << endl;
    cout << "    Precharge Stdby Energy during Auto-Refresh cycles " <<
	   "in Self-Refresh Power-Up: " << memArchSpec.nbrOfRanks * spup_ref_pre_energy << 
																		" pJ" << endl;
    cout << "  Self-Refresh Power-Up Energy: " << memArchSpec.nbrOfRanks * spup_energy << 
																		" pJ" << endl;
    cout << "Total Idle Energy (Active + Precharged): " << memArchSpec.nbrOfRanks * 
								 (idle_energy_act + idle_energy_pre) << " pJ" << endl;
    cout << "Total Power-Down Energy: " << memArchSpec.nbrOfRanks * (f_act_pd_energy + 
			    f_pre_pd_energy + s_act_pd_energy + s_pre_pd_energy) << " pJ" << endl;
    cout << "  Fast-Exit Active Power-Down Energy: " << memArchSpec.nbrOfRanks * 
													 f_act_pd_energy << " pJ" << endl;
    cout << "  Slow-Exit Active Power-Down Energy: " << memArchSpec.nbrOfRanks * 
													 s_act_pd_energy << " pJ" << endl;
    cout << "    Slow-Exit Active Power-Down Energy during Auto-Refresh cycles "
                << "in Self-Refresh: " << memArchSpec.nbrOfRanks * sref_ref_act_energy << 
																		" pJ" << endl;
    cout << "  Fast-Exit Precharged Power-Down Energy: " << memArchSpec.nbrOfRanks * 
													 f_pre_pd_energy << " pJ" << endl;
    cout << "  Slow-Exit Precharged Power-Down Energy: " << memArchSpec.nbrOfRanks * 
													 s_pre_pd_energy << " pJ" << endl;
    cout << "    Slow-Exit Precharged Power-Down Energy during Auto-Refresh " <<
			 "cycles in Self-Refresh: " << memArchSpec.nbrOfRanks * sref_ref_pre_energy <<
																		" pJ" << endl;
    cout << "Auto-Refresh Energy: " << ref_energy << " pJ" << endl;
    cout << "Self-Refresh Energy: " << memArchSpec.nbrOfRanks * sref_energy << 
																		" pJ" << endl;
    cout << "----------------------------------------" << endl;
    cout << "Total Trace Energy: " << total_energy << " pJ" << endl;
    cout << "Average Power: " << total_energy / (total_cycles * 
											memTimingSpec.clkPeriod) << " mW" << endl;
    cout << "----------------------------------------" << endl;

}

//Activation energy estimation
double MemoryPowerModel::engy_act(double idd3n, double idd0, double vdd,
        int tras, double clk) {
    double act_engy;
    act_engy = (idd0 - idd3n) * tras * vdd * clk;
    return act_engy;
}

//Precharging energy estimation
double MemoryPowerModel::engy_pre(double idd2n, double idd0, double vdd,
        int tras, int trc, double clk) {
    double pre_engy;
    pre_engy = (idd0 - idd2n) * (trc - tras) * vdd * clk;
    return pre_engy;
}

//Read command energy estimation
double MemoryPowerModel::engy_read_cmd(double idd3n, double idd4r, double vdd,
        int tr, double clk) {
    double read_cmd_engy;
    read_cmd_engy = (idd4r - idd3n) * tr * vdd * clk;
    return read_cmd_engy;
}

//Write command energy estimation
double MemoryPowerModel::engy_write_cmd(double idd3n, double idd4w, double vdd,
        int tw, double clk) {
    double write_cmd_engy;
    write_cmd_engy = (idd4w - idd3n) * tw * vdd * clk;
    return write_cmd_engy;
}

//Refresh operation energy estimation
double MemoryPowerModel::engy_ref(double idd3n, double idd5, double vdd,
                                                        int trfc, double clk) {
    double ref_engy;
    ref_engy = (idd5 - idd3n) * trfc * vdd * clk;
    return ref_engy;
}

//Precharge standby energy estimation
double MemoryPowerModel::engy_pre_stdby(double idd2n, double vdd, double precycles,
        double clk) {
    double pre_stdby_engy;
    pre_stdby_engy = idd2n * vdd * precycles * clk;
    return pre_stdby_engy;
}

//Active standby energy estimation
double MemoryPowerModel::engy_act_stdby(double idd3n, double vdd, double actcycles,
        double clk) {
    double act_stdby_engy;
    act_stdby_engy = idd3n * vdd * actcycles * clk;
    return act_stdby_engy;
}

//Fast-exit active power-down energy
double MemoryPowerModel::engy_f_act_pd(double idd3p1, double vdd,
        double f_act_pdcycles, double clk) {
    double f_act_pd_energy;
    f_act_pd_energy = idd3p1 * vdd * f_act_pdcycles * clk;
    return f_act_pd_energy;
}

//Fast-exit precharge power-down energy
double MemoryPowerModel::engy_f_pre_pd(double idd2p1, double vdd,
        double f_pre_pdcycles, double clk) {
    double f_pre_pd_energy;
    f_pre_pd_energy = idd2p1 * vdd * f_pre_pdcycles * clk;
    return f_pre_pd_energy;
}

//Slow-exit active power-down energy
double MemoryPowerModel::engy_s_act_pd(double idd3p0, double vdd,
        double s_act_pdcycles, double clk) {
    double s_act_pd_energy;
    s_act_pd_energy = idd3p0 * vdd * s_act_pdcycles * clk;
    return s_act_pd_energy;
}

//Slow-exit precharge power-down energy
double MemoryPowerModel::engy_s_pre_pd(double idd2p0, double vdd,
        double s_pre_pdcycles, double clk) {
    double s_pre_pd_energy;
    s_pre_pd_energy = idd2p0 * vdd * s_pre_pdcycles * clk;
    return s_pre_pd_energy;
}

//Self-refresh active energy estimation (not including background energy)
double MemoryPowerModel::engy_sref(double idd6, double idd3n, double idd5,
        double vdd, double sref_cycles, double sref_ref_act_cycles,
        double sref_ref_pre_cycles, double spup_ref_act_cycles,
        double spup_ref_pre_cycles, double clk) {
    double sref_energy;
    sref_energy = ((idd6 * sref_cycles) + ((idd5 - idd3n) * (sref_ref_act_cycles
            + spup_ref_act_cycles + sref_ref_pre_cycles + spup_ref_pre_cycles)))
                                                                    * vdd * clk;
    return sref_energy;
}

//IO and Termination power calculation based on Micron Power Calculators
//Absolute power measures are obtained from Micron Power Calculator (mentioned in mW)
void MemoryPowerModel::io_term_power(MemorySpecification memSpec) {
	
    MemTimingSpec& memTimingSpec = memSpec.memTimingSpec;
    MemArchitectureSpec& memArchSpec = memSpec.memArchSpec;
    MemPowerSpec& memPowerSpec = memSpec.memPowerSpec;
    MemorySpecification::MemoryType& memoryType = memSpec.memoryType;

	//For LPDDR/2/3 memories - IO Power depends on DRAM clock frequency
	//No ODT (Termination) in LPDDR/2/3 and DDR memories
	IO_power = 0.5 * pow(memPowerSpec.vdd2, 2.0) * memTimingSpec.clkMhz * 1000000;
	
	//LPDDR/2/3 IO Capacitance in mF
	double LPDDR_Cap  = 0.0000000045;
	double LPDDR2_Cap = 0.0000000025;
	double LPDDR3_Cap = 0.0000000018;
	
	//Conservative estimates based on Micron DDR2 Power Calculator
	if(memoryType == MemorySpecification::getMemoryTypeFromName("DDR2"))
	{
		IO_power = 1.5;//in mW
		WR_ODT_power = 8.2;//in mW
		if(memArchSpec.nbrOfRanks > 1)
		{
			TermRD_power = 13.1;//in mW
			TermWR_power = 14.6;//in mW
		}	
	} 
	//Conservative estimates based on Micron DDR3 Power Calculator
	else if(memoryType == MemorySpecification::getMemoryTypeFromName("DDR3"))
	{
		IO_power = 4.6;//in mW
		WR_ODT_power = 21.2;//in mW
		if(memArchSpec.nbrOfRanks > 1)
		{
			TermRD_power = 15.5;//in mW
			TermWR_power = 15.4;//in mW
		}	
	}
	//Conservative estimates based on Micron DDR3 Power Calculator
	//using available termination resistance values from Micron DDR4 Datasheets 
	else if(memoryType == MemorySpecification::getMemoryTypeFromName("DDR4"))
	{
		IO_power = 3.7;//in mW
		WR_ODT_power = 17.0;//in mW
		if(memArchSpec.nbrOfRanks > 1)
		{
			TermRD_power = 12.4;//in mW
			TermWR_power = 12.3;//in mW
		}	
	}
	//LPDDR/2/3 and DDR memories only have IO Power (no ODT)
	//Conservative estimates based on Micron Mobile LPDDR2 Power Calculator
	else if(memoryType == MemorySpecification::getMemoryTypeFromName("LPDDR"))
	{
		IO_power = LPDDR_Cap * IO_power;
	} 
	else if(memoryType == MemorySpecification::getMemoryTypeFromName("LPDDR2"))
	{
		IO_power = LPDDR2_Cap * IO_power;
	} 
	else if(memoryType == MemorySpecification::getMemoryTypeFromName("LPDDR3"))
	{
		IO_power = LPDDR3_Cap * IO_power;
	}	
	else if(memoryType == MemorySpecification::getMemoryTypeFromName("DDR"))
	{
		IO_power = 6.88;//in mW
	}		
}

