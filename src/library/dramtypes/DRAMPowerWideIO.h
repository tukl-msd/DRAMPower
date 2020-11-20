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
 * Authors: Matthias Jung
 *          Omar Naji
 *          Subash Kannoth
 *          Éder F. Zulian
 *          Felipe S. Prado
 *          Luiza Correa
 *
 */

#ifndef WideIO_H
#define WideIO_H

#include <vector>
#include <stdint.h>
#include <cmath>  // For pow


#include "../memspec/MemSpecWideIO.h"
#include "../MemCommand.h"
#include "../counters/Counters.h"
#include "../counters/CountersWideIO.h"
#include "DRAMPowerIF.h"
#include "../DebugManager.h"

namespace DRAMPower {


class DRAMPowerWideIO final : public DRAMPowerIF
{
public:
    DRAMPowerWideIO(MemSpecWideIO &memSpec,
                    const bool includeIoAndTermination __attribute__((unused))=false,
                    const bool debug __attribute__((unused))=false,
                    const bool writeToConsole __attribute__((unused))=false,
                    const bool writeToFile __attribute__((unused))=false,
                    const std::string &traceName __attribute__((unused))="");

    ~DRAMPowerWideIO(){}

    //////Interface methods

    void doCommand( int64_t timestamp,
                    DRAMPower::MemCommand::cmds type,
                    int rank,
                    int bank) override;

    void setupDebugManager(const bool debug __attribute__((unused))=false,
                           const bool writeToConsole __attribute__((unused))=false,
                           const bool writeToFile __attribute__((unused))=false,
                           const std::string &traceName __attribute__((unused))="") override;

    void calcEnergy() override;

    void calcWindowEnergy(int64_t timestamp) override;

    double getEnergy() override;

    double getPower() override;

    void clearCountersWrapper() override;

    void powerPrint() override;

    void updateCounters(bool lastUpdate, unsigned rank, int64_t timestamp = 0);

    struct Energy {

        // Total energy of all activates
        std::vector<double> act_energy_banks;

        // Total energy of all precharges
        std::vector<double> pre_energy_banks;

        // Total energy of all reads
        std::vector<double> read_energy_banks;

        // Total energy of all writes
        std::vector<double> write_energy_banks;

        // Total energy of all refreshes
        std::vector<double> ref_energy_banks;

        // Bankwise refresh energy
        std::vector<double> refb_energy_banks;

        // Total background energy of all active standby cycles
        std::vector<double> act_stdby_energy_banks;

        // Total background energy of all precharge standby cycles
        std::vector<double> pre_stdby_energy_banks;

        // Total energy of idle cycles in the active mode
        std::vector<double> idle_energy_act_banks;

        // Total energy of idle cycles in the precharge mode
        std::vector<double> idle_energy_pre_banks;

        // Window energy banks
        std::vector<double> window_energy_banks;

        // Total energy banks
        std::vector<double> total_energy_banks;

        // Energy consumed in active/precharged fast/slow-exit modes
        std::vector<double> f_act_pd_energy_banks;

        std::vector<double> f_pre_pd_energy_banks;

        // Energy consumed in self-refresh mode
        std::vector<double> sref_energy_banks;

        // Energy consumed in auto-refresh during self-refresh mode
        std::vector<double> sref_ref_energy_banks;

        std::vector<double> sref_ref_act_energy_banks;

        std::vector<double> sref_ref_pre_energy_banks;

        // Energy consumed in powering-up from self-refresh mode
        std::vector<double> spup_energy_banks;

        // Energy consumed in auto-refresh during self-refresh power-up
        std::vector<double> spup_ref_energy_banks;

        std::vector<double> spup_ref_act_energy_banks;

        std::vector<double> spup_ref_pre_energy_banks;

        // Energy consumed in powering-up from active/precharged power-down modes
        std::vector<double> pup_act_energy_banks;

        std::vector<double> pup_pre_energy_banks;

        // Energy consumed by IO and Termination
        double read_io_energy;     // Read IO Energy
        double write_term_energy;  // Write Termination Energy
        // Total IO and Termination Energy
        double io_term_energy;

        void clearEnergy(int64_t nbrofBanks);
    };

    std::vector<std::vector<Energy>> energy;

private:
    MemSpecWideIO memSpec;

    std::vector<std::vector<DRAMPower::MemCommand>> cmdListPerRank;

    std::vector<CountersWideIO> counters;

    // Cycles
    std::vector<int64_t> total_cycles;
    std::vector<int64_t> window_cycles;

    //Rank Energy
    std::vector<double>  rank_total_energy;
    std::vector<double>  rank_window_energy;

    //Rank Power
    std::vector<double>  rank_total_average_power;
    std::vector<double>  rank_window_average_power;

    //Total Energy
    double window_trace_energy;
    double total_trace_energy;

    double window_trace_average_power;
    double total_trace_average_power;

    bool includeIoAndTermination;

    void evaluateCommands(unsigned rank);

    void splitCmdList();

    void bankEnergyCalc(DRAMPowerWideIO::Energy& e, CountersWideIO& c, MemSpecWideIO::MemPowerSpec& mps);

    void updateCycles(unsigned rank);

    void rankPowerCalc(unsigned rank);

    void traceEnergyCalc();

    // To calculate IO and Termination Energy
    void io_term_power();

    void calcIoTermEnergy(unsigned rank);

    template <typename T> T sum(const std::vector<T> vec) const { return std::accumulate(vec.begin(), vec.end(), static_cast<T>(0)); }

};
}
#endif // WideIO_H
