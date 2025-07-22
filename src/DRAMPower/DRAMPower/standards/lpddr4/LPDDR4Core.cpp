#include "LPDDR4Core.h"

namespace DRAMPower {

void LPDDR4Core::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
    bank.counter.act++;

    bank.cycles.act.start_interval(timestamp);

    if (!rank.isActive(timestamp)) {
        rank.cycles.act.start_interval(timestamp);
    }

    bank.bankState = Bank::BankState::BANK_ACTIVE;
}

void LPDDR4Core::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
    if (bank.bankState == Bank::BankState::BANK_PRECHARGED)
        return;

    bank.counter.pre++;
    bank.cycles.act.close_interval(timestamp);
    bank.latestPre = timestamp;
    bank.bankState = Bank::BankState::BANK_PRECHARGED;

    if (!rank.isActive(timestamp)) {
        rank.cycles.act.close_interval(timestamp);
    }
}

void LPDDR4Core::handlePreAll(Rank &rank, timestamp_t timestamp) {
    for (auto &bank: rank.banks) {
        handlePre(rank, bank, timestamp);
    }
}

void LPDDR4Core::handleRefreshOnBank(Rank &rank, Bank &bank, timestamp_t timestamp, uint64_t timing, uint64_t &counter) {
    ++counter;

    if (!rank.isActive(timestamp)) {
        rank.cycles.act.start_interval(timestamp);
    }

    bank.bankState = Bank::BankState::BANK_ACTIVE;
    auto timestamp_end = timestamp + timing;
    bank.refreshEndTime = timestamp_end;
    if (!bank.cycles.act.is_open())
        bank.cycles.act.start_interval(timestamp);

    // Execute implicit pre-charge at refresh end
    m_implicitCommandInserter.addImplicitCommand(timestamp_end, [&bank, &rank, timestamp_end]() {
        bank.bankState = Bank::BankState::BANK_PRECHARGED;
        bank.cycles.act.close_interval(timestamp_end);

        if (!rank.isActive(timestamp_end)) {
            rank.cycles.act.close_interval(timestamp_end);
        }
    });
}

void LPDDR4Core::handleRefAll(Rank &rank, timestamp_t timestamp) {
    for (auto &bank: rank.banks) {
        handleRefreshOnBank(rank, bank, timestamp, m_memSpec.memTimingSpec.tRFC, bank.counter.refAllBank);
    }
    rank.endRefreshTime = timestamp + m_memSpec.memTimingSpec.tRFC;
}

void LPDDR4Core::handleRefPerBank(Rank &rank, Bank &bank, timestamp_t timestamp) {
    handleRefreshOnBank(rank, bank, timestamp, m_memSpec.memTimingSpec.tRFCPB, bank.counter.refPerBank);
}

void LPDDR4Core::handleSelfRefreshEntry(Rank &rank, timestamp_t timestamp) {
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

void LPDDR4Core::handleSelfRefreshExit(Rank &rank, timestamp_t timestamp) {
    assert(rank.memState == MemState::SREF);
    rank.cycles.sref.close_interval(timestamp);  // Duration start between entry and exit
    rank.memState = MemState::NOT_IN_PD;
}

void LPDDR4Core::handlePowerDownActEntry(Rank &rank, timestamp_t timestamp) {
    auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
    auto entryTime = std::max(timestamp, earliestPossibleEntry);
    m_implicitCommandInserter.addImplicitCommand(entryTime, [&rank, entryTime]() {
        rank.cycles.powerDownAct.start_interval(entryTime);
        rank.memState = MemState::PDN_ACT;
        if (rank.cycles.act.is_open()) {
            rank.cycles.act.close_interval(entryTime);
        }
        for (auto &bank: rank.banks) {
            if (bank.cycles.act.is_open()) {
                bank.cycles.act.close_interval(entryTime);
            }
        }
    });
}

void LPDDR4Core::handlePowerDownActExit(Rank &rank, timestamp_t timestamp) {
    auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
    auto exitTime = std::max(timestamp, earliestPossibleExit);

    m_implicitCommandInserter.addImplicitCommand(exitTime, [&rank, exitTime]() {
        rank.memState = MemState::NOT_IN_PD;
        rank.cycles.powerDownAct.close_interval(exitTime);

        bool rank_active = false;

        for (auto & bank : rank.banks) {
            if (bank.counter.act != 0 && bank.cycles.act.get_end() == rank.cycles.powerDownAct.get_start()) {
                rank_active = true;
                bank.cycles.act.start_interval(exitTime);
            }
        }

        if (rank_active) {
            rank.cycles.act.start_interval(exitTime);
        }

    });
};

void LPDDR4Core::handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp) {
    auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
    auto entryTime = std::max(timestamp, earliestPossibleEntry);
    m_implicitCommandInserter.addImplicitCommand(entryTime, [&rank, entryTime]() {
        rank.cycles.powerDownPre.start_interval(entryTime);
        rank.memState = MemState::PDN_PRE;
    });
}

void LPDDR4Core::handlePowerDownPreExit(Rank &rank, timestamp_t timestamp) {
    auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
    auto exitTime = std::max(timestamp, earliestPossibleExit);

    m_implicitCommandInserter.addImplicitCommand(exitTime, [&rank, exitTime]() {
        rank.memState = MemState::NOT_IN_PD;
        rank.cycles.powerDownPre.close_interval(exitTime);
    });
}

void LPDDR4Core::handleRead(Rank&, Bank &bank, timestamp_t) {
    ++bank.counter.reads;
}

void LPDDR4Core::handleWrite(Rank&, Bank &bank, timestamp_t) {
    ++bank.counter.writes;
}

void LPDDR4Core::handleReadAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
    ++bank.counter.readAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    auto minReadActiveTime = timestamp + m_memSpec.prechargeOffsetRD;

    auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandInserter.addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
        this->handlePre(rank, bank, delayed_timestamp);
    });
}

void LPDDR4Core::handleWriteAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
    ++bank.counter.writeAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    auto minWriteActiveTime = timestamp + m_memSpec.prechargeOffsetWR;

    auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandInserter.addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
        this->handlePre(rank, bank, delayed_timestamp);
    });
}

timestamp_t LPDDR4Core::earliestPossiblePowerDownEntryTime(Rank & rank) const {
    timestamp_t entryTime = 0;

    for (const auto & bank : rank.banks) {
        entryTime = std::max({ entryTime,
                                bank.counter.act == 0 ? 0 :  bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRCD,
                                bank.counter.pre == 0 ? 0 : bank.latestPre + m_memSpec.memTimingSpec.tRP,
                                bank.refreshEndTime
                                });
    }

    return entryTime;
};

void LPDDR4Core::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
    stats.bank.resize(m_memSpec.numberOfBanks * m_memSpec.numberOfRanks);
    stats.rank_total.resize(m_memSpec.numberOfRanks);

    auto simulation_duration = timestamp;
    for (size_t i = 0; i < m_memSpec.numberOfRanks; ++i) {
        const Rank &rank = m_ranks[i];
        size_t bank_offset = i * m_memSpec.numberOfBanks;

        for (std::size_t j = 0; j < m_memSpec.numberOfBanks; ++j) {
            stats.bank[bank_offset + j].counter = rank.banks[j].counter;
            stats.bank[bank_offset + j].cycles.act =
                rank.banks[j].cycles.act.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.selfRefresh =
                rank.cycles.sref.get_count_at(timestamp) -
                rank.cycles.deepSleepMode.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.deepSleepMode =
                rank.cycles.deepSleepMode.get_count_at(timestamp);
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

        stats.rank_total[i].cycles.pre =
            simulation_duration - (rank.cycles.act.get_count_at(timestamp) +
                                    rank.cycles.powerDownAct.get_count_at(timestamp) +
                                    rank.cycles.powerDownPre.get_count_at(timestamp) +
                                    rank.cycles.sref.get_count_at(timestamp));

        stats.rank_total[i].cycles.act = rank.cycles.act.get_count_at(timestamp);
        stats.rank_total[i].cycles.powerDownAct =
            rank.cycles.powerDownAct.get_count_at(timestamp);
        stats.rank_total[i].cycles.powerDownPre =
            rank.cycles.powerDownPre.get_count_at(timestamp);
        stats.rank_total[i].cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp);
    }
}

void LPDDR4Core::serialize(std::ostream& stream) const {
    // Serialize the ranks
    const std::size_t numBanks = m_memSpec.numberOfBanks;
    const std::size_t rankCount = m_ranks.size();
    stream.write(reinterpret_cast<const char*>(&numBanks), sizeof(numBanks));
    stream.write(reinterpret_cast<const char*>(&rankCount), sizeof(rankCount));
    for (const auto& rank : m_ranks) {
        rank.serialize(stream);
    }
}

void LPDDR4Core::deserialize(std::istream& stream) {
    // Deserialize the ranks
    std::size_t numBanks = 0;
    std::size_t rankCount = 0;
    stream.read(reinterpret_cast<char*>(&numBanks), sizeof(numBanks));
    stream.read(reinterpret_cast<char*>(&rankCount), sizeof(rankCount));
    m_ranks.clear();
    m_ranks.reserve(rankCount);
    for (std::size_t i = 0; i < rankCount; i++) {
        Rank rank{numBanks};
        rank.deserialize(stream);
        m_ranks.push_back(std::move(rank));
    }
}

} // namespace DRAMPower