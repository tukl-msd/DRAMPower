/*
 * Copyright (c) 2012-2014, TU Delft
 * Copyright (c) 2012-2014, TU Eindhoven
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
 * Authors: Subash Kannoth
 *          Luiza Correa
 *
 */

#include "DRAMPowerIF.h"
#include "DRAMPowerDDR3.h"
using namespace DRAMPower;
using namespace std;


DRAMPowerDDR3::DRAMPowerDDR3(MemSpecDDR3& memSpec, bool includeIoAndTermination,
                             const bool debug,
                             const bool writeToConsole,
                             const bool writeToFile,
                             const std::string &traceName):
    memSpec(memSpec),
    includeIoAndTermination(includeIoAndTermination)
{
    setupDebugManager(debug, writeToConsole, writeToFile, traceName);
    cmdListPerRank.resize(memSpec.numberOfRanks);
    total_cycles.resize(memSpec.numberOfRanks);
    window_cycles.resize(memSpec.numberOfRanks);

    rank_total_energy.resize(memSpec.numberOfRanks);
    rank_window_energy.resize(memSpec.numberOfRanks);

    rank_total_average_power.resize(memSpec.numberOfRanks);
    rank_window_average_power.resize(memSpec.numberOfRanks);

    energy.resize(memSpec.numberOfRanks);
    for (unsigned rank = 0; rank < memSpec.numberOfRanks; ++rank) {
        counters.push_back(CountersDDR3(memSpec));
        energy[rank].clearEnergy(memSpec.numberOfBanks);
        total_cycles[rank]             = 0.0;
        rank_total_energy[rank]        = 0.0;
        rank_total_average_power[rank] = 0.0;
    }
    total_trace_energy = 0.0;

}

void DRAMPowerDDR3::doCommand( int64_t timestamp, DRAMPower::MemCommand::cmds type, int rank,  int bank)
{
    DRAMPower::MemCommand cmd(timestamp,type, static_cast<unsigned>(rank), static_cast<unsigned>(bank));
    cmdList.push_back(cmd);
}

void DRAMPowerDDR3::calcEnergy()
{
    splitCmdList();
    for (unsigned rank = 0; rank < energy.size(); ++rank) {
        if (cmdListPerRank[rank].size() != 0) {
            updateCounters(true,rank);
            updateCycles(rank);
            if (includeIoAndTermination) calcIoTermEnergy(rank);
            bankEnergyCalc(energy[rank],counters[rank]);
            rankPowerCalc(rank);
        }
    }
    traceEnergyCalc();
}

void DRAMPowerDDR3::calcWindowEnergy(int64_t timestamp)
{
    for (unsigned rank = 0; rank < memSpec.numberOfRanks; ++rank) {
        doCommand(timestamp, MemCommand::NOP, rank, 0);
    }
    splitCmdList();
    for (unsigned rank = 0; rank < energy.size(); ++rank) {
        updateCounters(false, rank, timestamp);
        if (includeIoAndTermination) calcIoTermEnergy(rank);
        bankEnergyCalc(energy[rank],counters[rank]);
        updateCycles(rank);
        counters[rank].clearCounters(timestamp);
        rankPowerCalc(rank);
    }
    traceEnergyCalc();
}


double DRAMPowerDDR3::getEnergy() {
    return total_trace_energy;
}

double DRAMPowerDDR3::getPower()
{
    return total_trace_average_power;
}


void DRAMPowerDDR3::updateCounters(bool lastUpdate, unsigned rank, int64_t timestamp)
{
    counters[rank].getCommands(cmdListPerRank[rank], lastUpdate, timestamp);
    evaluateCommands(rank); //command list already modified
    cmdListPerRank[rank].clear();
}

// Used to analyse a given list of commands and identify command timings
// and memory state transitions
void DRAMPowerDDR3::evaluateCommands(unsigned rank)
{
    // for each command identify timestamp, type and bank
    for (auto cmd : cmdListPerRank[rank]) {
        // For command type
        int type = cmd.getType();
        // For command bank
        unsigned bank = cmd.getBank();
        // Command Issue timestamp in clock cycles (cc)
        int64_t timestamp = cmd.getTimeInt64();

        if (bank < memSpec.numberOfBanks) {
            switch(type) {
            case MemCommand::ACT : {
                counters[rank].handleAct(bank, timestamp);
                break;
            }
            case MemCommand::RD : {
                counters[rank].handleRd(bank, timestamp);
                break;
            }
            case MemCommand::WR : {
                counters[rank].handleWr(bank, timestamp);
                break;
            }
            case MemCommand::REF : {
                counters[rank].handleRef(bank, timestamp);
                break;
            }
            case MemCommand::PRE : {
                counters[rank].handlePre(bank, timestamp);
                break;
            }
            case MemCommand::PREA : {
                counters[rank].handlePreA(bank, timestamp);
                break;
            }
            case MemCommand::PDN_F_ACT : {
                counters[rank].handlePdnFAct(bank, timestamp);
                break;
            }
            case MemCommand::PDN_F_PRE : {
                counters[rank].handlePdnFPre(bank, timestamp);
                break;
            }
            case MemCommand::PDN_S_PRE : {
                counters[rank].handlePdnSPre(bank, timestamp);
                break;
            }
            case MemCommand::PUP_ACT : {
                counters[rank].handlePupAct(timestamp);
                break;
            }
            case MemCommand::PUP_PRE : {
                counters[rank].handlePupPre(timestamp);
                break;
            }
            case MemCommand::SREN : {
                counters[rank].handleSREn(bank, timestamp);
                break;
            }
            case MemCommand::SREX : {
                counters[rank].handleSREx(bank, timestamp);
                break;
            }
            case MemCommand::NOP :
            case MemCommand::END : {
                counters[rank].handleNopEnd(timestamp);
                break;
            }
            default: {
                PRINTDEBUGMESSAGE("Unknown command given, exiting.", timestamp, type, bank);
                exit(-1);
            }
            }//end switch
        } //end if bank<nbrOfBanks
        else PRINTDEBUGMESSAGE("Command given to non-existent bank", timestamp, type, bank);
    }//end for
} // Counters::evaluateCommands

//call the clear counters
void DRAMPowerDDR3::clearCountersWrapper()
{
    counters.clear();
}

//////////////////POWER CALCULATION////////////////////////


void DRAMPowerDDR3::Energy::clearEnergy(int64_t nbrofBanks)
{
    act_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    pre_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    read_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    write_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    ref_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    refb_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    act_stdby_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    pre_stdby_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    idle_energy_act_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    idle_energy_pre_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    f_act_pd_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    f_pre_pd_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    s_pre_pd_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    ref_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    sref_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    sref_ref_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    sref_ref_act_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    sref_ref_pre_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    spup_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    spup_ref_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    spup_ref_act_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    spup_ref_pre_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    pup_act_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    pup_pre_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    total_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
    window_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);

    read_io_energy      = 0.0;
    write_term_energy   = 0.0;
    io_term_energy      = 0.0;
}

//Split command list in sublists for each rank
void DRAMPowerDDR3::splitCmdList()
{
    for (size_t i = 0; i < cmdList.size(); ++i) {
        MemCommand& cmd = cmdList[i];
        unsigned rank = cmd.getRank();
        if (rank < memSpec.numberOfRanks) cmdListPerRank[rank].push_back(cmd);
    }
}


void DRAMPowerDDR3::bankEnergyCalc(Energy& e, CountersDDR3& c)
{
    const MemSpecDDR3::MemTimingSpec& t = memSpec.memTimingSpec;
    const MemSpecDDR3::BankWiseParams& bwPowerParams = memSpec.bwParams;
    const int64_t nbrofBanks               = memSpec.numberOfBanks;
    const MemSpecDDR3::MemPowerSpec& mps = memSpec.memPowerSpec;

    int64_t burstCc = memSpec.burstLength / memSpec.dataRate;

    // Using the number of cycles that at least one bank is active here
    // But the current iDDrho is less than iDD3N1
    double iDDrho = (static_cast<double>(bwPowerParams.bwPowerFactRho) / 100.0) * (mps.iDD3N - mps.iDD2N) + mps.iDD2N;
    double esharedActStdby = static_cast<double>(c.actcycles) * t.tCK * iDDrho * mps.vDD;
    // Fixed componenent for PASR
    double iDDsigma = (static_cast<double>(bwPowerParams.bwPowerFactSigma) / 100.0) * mps.iDD6;
    double esharedPASR = static_cast<double>(c.sref_cycles) * t.tCK * iDDsigma * mps.iDD6;
    // ione is Active background current for a single bank. When a single bank is Active
    //,all the other remainig (B-1) banks will consume  a current of iDDrho (based on factor Rho)
    // So to derrive ione we add (B-1)*iDDrho to the iDD3N and distribute it to each banks.
    double ione = (mps.iDD3N + (iDDrho * (static_cast<double>(nbrofBanks - 1)))) / (static_cast<double>(nbrofBanks));


    //Distribution of energy componets to each banks
    for (unsigned i = 0; i < nbrofBanks; i++) {
        e.act_energy_banks[i]         = static_cast<double>(c.numberofactsBanks[i] * t.tRAS) * t.tCK
                                                                           * (mps.iDD0 - ione) * mps.vDD;

        e.pre_energy_banks[i]          = static_cast<double>(c.numberofpresBanks[i] * t.tRP) * t.tCK
                                                                           * (mps.iDD0 - ione) * mps.vDD;

        e.read_energy_banks[i]         = static_cast<double>(c.numberofreadsBanks[i] * burstCc)* t.tCK
                                                                       * (mps.iDD4R - mps.iDD3N) * mps.vDD;

        e.write_energy_banks[i]        = static_cast<double>(c.numberofwritesBanks[i] * burstCc) * t.tCK
                                                                         * (mps.iDD4W - mps.iDD3N) * mps.vDD;

        e.ref_energy_banks[i]          = static_cast<double>(c.numberofrefs * t.tRFC) * t.tCK * (mps.iDD5
                                                     - mps.iDD3N) * mps.vDD / static_cast<double>(nbrofBanks);

        e.pre_stdby_energy_banks[i]    = static_cast<double>(c.precycles) * t.tCK * mps.iDD2N * mps.vDD
                                                                          / static_cast<double>(nbrofBanks);

        e.act_stdby_energy_banks[i]    = (static_cast<double>(c.actcyclesBanks[i]) * t.tCK * (mps.iDD3N
                                                       - iDDrho) * mps.vDD/ static_cast<double>(nbrofBanks))
                                                        + esharedActStdby / static_cast<double>(nbrofBanks);

        e.idle_energy_act_banks[i]     = static_cast<double>(c.idlecycles_act) * t.tCK * mps.iDD3N * mps.vDD
                                                                               / static_cast<double>(nbrofBanks);

        e.idle_energy_pre_banks[i]     = static_cast<double>(c.idlecycles_pre) * t.tCK * mps.iDD2N * mps.vDD
                                                                               / static_cast<double>(nbrofBanks);

        e.f_act_pd_energy_banks[i]     = static_cast<double>(c.f_act_pdcycles) * t.tCK * mps.iDD3P * mps.vDD
                                                                               / static_cast<double>(nbrofBanks);

        e.f_pre_pd_energy_banks[i]     = static_cast<double>(c.f_pre_pdcycles) * t.tCK * mps.iDD2P1 * mps.vDD
                                                                                / static_cast<double>(nbrofBanks);

        e.s_pre_pd_energy_banks[i]     = static_cast<double>(c.s_pre_pdcycles) * t.tCK * mps.iDD2P0 * mps.vDD
                                                                                / static_cast<double>(nbrofBanks);

        e.sref_energy_banks[i]         = engy_sref_banks(c, mps, esharedPASR, i);

        e.sref_ref_act_energy_banks[i] = static_cast<double>(c.sref_ref_act_cycles) * t.tCK * mps.iDD3P * mps.vDD
                                                                                    / static_cast<double>(nbrofBanks);

        e.sref_ref_pre_energy_banks[i] = static_cast<double>(c.sref_ref_pre_cycles) * t.tCK * mps.iDD2P0 * mps.vDD
                                                                                     / static_cast<double>(nbrofBanks);

        e.sref_ref_energy_banks[i]     = e.sref_ref_act_energy_banks[i]
                                            + e.sref_ref_pre_energy_banks[i];

        e.spup_energy_banks[i] = static_cast<double>(c.spup_cycles) * t.tCK * mps.iDD2N * mps.vDD
                                                                    / static_cast<double>(nbrofBanks);

        e.spup_ref_act_energy_banks[i] = static_cast<double>(c.spup_ref_act_cycles) * t.tCK * mps.iDD3N * mps.vDD
                                                                                    / static_cast<double>(nbrofBanks);

        e.spup_ref_pre_energy_banks[i] = static_cast<double>(c.spup_ref_pre_cycles) * t.tCK * mps.iDD2N * mps.vDD
                                                                                    / static_cast<double>(nbrofBanks);

        e.spup_ref_energy_banks[i]     = e.spup_ref_act_energy_banks[i]
                                            + e.spup_ref_pre_energy_banks[i];

        e.pup_act_energy_banks[i]      = static_cast<double>(c.pup_act_cycles) * t.tCK * mps.iDD3N * mps.vDD
                                                                               / static_cast<double>(nbrofBanks);

        e.pup_pre_energy_banks[i]      = static_cast<double>(c.pup_pre_cycles) * t.tCK * mps.iDD2N * mps.vDD
                                                                               / static_cast<double>(nbrofBanks);
    }


    // Calculate total energy per bank.
    for (unsigned i = 0; i < nbrofBanks; i++) {
        e.window_energy_banks[i] = e.act_energy_banks[i]
                + e.pre_energy_banks[i]
                + e.read_energy_banks[i]
                + e.ref_energy_banks[i]
                + e.write_energy_banks[i]
                + e.act_stdby_energy_banks[i]
                + e.pre_stdby_energy_banks[i]
                + e.sref_energy_banks[i]
                + e.f_pre_pd_energy_banks[i]
                + e.f_act_pd_energy_banks[i]
                + e.s_pre_pd_energy_banks[i]
                + e.sref_ref_energy_banks[i]
                + e.spup_ref_energy_banks[i];
        e.total_energy_banks[i]  += e.window_energy_banks[i];
    }

//    // Calculate total energy for all banks.
//    energy.window_energy = sum(energy.total_energy_banks) + energy.io_term_energy;

//    energy.window_average_power = energy.window_energy / (static_cast<double>(window_cycles) * t.tCK);

//    window_cycles = c.actcycles + c.precycles +
//            c.f_act_pdcycles + c.f_pre_pdcycles +
//            + c.s_pre_pdcycles + c.sref_cycles
//            + c.sref_ref_act_cycles + c.sref_ref_pre_cycles +
//            c.spup_ref_act_cycles + c.spup_ref_pre_cycles;

//    total_cycles += window_cycles;

//    energy.total_energy += energy.window_energy;

//    // Calculate the average power consumption
//    energy.average_power = energy.total_energy / (static_cast<double>(total_cycles) * t.tCK);

} // DRAMPowerDDR3::bankPowerCalc

void DRAMPowerDDR3::updateCycles(unsigned rank)
{
    const CountersDDR3& c = counters[rank];
    window_cycles[rank] = c.actcycles + c.precycles +
                          c.f_act_pdcycles + c.f_pre_pdcycles +
                          c.s_pre_pdcycles + c.sref_cycles +
                          c.sref_ref_act_cycles + c.sref_ref_pre_cycles +
                          c.spup_ref_act_cycles + c.spup_ref_pre_cycles;
    total_cycles[rank]  += window_cycles[rank];
}


void DRAMPowerDDR3::rankPowerCalc(unsigned rank)
{
    rank_window_energy[rank] = energy[rank].io_term_energy
                             + sum(energy[rank].window_energy_banks);

    rank_window_average_power[rank] = rank_window_energy[rank] / (window_cycles[rank]
                                                                  * memSpec.memTimingSpec.tCK);

    rank_total_energy[rank] += rank_window_energy[rank];
    rank_total_average_power[rank] = rank_total_energy[rank] / (total_cycles[rank]
                                                                * memSpec.memTimingSpec.tCK);
}

void DRAMPowerDDR3::traceEnergyCalc()
{
    window_trace_energy = 0.0;

    window_trace_energy += sum(rank_window_energy);

    total_trace_energy = sum(rank_total_energy);

    window_trace_average_power = sum(rank_window_average_power) / rank_window_average_power.size();

    total_trace_average_power = sum(rank_total_average_power) / rank_total_average_power.size();
}





// Self-refresh active energy estimation per banks
double DRAMPowerDDR3::engy_sref_banks(const CountersDDR3& c, const MemSpecDDR3::MemPowerSpec& mps, double esharedPASR, unsigned bnkIdx)
{

    const MemSpecDDR3::BankWiseParams& bwPowerParams = memSpec.bwParams;
    const MemSpecDDR3::MemTimingSpec& t                 = memSpec.memTimingSpec;

    // Bankwise Self-refresh energy
    double sref_energy_banks;
    // Dynamic componenents for PASR energy varying based on PASR mode
    double iDDsigmaDynBanks;
    double pasr_energy_dyn;
    // This component is distributed among all banks
    double sref_energy_shared;
    //Is PASR Active
    if (bwPowerParams.flgPASR) {
        sref_energy_shared = (((mps.iDD5 - mps.iDD3N) * (static_cast<double>(c.sref_ref_act_cycles
                             + c.spup_ref_act_cycles + c.sref_ref_pre_cycles + c.spup_ref_pre_cycles)))
                                                           * mps.vDD * t.tCK) / memSpec.numberOfBanks;
        //if the bank is active under current PASR mode
        if (bwPowerParams.isBankActiveInPasr(bnkIdx)) {
            // Distribute the sref energy to the active banks
            iDDsigmaDynBanks = (static_cast<double>(100 - bwPowerParams.bwPowerFactSigma) /
                          (100.0 * static_cast<double>(memSpec.numberOfBanks))) * mps.iDD6;

            pasr_energy_dyn = mps.vDD * iDDsigmaDynBanks * static_cast<double>(c.sref_cycles);
            // Add the static components
            sref_energy_banks = sref_energy_shared + pasr_energy_dyn + (esharedPASR
                                      / static_cast<double>(memSpec.numberOfBanks));

        }else {
            sref_energy_banks = (esharedPASR /static_cast<double>(memSpec.numberOfBanks));
        }
    }
    //When PASR is not active total all the banks are in Self-Refresh. Thus total Self-Refresh energy is distributed across all banks
    else {
        sref_energy_banks = (((mps.iDD6 * static_cast<double>(c.sref_cycles)) + ((mps.iDD5 - mps.iDD3N)
                             * static_cast<double>(c.sref_ref_act_cycles + c.spup_ref_act_cycles
                             + c.sref_ref_pre_cycles + c.spup_ref_pre_cycles))) * mps.vDD * t.tCK)
                             / static_cast<double>(memSpec.numberOfBanks);
    }
    return sref_energy_banks;
}




void DRAMPowerDDR3::calcIoTermEnergy(unsigned rank)
{

    const MemSpecDDR3::MemTimingSpec& t                 = memSpec.memTimingSpec;
    const CountersDDR3& c = counters[rank];

    IO_power     = memSpec.memPowerSpec.ioPower;    // in W
    WR_ODT_power = memSpec.memPowerSpec.wrOdtPower; // in W


    if (memSpec.memPowerSpec.capacitance != 0.0) {
        // If capacity is given, then IO Power depends on DRAM clock frequency.
        IO_power = memSpec.memPowerSpec.capacitance * 0.5 * pow(memSpec.memPowerSpec.vDD, 2.0)
                * memSpec.memTimingSpec.fCKMHz * 1000000;
    }

    // memSpec.width represents the number of data (dq) pins.
    // 1 DQS pin is associated with every data byte
    int64_t dqPlusDqsBits = memSpec.bitWidth + memSpec.bitWidth / 8;
    // 1 DQS and 1 DM pin is associated with every data byte
    int64_t dqPlusDqsPlusMaskBits = memSpec.bitWidth + memSpec.bitWidth / 8 + memSpec.bitWidth / 8;
    // Size of one clock period for the data bus.
    double ddtRPeriod = t.tCK / static_cast<double>(memSpec.dataRate);

    // Read IO power is consumed by each DQ (data) and DQS (data strobe) pin
    energy[rank].read_io_energy = static_cast<double>(sum(c.numberofreadsBanks) * memSpec.burstLength)
            * ddtRPeriod * IO_power * static_cast<double>(dqPlusDqsBits);


    // Write ODT power is consumed by each DQ (data), DQS (data strobe) and DM
    energy[rank].write_term_energy = static_cast<double>(sum(c.numberofwritesBanks) * memSpec.burstLength)
            * ddtRPeriod * WR_ODT_power * static_cast<double>(dqPlusDqsPlusMaskBits);

    // Sum of all IO and termination energy
    energy[rank].io_term_energy = energy[rank].read_io_energy + energy[rank].write_term_energy;

}




void DRAMPowerDDR3::powerPrint()
{
    const char eUnit[] = " pJ";
    const int64_t nbrofBanks = memSpec.numberOfBanks;
    ios_base::fmtflags flags = cout.flags();
    streamsize precision = cout.precision();
    cout.precision(2);
    for (unsigned rank = 0; rank < energy.size(); ++rank) {
        const CountersDDR3& c = counters[rank];
        const Energy& e = energy[rank];
        cout << endl << "* Commands to rank " << rank << ":" << endl;

        cout << endl << "  #ACT commands: " << sum(counters[rank].numberofactsBanks)
             << endl << "  #RD + #RDA commands: " << sum(counters[rank].numberofreadsBanks)
             << endl << "  #WR + #WRA commands: " << sum(c.numberofwritesBanks)
             << endl << "  #PRE (+ PREA) commands: " << sum(c.numberofpresBanks)
             << endl << "  #REF commands: " << c.numberofrefs;

        cout << endl;

    for (unsigned i = 0; i < nbrofBanks; i++) {
        cout << endl << " ## @Rank " << rank << " @ Bank " << i << fixed
             << endl << "  ACT Cmd Energy: " << e.act_energy_banks[i] << eUnit
             << endl << "  PRE Cmd Energy: " << e.pre_energy_banks[i] << eUnit
             << endl << "  RD Cmd Energy: " << e.read_energy_banks[i] << eUnit
             << endl << "  WR Cmd Energy: " << e.write_energy_banks[i] << eUnit
             << endl << "  Auto-Refresh Energy: " << e.ref_energy_banks[i] << eUnit
             << endl << "  ACT Stdby Energy: " <<  e.act_stdby_energy_banks[i] << eUnit
             << endl << "  PRE Stdby Energy: " << e.pre_stdby_energy_banks[i] << eUnit
             << endl << "  Active Idle Energy: "<<  e.idle_energy_act_banks[i] << eUnit
             << endl << "  Precharge Idle Energy: "<<  e.idle_energy_pre_banks[i] << eUnit
             << endl << "  Fast-Exit Active Power-Down Energy: "<<  e.f_act_pd_energy_banks[i] << eUnit
             << endl << "  Fast-Exit Precharged Power-Down Energy: "<<  e.f_pre_pd_energy_banks[i] << eUnit
             << endl << "  Self-Refresh Energy: "<<  e.sref_energy_banks[i] << eUnit
             << endl << "  Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: "<<  e.sref_ref_act_energy_banks[i] << eUnit
             << endl << "  Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: " <<  e.sref_ref_pre_energy_banks[i] << eUnit
             << endl << "  Self-Refresh Power-Up Energy: "<< e.spup_energy_banks[i] << eUnit
             << endl << "  Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: "<<  e.spup_ref_act_energy_banks[i] << eUnit
             << endl << "  Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: "<<  e.spup_ref_pre_energy_banks[i] << eUnit
             << endl << "  Active Power-Up Energy: "<<  e.pup_act_energy_banks[i] << eUnit
             << endl << "  Precharged Power-Up Energy: "<< e.pup_pre_energy_banks[i] << eUnit
             << endl << "  Total Energy of Bank: " << e.total_energy_banks[i] << eUnit
             << endl;
    }

    if (includeIoAndTermination) {
        cout << endl << "RD I/O Energy: " << e.read_io_energy << eUnit << endl;
        // No Termination for LPDDR/2/3 and DDR memories
        cout << "WR Termination Energy: " << e.write_term_energy << eUnit << endl;
    }

    cout << endl
         << endl << "  Rank " << rank << " Energy : "<< rank_total_energy[rank] << eUnit
         << endl << "  Rank " << rank << " Average Power : " << rank_total_average_power[rank] << " mW" << endl
         << endl << "  Cycles: " << total_cycles[rank];
    }

    cout << endl << "----------------------------------------"
         << endl << "  Total Trace Energy : "  << total_trace_energy << eUnit
         << endl << "  Total Average Power : " << total_trace_average_power << " mW"
         << endl << "----------------------------------------" << endl;

    cout.flags(flags);
    cout.precision(precision);
}

void DRAMPowerDDR3::setupDebugManager(const bool debug __attribute__((unused)),
                                    const bool writeToConsole __attribute__((unused)),
                                    const bool writeToFile __attribute__((unused)),
                                    const std::string &traceName __attribute__((unused)))

{
#ifndef NDEBUG
    auto &dbg = DebugManager::getInstance();
    dbg.debug = debug;
    dbg.writeToConsole = writeToConsole;
    dbg.writeToFile = writeToFile;
    if (dbg.writeToFile && (traceName!=""))
        dbg.openDebugFile(traceName + ".txt");
#endif
}

