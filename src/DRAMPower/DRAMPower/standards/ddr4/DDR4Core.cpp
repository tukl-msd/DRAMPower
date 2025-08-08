#include "DDR4Core.h"

namespace DRAMPower {

void DDR4Core::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
    bank.counter.act++;
    bank.bankState = Bank::BankState::BANK_ACTIVE;

    bank.cycles.act.start_interval(timestamp);
    
    rank.cycles.act.start_interval_if_not_running(timestamp);
    //rank.cycles.pre.close_interval(timestamp);
}

void DDR4Core::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
    // If statement necessary for core power calculation
    // bank.counter.pre doesn't correspond to the number of pre commands
    // It corresponds to the number of state transisitons to the pre state
    if (bank.bankState == Bank::BankState::BANK_PRECHARGED) return;
    bank.counter.pre++;

    bank.bankState = Bank::BankState::BANK_PRECHARGED;
    bank.cycles.act.close_interval(timestamp);
    bank.latestPre = timestamp;                                                // used for earliest power down calculation
    if ( !rank.isActive(timestamp) )                                            // stop rank active interval if no more banks active
    {
        // active counter increased if at least 1 bank is active, precharge counter increased if all banks are precharged
        rank.cycles.act.close_interval(timestamp);
        //rank.cycles.pre.start_interval_if_not_running(timestamp);             
    }
}

void DDR4Core::handlePreAll(Rank &rank, timestamp_t timestamp) {
    for (auto &bank: rank.banks) 
        handlePre(rank, bank, timestamp);
}

void DDR4Core::handleRefAll(Rank &rank, timestamp_t timestamp) {
    auto timestamp_end = timestamp + m_memSpec.memTimingSpec.tRFC;
    rank.endRefreshTime = timestamp_end;
    rank.cycles.act.start_interval_if_not_running(timestamp);
    //rank.cycles.pre.close_interval(timestamp);
    for (auto& bank : rank.banks) {
        bank.bankState = Bank::BankState::BANK_ACTIVE;
        
        ++bank.counter.refAllBank;
        bank.cycles.act.start_interval_if_not_running(timestamp);


        bank.refreshEndTime = timestamp_end;                                    // used for earliest power down calculation

        // Execute implicit pre-charge at refresh end
        m_implicitCommandInserter.addImplicitCommand(timestamp_end, [&bank, &rank, timestamp_end]() {
            bank.bankState = Bank::BankState::BANK_PRECHARGED;
            bank.cycles.act.close_interval(timestamp_end);
            // stop rank active interval if no more banks active
            if (!rank.isActive(timestamp_end))                                  // stop rank active interval if no more banks active
            {
                rank.cycles.act.close_interval(timestamp_end);
                //rank.cycles.pre.start_interval(timestamp_end);
            }
        });
    }

    // Required for precharge power-down
}

void DDR4Core::handleRead(Rank&, Bank &bank, timestamp_t) {
    ++bank.counter.reads;
}

void DDR4Core::handleReadAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
    ++bank.counter.readAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    auto minReadActiveTime = timestamp + m_memSpec.prechargeOffsetRD;

    auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandInserter.addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
        this->handlePre(rank, bank, delayed_timestamp);
    });
}

void DDR4Core::handleWrite(Rank&, Bank &bank, timestamp_t) {
    ++bank.counter.writes;
}

void DDR4Core::handleWriteAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
    ++bank.counter.writeAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    auto minWriteActiveTime =  timestamp + m_memSpec.prechargeOffsetWR;

    auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandInserter.addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
        this->handlePre(rank, bank, delayed_timestamp);
    });
}

void DDR4Core::handleSelfRefreshEntry(Rank &rank, timestamp_t timestamp) {
    // Issue implicit refresh
    handleRefAll(rank, timestamp);
    // Handle self-refresh entry after tRFC
    auto timestampSelfRefreshStart = timestamp + m_memSpec.memTimingSpec.tRFC;
    m_implicitCommandInserter.addImplicitCommand(timestampSelfRefreshStart, [&rank, timestampSelfRefreshStart]() {
        rank.counter.selfRefresh++;
        rank.cycles.sref.start_interval(timestampSelfRefreshStart);
        rank.memState = MemState::SREF;
    });
}

void DDR4Core::handleSelfRefreshExit(Rank &rank, timestamp_t timestamp) {
    assert(rank.memState == MemState::SREF);                                    // check for previous SelfRefreshEntry
    rank.cycles.sref.close_interval(timestamp);                                 // Duration between entry and exit
    rank.memState = MemState::NOT_IN_PD;
}

void DDR4Core::handlePowerDownActEntry(Rank &rank, timestamp_t timestamp) {
    auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
    auto entryTime = std::max(timestamp, earliestPossibleEntry);
    m_implicitCommandInserter.addImplicitCommand(entryTime, [&rank, entryTime]() {
        rank.memState = MemState::PDN_ACT;
        rank.cycles.powerDownAct.start_interval(entryTime);
        rank.cycles.act.close_interval(entryTime);
        //rank.cycles.pre.close_interval(entryTime);
        for (auto & bank : rank.banks) {
            bank.cycles.act.close_interval(entryTime);
        }
    });
}

void DDR4Core::handlePowerDownActExit(Rank &rank, timestamp_t timestamp) {
    assert(rank.memState == MemState::PDN_ACT);
    auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
    auto exitTime = std::max(timestamp, earliestPossibleExit);

    m_implicitCommandInserter.addImplicitCommand(exitTime, [&rank, exitTime]() {
        rank.memState = MemState::NOT_IN_PD;
        rank.cycles.powerDownAct.close_interval(exitTime);

        // Activate banks that were active prior to PDA
        for (auto & bank : rank.banks)
        {
            if (bank.bankState==Bank::BankState::BANK_ACTIVE)
            {
                bank.cycles.act.start_interval(exitTime);
            }
        }
        // Activate rank if at least one bank is active
        // At least one bank must be active for PDA -> remove if statement?
        if(rank.isActive(exitTime))
            rank.cycles.act.start_interval(exitTime); 

    });
}

void DDR4Core::handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp) {
    auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
    auto entryTime = std::max(timestamp, earliestPossibleEntry);

    m_implicitCommandInserter.addImplicitCommand(entryTime, [&rank, entryTime]() {
        for (auto &bank : rank.banks)
            bank.cycles.act.close_interval(entryTime);
        rank.memState = MemState::PDN_PRE;
        rank.cycles.powerDownPre.start_interval(entryTime);
        //rank.cycles.pre.close_interval(entryTime);
        rank.cycles.act.close_interval(entryTime);
    });
}

void DDR4Core::handlePowerDownPreExit(Rank &rank, timestamp_t timestamp) {
    // The computation is necessary to exit at the earliest timestamp (upon entry)
    auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
    auto exitTime = std::max(timestamp, earliestPossibleExit);

    m_implicitCommandInserter.addImplicitCommand(exitTime, [&rank, exitTime]() {
        rank.memState = MemState::NOT_IN_PD;
        rank.cycles.powerDownPre.close_interval(exitTime);

        // Precharge banks that were precharged prior to PDP
        for (auto & bank : rank.banks)
        {
            if (bank.bankState==Bank::BankState::BANK_ACTIVE)
            {
                bank.cycles.act.start_interval(exitTime);
            }
        }
        // Precharge rank if all banks are precharged
        // At least one bank must be precharged for PDP -> remove if statement?
        // If statement ensures right state diagramm traversal
        //if(!rank.isActive(exitTime))
        //    rank.cycles.pre.start_interval(exitTime); 
    });
}

timestamp_t DDR4Core::earliestPossiblePowerDownEntryTime(Rank & rank) const {
    timestamp_t entryTime = 0;

    for (const auto & bank : rank.banks) {
        entryTime = std::max({ entryTime,
                                bank.counter.act == 0 ? 0 :  bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRCD,
                                bank.counter.pre == 0 ? 0 : bank.latestPre + m_memSpec.memTimingSpec.tRP,
                                bank.refreshEndTime
                                });
    }

    return entryTime;
}

void DDR4Core::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
    // resize banks and ranks
    stats.bank.resize(m_memSpec.numberOfBanks * m_memSpec.numberOfRanks);
    stats.rank_total.resize(m_memSpec.numberOfRanks);

    auto simulation_duration = timestamp;
    for (size_t i = 0; i < m_memSpec.numberOfRanks; ++i) {
        const Rank &rank = m_ranks[i];
        size_t bank_offset = i * m_memSpec.numberOfBanks;
        for (size_t j = 0; j < m_memSpec.numberOfBanks; ++j) {
            stats.bank[bank_offset + j].counter = rank.banks[j].counter;
            stats.bank[bank_offset + j].cycles.act =
                rank.banks[j].cycles.act.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.selfRefresh =
                rank.cycles.sref.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.powerDownAct =
                rank.cycles.powerDownAct.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.powerDownPre =
                rank.cycles.powerDownPre.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.pre =
                simulation_duration - (stats.bank[bank_offset + j].cycles.act +
                                        rank.cycles.powerDownAct.get_count_at(timestamp) +
                                        rank.cycles.powerDownPre.get_count_at(timestamp) +
                                        rank.cycles.sref.get_count_at(timestamp));
        }
        stats.rank_total[i].cycles.act = rank.cycles.act.get_count_at(timestamp);
        stats.rank_total[i].cycles.powerDownAct = rank.cycles.powerDownAct.get_count_at(timestamp);
        stats.rank_total[i].cycles.powerDownPre = rank.cycles.powerDownPre.get_count_at(timestamp);
        stats.rank_total[i].cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp);
        stats.rank_total[i].cycles.pre = simulation_duration - 
        (
            stats.rank_total[i].cycles.act +
            stats.rank_total[i].cycles.powerDownAct +
            stats.rank_total[i].cycles.powerDownPre +
            stats.rank_total[i].cycles.selfRefresh
        );
        //stats.rank_total[i].cycles.pre = rank.cycles.pre.get_count_at(timestamp);
    }
}

} // namespace DRAMPower