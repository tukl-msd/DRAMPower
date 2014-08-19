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
 * Authors: Karthik Chandrasekar, Matthias Jung, Omar Naji
 *
 */

#include "MemoryPowerModel.h"

#include <cmath>  // For pow

using namespace std;
using namespace Data;

// Calculate energy and average power consumption for the given command trace

void MemoryPowerModel::power_calc(MemorySpecification memSpec,
                                  const CommandAnalysis& counters,
                                  int term)
{
  MemTimingSpec& memTimingSpec     = memSpec.memTimingSpec;
  MemArchitectureSpec& memArchSpec = memSpec.memArchSpec;
  MemPowerSpec&  memPowerSpec      = memSpec.memPowerSpec;

  energy.act_energy          = 0.0;
  energy.pre_energy          = 0.0;
  energy.read_energy         = 0.0;
  energy.write_energy        = 0.0;
  energy.ref_energy          = 0.0;
  energy.act_stdby_energy    = 0.0;
  energy.pre_stdby_energy    = 0.0;
  energy.idle_energy_act     = 0.0;
  energy.idle_energy_pre     = 0.0;
  energy.total_energy        = 0.0;
  energy.f_act_pd_energy     = 0.0;
  energy.f_pre_pd_energy     = 0.0;
  energy.s_act_pd_energy     = 0.0;
  energy.s_pre_pd_energy     = 0.0;
  energy.sref_energy         = 0.0;
  energy.sref_ref_energy     = 0.0;
  energy.sref_ref_act_energy = 0.0;
  energy.sref_ref_pre_energy = 0.0;
  energy.spup_energy         = 0.0;
  energy.spup_ref_energy     = 0.0;
  energy.spup_ref_act_energy = 0.0;
  energy.spup_ref_pre_energy = 0.0;
  energy.pup_act_energy      = 0.0;
  energy.pup_pre_energy      = 0.0;
  power.IO_power             = 0.0;
  power.WR_ODT_power         = 0.0;
  power.TermRD_power         = 0.0;
  power.TermWR_power         = 0.0;
  energy.read_io_energy      = 0.0;
  energy.write_term_energy   = 0.0;
  energy.read_oterm_energy   = 0.0;
  energy.write_oterm_energy  = 0.0;
  energy.io_term_energy      = 0.0;

  // IO and Termination Power measures are included, if required.
  if (term) {
    io_term_power(memSpec);

    // Read IO power is consumed by each DQ (data) and DQS (data strobe) pin
    // Width represents data pins
    // 1 DQS pin is associated with every data byte
    energy.read_io_energy = (power.IO_power * static_cast<double>(counters.numberofreads) * memArchSpec.burstLength
                             * (memArchSpec.width + memArchSpec.width / 8));

    // Write ODT power is consumed by each DQ (data), DQS (data strobe) and DM
    // (data mask) pin.
    // Width represents data pins
    // 1 DQS and 1 DM pin is associated with every data byte
    energy.write_term_energy = (power.WR_ODT_power * static_cast<double>(counters.numberofwrites) *
                                memArchSpec.burstLength * (memArchSpec.width + memArchSpec.width / 4));

    if (memArchSpec.nbrOfRanks > 1) {
      // Termination power consumed in the idle rank during reads on the active
      // rank by each DQ (data) and DQS (data strobe) pin.
      // Width represents data pins
      // 1 DQS pin is associated with every data byte
      energy.read_oterm_energy = (power.TermRD_power * static_cast<double>(counters.numberofreads) *
                                  memArchSpec.burstLength * (memArchSpec.width + memArchSpec.width / 8));

      // Termination power consumed in the idle rank during writes on the active
      // rank by each DQ (data), DQS (data strobe) and DM (data mask) pin.
      // Width represents data pins
      // 1 DQS and 1 DM pin is associated with every data byte
      energy.write_oterm_energy = (power.TermWR_power * static_cast<double>(counters.numberofwrites) *
                                   memArchSpec.burstLength * (memArchSpec.width + memArchSpec.width / 4));
    }

    // Sum of all IO and termination energy
    energy.io_term_energy = energy.read_io_energy + energy.write_term_energy
                            + energy.read_oterm_energy + energy.write_oterm_energy;
  }

  MemoryType memoryType;
  memoryType   = memSpec.memoryType;

  total_cycles = counters.actcycles + counters.precycles +
                 counters.f_act_pdcycles + counters.f_pre_pdcycles +
                 counters.s_act_pdcycles + counters.s_pre_pdcycles + counters.sref_cycles
                 + counters.sref_ref_act_cycles + counters.sref_ref_pre_cycles +
                 counters.spup_ref_act_cycles + counters.spup_ref_pre_cycles;

  energy.act_energy = static_cast<double>(counters.numberofacts) * engy_act(memPowerSpec.idd3n,
                                                      memPowerSpec.idd0, memPowerSpec.vdd, memTimingSpec.RAS,
                                                      memTimingSpec.clkPeriod);

  energy.pre_energy = static_cast<double>(counters.numberofpres) * engy_pre(memPowerSpec.idd2n,
                                                      memPowerSpec.idd0, memPowerSpec.vdd, memTimingSpec.RAS,
                                                      memTimingSpec.RC, memTimingSpec.clkPeriod);

  energy.read_energy = static_cast<double>(counters.numberofreads) * engy_read_cmd(memPowerSpec.idd3n,
                                                             memPowerSpec.idd4r, memPowerSpec.vdd, memArchSpec.burstLength /
                                                             memArchSpec.dataRate, memTimingSpec.clkPeriod);

  energy.write_energy = static_cast<double>(counters.numberofwrites) * engy_write_cmd(memPowerSpec.idd3n,
                                                                memPowerSpec.idd4w, memPowerSpec.vdd, memArchSpec.burstLength /
                                                                memArchSpec.dataRate, memTimingSpec.clkPeriod);

  energy.ref_energy = static_cast<double>(counters.numberofrefs) * engy_ref(memPowerSpec.idd3n,
                                                      memPowerSpec.idd5, memPowerSpec.vdd, memTimingSpec.RFC,
                                                      memTimingSpec.clkPeriod);

  energy.pre_stdby_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
                                           static_cast<double>(counters.precycles), memTimingSpec.clkPeriod);

  energy.act_stdby_energy = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
                                           static_cast<double>(counters.actcycles), memTimingSpec.clkPeriod);

  // Idle energy in the active standby clock cycles
  energy.idle_energy_act = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
                                          static_cast<double>(counters.idlecycles_act), memTimingSpec.clkPeriod);

  // Idle energy in the precharge standby clock cycles
  energy.idle_energy_pre = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
                                          static_cast<double>(counters.idlecycles_pre), memTimingSpec.clkPeriod);

  // fast-exit active power-down cycles energy
  energy.f_act_pd_energy = engy_f_act_pd(memPowerSpec.idd3p1, memPowerSpec.vdd,
                                         static_cast<double>(counters.f_act_pdcycles), memTimingSpec.clkPeriod);

  // fast-exit precharged power-down cycles energy
  energy.f_pre_pd_energy = engy_f_pre_pd(memPowerSpec.idd2p1, memPowerSpec.vdd,
                                         static_cast<double>(counters.f_pre_pdcycles), memTimingSpec.clkPeriod);

  // slow-exit active power-down cycles energy
  energy.s_act_pd_energy = engy_s_act_pd(memPowerSpec.idd3p0, memPowerSpec.vdd,
                                         static_cast<double>(counters.s_act_pdcycles), memTimingSpec.clkPeriod);

  // slow-exit precharged power-down cycles energy
  energy.s_pre_pd_energy = engy_s_pre_pd(memPowerSpec.idd2p0, memPowerSpec.vdd,
                                         static_cast<double>(counters.s_pre_pdcycles), memTimingSpec.clkPeriod);

  // self-refresh cycles energy including a refresh per self-refresh entry
  energy.sref_energy = engy_sref(memPowerSpec.idd6, memPowerSpec.idd3n,
                                 memPowerSpec.idd5, memPowerSpec.vdd,
                                 static_cast<double>(counters.sref_cycles), static_cast<double>(counters.sref_ref_act_cycles),
                                 static_cast<double>(counters.sref_ref_pre_cycles), static_cast<double>(counters.spup_ref_act_cycles),
                                 static_cast<double>(counters.spup_ref_pre_cycles), memTimingSpec.clkPeriod);

  // background energy during active auto-refresh cycles in self-refresh
  energy.sref_ref_act_energy = engy_s_act_pd(memPowerSpec.idd3p0, memPowerSpec.vdd,
                                             static_cast<double>(counters.sref_ref_act_cycles), memTimingSpec.clkPeriod);

  // background energy during precharged auto-refresh cycles in self-refresh
  energy.sref_ref_pre_energy = engy_s_pre_pd(memPowerSpec.idd2p0, memPowerSpec.vdd,
                                             static_cast<double>(counters.sref_ref_pre_cycles), memTimingSpec.clkPeriod);

  // background energy during active auto-refresh cycles in self-refresh exit
  energy.spup_ref_act_energy = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
                                              static_cast<double>(counters.spup_ref_act_cycles), memTimingSpec.clkPeriod);

  // background energy during precharged auto-refresh cycles in self-refresh exit
  energy.spup_ref_pre_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
                                              static_cast<double>(counters.spup_ref_pre_cycles), memTimingSpec.clkPeriod);

  // self-refresh power-up cycles energy -- included
  energy.spup_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
                                      static_cast<double>(counters.spup_cycles), memTimingSpec.clkPeriod);

  // active power-up cycles energy - same as active standby -- included
  energy.pup_act_energy = engy_act_stdby(memPowerSpec.idd3n, memPowerSpec.vdd,
                                         static_cast<double>(counters.pup_act_cycles), memTimingSpec.clkPeriod);

  // precharged power-up cycles energy - same as precharged standby -- included
  energy.pup_pre_energy = engy_pre_stdby(memPowerSpec.idd2n, memPowerSpec.vdd,
                                         static_cast<double>(counters.pup_pre_cycles), memTimingSpec.clkPeriod);

  // similar equations as before to support multiple voltage domains in LPDDR2
  // and WIDEIO memories
  if (memoryType.isLPDDRFamily()) {
    energy.act_energy += static_cast<double>(counters.numberofacts) * engy_act(memPowerSpec.idd3n2,
                                                         memPowerSpec.idd02, memPowerSpec.vdd2, memTimingSpec.RAS,
                                                         memTimingSpec.clkPeriod);

    energy.pre_energy += static_cast<double>(counters.numberofpres) * engy_pre(memPowerSpec.idd2n2,
                                                         memPowerSpec.idd02, memPowerSpec.vdd2, memTimingSpec.RAS,
                                                         memTimingSpec.RC, memTimingSpec.clkPeriod);

    energy.read_energy += static_cast<double>(counters.numberofreads) * engy_read_cmd(memPowerSpec.idd3n2,
                                                                memPowerSpec.idd4r2, memPowerSpec.vdd2, memArchSpec.burstLength /
                                                                memArchSpec.dataRate, memTimingSpec.clkPeriod);

    energy.write_energy += static_cast<double>(counters.numberofwrites) *
                           engy_write_cmd(memPowerSpec.idd3n2, memPowerSpec.idd4w2,
                                          memPowerSpec.vdd2, memArchSpec.burstLength /
                                          memArchSpec.dataRate, memTimingSpec.clkPeriod);

    energy.ref_energy += static_cast<double>(counters.numberofrefs) * engy_ref(memPowerSpec.idd3n2,
                                                         memPowerSpec.idd52, memPowerSpec.vdd2, memTimingSpec.RFC,
                                                         memTimingSpec.clkPeriod);

    energy.pre_stdby_energy += engy_pre_stdby(memPowerSpec.idd2n2, memPowerSpec.vdd2,
                                              static_cast<double>(counters.precycles), memTimingSpec.clkPeriod);

    energy.act_stdby_energy += engy_act_stdby(memPowerSpec.idd3n2, memPowerSpec.vdd2,
                                              static_cast<double>(counters.actcycles), memTimingSpec.clkPeriod);

    energy.idle_energy_act  += engy_act_stdby(memPowerSpec.idd3n2, memPowerSpec.vdd2,
                                              static_cast<double>(counters.idlecycles_act), memTimingSpec.clkPeriod);

    energy.idle_energy_pre  += engy_pre_stdby(memPowerSpec.idd2n2, memPowerSpec.vdd2,
                                              static_cast<double>(counters.idlecycles_pre), memTimingSpec.clkPeriod);

    energy.f_act_pd_energy  += engy_f_act_pd(memPowerSpec.idd3p12, memPowerSpec.vdd2,
                                             static_cast<double>(counters.f_act_pdcycles), memTimingSpec.clkPeriod);

    energy.f_pre_pd_energy  += engy_f_pre_pd(memPowerSpec.idd2p12, memPowerSpec.vdd2,
                                             static_cast<double>(counters.f_pre_pdcycles), memTimingSpec.clkPeriod);

    energy.s_act_pd_energy  += engy_s_act_pd(memPowerSpec.idd3p02, memPowerSpec.vdd2,
                                             static_cast<double>(counters.s_act_pdcycles), memTimingSpec.clkPeriod);

    energy.s_pre_pd_energy  += engy_s_pre_pd(memPowerSpec.idd2p02, memPowerSpec.vdd2,
                                             static_cast<double>(counters.s_pre_pdcycles), memTimingSpec.clkPeriod);

    energy.sref_energy      += engy_sref(memPowerSpec.idd62, memPowerSpec.idd3n2,
                                         memPowerSpec.idd52, memPowerSpec.vdd2,
                                         static_cast<double>(counters.sref_cycles), static_cast<double>(counters.sref_ref_act_cycles),
                                         static_cast<double>(counters.sref_ref_pre_cycles), static_cast<double>(counters.spup_ref_act_cycles),
                                         static_cast<double>(counters.spup_ref_pre_cycles), memTimingSpec.clkPeriod);

    energy.sref_ref_act_energy += engy_s_act_pd(memPowerSpec.idd3p02,
                                                memPowerSpec.vdd2, static_cast<double>(counters.sref_ref_act_cycles),
                                                memTimingSpec.clkPeriod);

    energy.sref_ref_pre_energy += engy_s_pre_pd(memPowerSpec.idd2p02,
                                                memPowerSpec.vdd2, static_cast<double>(counters.sref_ref_pre_cycles),
                                                memTimingSpec.clkPeriod);

    energy.spup_ref_act_energy += engy_act_stdby(memPowerSpec.idd3n2,
                                                 memPowerSpec.vdd2, static_cast<double>(counters.spup_ref_act_cycles),
                                                 memTimingSpec.clkPeriod);

    energy.spup_ref_pre_energy += engy_pre_stdby(memPowerSpec.idd2n2,
                                                 memPowerSpec.vdd2, static_cast<double>(counters.spup_ref_pre_cycles),
                                                 memTimingSpec.clkPeriod);

    energy.spup_energy    += engy_pre_stdby(memPowerSpec.idd2n2, memPowerSpec.vdd2,
                                            static_cast<double>(counters.spup_cycles), memTimingSpec.clkPeriod);

    energy.pup_act_energy += engy_act_stdby(memPowerSpec.idd3n2, memPowerSpec.vdd2,
                                            static_cast<double>(counters.pup_act_cycles), memTimingSpec.clkPeriod);

    energy.pup_pre_energy += engy_pre_stdby(memPowerSpec.idd2n2, memPowerSpec.vdd2,
                                            static_cast<double>(counters.pup_pre_cycles), memTimingSpec.clkPeriod);
  }

  // auto-refresh energy during self-refresh cycles
  energy.sref_ref_energy = energy.sref_ref_act_energy + energy.sref_ref_pre_energy;

  // auto-refresh energy during self-refresh exit cycles
  energy.spup_ref_energy = energy.spup_ref_act_energy + energy.spup_ref_pre_energy;

  // adding all energy components for the active rank and all background and idle
  // energy components for both ranks (in a dual-rank system)
  energy.total_energy = energy.act_energy + energy.pre_energy + energy.read_energy +
                        energy.write_energy + energy.ref_energy + energy.io_term_energy +
                        memArchSpec.nbrOfRanks * (energy.act_stdby_energy +
                                                  energy.pre_stdby_energy + energy.sref_energy +
                                                  energy.f_act_pd_energy + energy.f_pre_pd_energy + energy.s_act_pd_energy
                                                  + energy.s_pre_pd_energy + energy.sref_ref_energy + energy.spup_ref_energy);

  // Calculate the average power consumption
  power.average_power = energy.total_energy / (static_cast<double>(total_cycles) * memTimingSpec.clkPeriod);
} // MemoryPowerModel::power_calc

void MemoryPowerModel::power_print(MemorySpecification memSpec, int term, const CommandAnalysis& counters) const
{
  MemTimingSpec& memTimingSpec     = memSpec.memTimingSpec;
  MemArchitectureSpec& memArchSpec = memSpec.memArchSpec;

  cout.precision(0);
  cout << "* Trace Details:" << endl;
  cout << "Number of Activates: " << fixed << counters.numberofacts << endl;
  cout << "Number of Reads: " << counters.numberofreads << endl;
  cout << "Number of Writes: " << counters.numberofwrites << endl;
  cout << "Number of Precharges: " << counters.numberofpres << endl;
  cout << "Number of Refreshes: " << counters.numberofrefs << endl;
  cout << "Number of Active Cycles: " << counters.actcycles << endl;
  cout << "  Number of Active Idle Cycles: " << counters.idlecycles_act << endl;
  cout << "  Number of Active Power-Up Cycles: " << counters.pup_act_cycles << endl;
  cout << "    Number of Auto-Refresh Active cycles during Self-Refresh " <<
    "Power-Up: " << counters.spup_ref_act_cycles << endl;
  cout << "Number of Precharged Cycles: " << counters.precycles << endl;
  cout << "  Number of Precharged Idle Cycles: " << counters.idlecycles_pre << endl;
  cout << "  Number of Precharged Power-Up Cycles: " << counters.pup_pre_cycles
       << endl;
  cout << "    Number of Auto-Refresh Precharged cycles during Self-Refresh"
       << " Power-Up: " << counters.spup_ref_pre_cycles << endl;
  cout << "  Number of Self-Refresh Power-Up Cycles: " << counters.spup_cycles
       << endl;
  cout << "Total Idle Cycles (Active + Precharged): " <<
    counters.idlecycles_act + counters.idlecycles_pre << endl;
  cout << "Number of Power-Downs: " << counters.f_act_pdns +
    counters.s_act_pdns + counters.f_pre_pdns + counters.s_pre_pdns << endl;
  cout << "  Number of Active Fast-exit Power-Downs: " << counters.f_act_pdns
       << endl;
  cout << "  Number of Active Slow-exit Power-Downs: " << counters.s_act_pdns
       << endl;
  cout << "  Number of Precharged Fast-exit Power-Downs: " <<
    counters.f_pre_pdns << endl;
  cout << "  Number of Precharged Slow-exit Power-Downs: " <<
    counters.s_pre_pdns << endl;
  cout << "Number of Power-Down Cycles: " << counters.f_act_pdcycles +
    counters.s_act_pdcycles + counters.f_pre_pdcycles + counters.s_pre_pdcycles << endl;
  cout << "  Number of Active Fast-exit Power-Down Cycles: " <<
    counters.f_act_pdcycles << endl;
  cout << "  Number of Active Slow-exit Power-Down Cycles: " <<
    counters.s_act_pdcycles << endl;
  cout << "    Number of Auto-Refresh Active cycles during Self-Refresh: " <<
    counters.sref_ref_act_cycles << endl;
  cout << "  Number of Precharged Fast-exit Power-Down Cycles: " <<
    counters.f_pre_pdcycles << endl;
  cout << "  Number of Precharged Slow-exit Power-Down Cycles: " <<
    counters.s_pre_pdcycles << endl;
  cout << "    Number of Auto-Refresh Precharged cycles during Self-Refresh: " <<
    counters.sref_ref_pre_cycles << endl;
  cout << "Number of Auto-Refresh Cycles: " << counters.numberofrefs *
    memTimingSpec.RFC << endl;
  cout << "Number of Self-Refreshes: " << counters.numberofsrefs << endl;
  cout << "Number of Self-Refresh Cycles: " << counters.sref_cycles << endl;
  cout << "----------------------------------------" << endl;
  cout << "Total Trace Length (clock cycles): " << total_cycles << endl;
  cout << "----------------------------------------" << endl;
  cout.precision(2);

  cout << "\n* Trace Power and Energy Estimates:" << endl;
  cout << "ACT Cmd Energy: " << energy.act_energy << " pJ" << endl;
  cout << "PRE Cmd Energy: " << energy.pre_energy << " pJ" << endl;
  cout << "RD Cmd Energy: " << energy.read_energy << " pJ" << endl;
  cout << "WR Cmd Energy: " << energy.write_energy << " pJ" << endl;
  if (term) {
    cout << "RD I/O Energy: " << energy.read_io_energy << " pJ" << endl;
    // No Termination for LPDDR/2/3 and DDR memories
    if (memSpec.memoryType.isDDRFamily()) {
      cout << "WR Termination Energy: " << energy.write_term_energy << " pJ" << endl;
    }

    if ((memArchSpec.nbrOfRanks > 1) && memSpec.memoryType.isDDRFamily()) {
      cout << "RD Termination Energy (Idle rank): " << energy.read_oterm_energy
           << " pJ" << endl;
      cout << "WR Termination Energy (Idle rank): " << energy.write_oterm_energy
           << " pJ" << endl;
    }
  }
  cout << "ACT Stdby Energy: " << memArchSpec.nbrOfRanks * energy.act_stdby_energy <<
    " pJ" << endl;
  cout << "  Active Idle Energy: " << memArchSpec.nbrOfRanks * energy.idle_energy_act <<
    " pJ" << endl;
  cout << "  Active Power-Up Energy: " << memArchSpec.nbrOfRanks * energy.pup_act_energy <<
    " pJ" << endl;
  cout << "    Active Stdby Energy during Auto-Refresh cycles in Self-Refresh"
       << " Power-Up: " << memArchSpec.nbrOfRanks * energy.spup_ref_act_energy <<
    " pJ" << endl;
  cout << "PRE Stdby Energy: " << memArchSpec.nbrOfRanks * energy.pre_stdby_energy <<
    " pJ" << endl;
  cout << "  Precharge Idle Energy: " << memArchSpec.nbrOfRanks * energy.idle_energy_pre <<
    " pJ" << endl;
  cout << "  Precharged Power-Up Energy: " << memArchSpec.nbrOfRanks * energy.pup_pre_energy <<
    " pJ" << endl;
  cout << "    Precharge Stdby Energy during Auto-Refresh cycles " <<
    "in Self-Refresh Power-Up: " << memArchSpec.nbrOfRanks * energy.spup_ref_pre_energy <<
    " pJ" << endl;
  cout << "  Self-Refresh Power-Up Energy: " << memArchSpec.nbrOfRanks * energy.spup_energy <<
    " pJ" << endl;
  cout << "Total Idle Energy (Active + Precharged): " << memArchSpec.nbrOfRanks *
  (energy.idle_energy_act + energy.idle_energy_pre) << " pJ" << endl;
  cout << "Total Power-Down Energy: " << memArchSpec.nbrOfRanks * (energy.f_act_pd_energy +
                                                                   energy.f_pre_pd_energy + energy.s_act_pd_energy + energy.s_pre_pd_energy) << " pJ" << endl;
  cout << "  Fast-Exit Active Power-Down Energy: " << memArchSpec.nbrOfRanks *
    energy.f_act_pd_energy << " pJ" << endl;
  cout << "  Slow-Exit Active Power-Down Energy: " << memArchSpec.nbrOfRanks *
    energy.s_act_pd_energy << " pJ" << endl;
  cout << "    Slow-Exit Active Power-Down Energy during Auto-Refresh cycles "
       << "in Self-Refresh: " << memArchSpec.nbrOfRanks * energy.sref_ref_act_energy <<
    " pJ" << endl;
  cout << "  Fast-Exit Precharged Power-Down Energy: " << memArchSpec.nbrOfRanks *
    energy.f_pre_pd_energy << " pJ" << endl;
  cout << "  Slow-Exit Precharged Power-Down Energy: " << memArchSpec.nbrOfRanks *
    energy.s_pre_pd_energy << " pJ" << endl;
  cout << "    Slow-Exit Precharged Power-Down Energy during Auto-Refresh " <<
    "cycles in Self-Refresh: " << memArchSpec.nbrOfRanks * energy.sref_ref_pre_energy <<
    " pJ" << endl;
  cout << "Auto-Refresh Energy: " << energy.ref_energy << " pJ" << endl;
  cout << "Self-Refresh Energy: " << memArchSpec.nbrOfRanks * energy.sref_energy <<
    " pJ" << endl;
  cout << "----------------------------------------" << endl;
  cout << "Total Trace Energy: " << energy.total_energy << " pJ" << endl;
  cout << "Average Power: " << power.average_power << " mW" << endl;
  cout << "----------------------------------------" << endl;
} // MemoryPowerModel::power_print

// Activation energy estimation
double MemoryPowerModel::engy_act(double idd3n, double idd0, double vdd,
                                  int tras, double clk)
{
  double act_engy;

  act_engy = (idd0 - idd3n) * tras * vdd * clk;
  return act_engy;
}

// Precharging energy estimation
double MemoryPowerModel::engy_pre(double idd2n, double idd0, double vdd,
                                  int tras, int trc, double clk)
{
  double pre_engy;

  pre_engy = (idd0 - idd2n) * (trc - tras) * vdd * clk;
  return pre_engy;
}

// Read command energy estimation
double MemoryPowerModel::engy_read_cmd(double idd3n, double idd4r, double vdd,
                                       int tr, double clk)
{
  double read_cmd_engy;

  read_cmd_engy = (idd4r - idd3n) * tr * vdd * clk;
  return read_cmd_engy;
}

// Write command energy estimation
double MemoryPowerModel::engy_write_cmd(double idd3n, double idd4w, double vdd,
                                        int tw, double clk)
{
  double write_cmd_engy;

  write_cmd_engy = (idd4w - idd3n) * tw * vdd * clk;
  return write_cmd_engy;
}

// Refresh operation energy estimation
double MemoryPowerModel::engy_ref(double idd3n, double idd5, double vdd,
                                  int trfc, double clk)
{
  double ref_engy;

  ref_engy = (idd5 - idd3n) * trfc * vdd * clk;
  return ref_engy;
}

// Precharge standby energy estimation
double MemoryPowerModel::engy_pre_stdby(double idd2n, double vdd, double precycles,
                                        double clk)
{
  double pre_stdby_engy;

  pre_stdby_engy = idd2n * vdd * precycles * clk;
  return pre_stdby_engy;
}

// Active standby energy estimation
double MemoryPowerModel::engy_act_stdby(double idd3n, double vdd, double actcycles,
                                        double clk)
{
  double act_stdby_engy;

  act_stdby_engy = idd3n * vdd * actcycles * clk;
  return act_stdby_engy;
}

// Fast-exit active power-down energy
double MemoryPowerModel::engy_f_act_pd(double idd3p1, double vdd,
                                       double f_act_pdcycles, double clk)
{
  double f_act_pd_energy;

  f_act_pd_energy = idd3p1 * vdd * f_act_pdcycles * clk;
  return f_act_pd_energy;
}

// Fast-exit precharge power-down energy
double MemoryPowerModel::engy_f_pre_pd(double idd2p1, double vdd,
                                       double f_pre_pdcycles, double clk)
{
  double f_pre_pd_energy;

  f_pre_pd_energy = idd2p1 * vdd * f_pre_pdcycles * clk;
  return f_pre_pd_energy;
}

// Slow-exit active power-down energy
double MemoryPowerModel::engy_s_act_pd(double idd3p0, double vdd,
                                       double s_act_pdcycles, double clk)
{
  double s_act_pd_energy;

  s_act_pd_energy = idd3p0 * vdd * s_act_pdcycles * clk;
  return s_act_pd_energy;
}

// Slow-exit precharge power-down energy
double MemoryPowerModel::engy_s_pre_pd(double idd2p0, double vdd,
                                       double s_pre_pdcycles, double clk)
{
  double s_pre_pd_energy;

  s_pre_pd_energy = idd2p0 * vdd * s_pre_pdcycles * clk;
  return s_pre_pd_energy;
}

// Self-refresh active energy estimation (not including background energy)
double MemoryPowerModel::engy_sref(double idd6, double idd3n, double idd5,
                                   double vdd, double sref_cycles, double sref_ref_act_cycles,
                                   double sref_ref_pre_cycles, double spup_ref_act_cycles,
                                   double spup_ref_pre_cycles, double clk)
{
  double sref_energy;

  sref_energy = ((idd6 * sref_cycles) + ((idd5 - idd3n) * (sref_ref_act_cycles
                                                           + spup_ref_act_cycles + sref_ref_pre_cycles + spup_ref_pre_cycles)))
                * vdd * clk;
  return sref_energy;
}

// IO and Termination power calculation based on Micron Power Calculators
// Absolute power measures are obtained from Micron Power Calculator (mentioned in mW)
void MemoryPowerModel::io_term_power(MemorySpecification memSpec)
{
  MemTimingSpec& memTimingSpec     = memSpec.memTimingSpec;
  MemArchitectureSpec& memArchSpec = memSpec.memArchSpec;
  MemPowerSpec&  memPowerSpec      = memSpec.memPowerSpec;
  MemoryType& memoryType           = memSpec.memoryType;

  // For LPDDR/2/3 memories - IO Power depends on DRAM clock frequency
  // No ODT (Termination) in LPDDR/2/3 and DDR memories
  power.IO_power = 0.5 * pow(memPowerSpec.vdd2, 2.0) * memTimingSpec.clkMhz * 1000000;

  // LPDDR/2/3 IO Capacitance in mF
  double LPDDR_Cap  = 0.0000000045;
  double LPDDR2_Cap = 0.0000000025;
  double LPDDR3_Cap = 0.0000000018;

  // Conservative estimates based on Micron DDR2 Power Calculator
  if (memoryType == MemoryType::DDR2) {
    power.IO_power     = 1.5;    // in mW
    power.WR_ODT_power = 8.2;    // in mW
    if (memArchSpec.nbrOfRanks > 1) {
      power.TermRD_power = 13.1; // in mW
      power.TermWR_power = 14.6; // in mW
    }
  }
  // Conservative estimates based on Micron DDR3 Power Calculator
  else if (memoryType == MemoryType::DDR3) {
    power.IO_power     = 4.6;    // in mW
    power.WR_ODT_power = 21.2;   // in mW
    if (memArchSpec.nbrOfRanks > 1) {
      power.TermRD_power = 15.5; // in mW
      power.TermWR_power = 15.4; // in mW
    }
  }
  // Conservative estimates based on Micron DDR3 Power Calculator
  // using available termination resistance values from Micron DDR4 Datasheets
  else if (memoryType == MemoryType::DDR4) {
    power.IO_power     = 3.7;    // in mW
    power.WR_ODT_power = 17.0;   // in mW
    if (memArchSpec.nbrOfRanks > 1) {
      power.TermRD_power = 12.4; // in mW
      power.TermWR_power = 12.3; // in mW
    }
  }
  // LPDDR/2/3 and DDR memories only have IO Power (no ODT)
  // Conservative estimates based on Micron Mobile LPDDR2 Power Calculator
  else if (memoryType == MemoryType::LPDDR) {
    power.IO_power = LPDDR_Cap * power.IO_power;
  } else if (memoryType == MemoryType::LPDDR2)    {
    power.IO_power = LPDDR2_Cap * power.IO_power;
  } else if (memoryType == MemoryType::LPDDR3)    {
    power.IO_power = LPDDR3_Cap * power.IO_power;
  }
} // MemoryPowerModel::io_term_power
