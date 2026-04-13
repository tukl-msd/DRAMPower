#include "LPDDR4Core.h"
#include "DRAMPower/util/RegisterHelper.h"

namespace DRAMPower {

void LPDDR4Core::doCommand(const Command& cmd) {
    m_implicitCommandHandler.processImplicitCommandQueue(*this, cmd.timestamp, m_last_command_time);
    m_last_command_time = std::max(cmd.timestamp, m_last_command_time);
    switch(cmd.type) {
        case CmdType::ACT:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &LPDDR4Core::handleAct);
            break;
        case CmdType::PRE:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &LPDDR4Core::handlePre);
            break;
        case CmdType::PREA:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR4Core::handlePreAll);
            break;
        case CmdType::REFB:
            util::coreHelpers::bankHandlerIdx(cmd, m_ranks, this, &LPDDR4Core::handleRefPerBank);
            break;
        case CmdType::RD:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &LPDDR4Core::handleRead);
            break;
        case CmdType::RDA:
            util::coreHelpers::bankHandlerIdx(cmd, m_ranks, this, &LPDDR4Core::handleReadAuto);
            break;
        case CmdType::WR:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &LPDDR4Core::handleWrite);
            break;
        case CmdType::WRA:
            util::coreHelpers::bankHandlerIdx(cmd, m_ranks, this, &LPDDR4Core::handleWriteAuto);
            break;
        case CmdType::REFA:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &LPDDR4Core::handleRefAll);
            break;
        case CmdType::PDEA:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &LPDDR4Core::handlePowerDownActEntry);
            break;
        case CmdType::PDXA:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &LPDDR4Core::handlePowerDownActExit);
            break;
        case CmdType::PDEP:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &LPDDR4Core::handlePowerDownPreEntry);
            break;
        case CmdType::PDXP:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &LPDDR4Core::handlePowerDownPreExit);
            break;
        case CmdType::SREFEN:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &LPDDR4Core::handleSelfRefreshEntry);
            break;
        case CmdType::SREFEX:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR4Core::handleSelfRefreshExit);
            break;
        case CmdType::END_OF_SIMULATION:
            break;
        default:
            assert(false && "Unsupported command");
            break;
    }
}

timestamp_t LPDDR4Core::getLastCommandTime() const {
    return m_last_command_time;
}

bool LPDDR4Core::isSerializable() const {
    return 0 == m_implicitCommandHandler.implicitCommandCount();
}

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

void LPDDR4Core::handleRefreshOnBank(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp, uint64_t timing, uint64_t &counter) {
    ++counter;
    auto& rank = m_ranks[rank_idx];
    auto& bank = rank.banks[bank_idx];

    if (!rank.isActive(timestamp)) {
        rank.cycles.act.start_interval(timestamp);
    }

    bank.bankState = Bank::BankState::BANK_ACTIVE;
    auto timestamp_end = timestamp + timing;
    bank.refreshEndTime = timestamp_end;
    if (!bank.cycles.act.is_open())
        bank.cycles.act.start_interval(timestamp);

    // Execute implicit pre-charge at refresh end
    m_implicitCommandHandler.addImplicitCommand(timestamp_end, [rank_idx, bank_idx, timestamp_end](LPDDR4Core& self) {
        auto& rank = self.m_ranks[rank_idx];
        auto& bank = rank.banks[bank_idx];
        bank.bankState = Bank::BankState::BANK_PRECHARGED;
        bank.cycles.act.close_interval(timestamp_end);

        if (!rank.isActive(timestamp_end)) {
            rank.cycles.act.close_interval(timestamp_end);
        }
    });
}

void LPDDR4Core::handleRefAll(std::size_t rank_idx, timestamp_t timestamp) {
    auto& rank = m_ranks[rank_idx];
    for (std::size_t bank_idx = 0; bank_idx < rank.banks.size(); ++bank_idx) {
        auto& counter = rank.banks[bank_idx].counter.refAllBank;
        handleRefreshOnBank(rank_idx, bank_idx, timestamp, m_memSpec.memTimingSpec.tRFC, counter);
    }
    rank.endRefreshTime = timestamp + m_memSpec.memTimingSpec.tRFC;
}

void LPDDR4Core::handleRefPerBank(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp) {
    auto& counter = m_ranks[rank_idx].banks[bank_idx].counter.refPerBank;
    handleRefreshOnBank(rank_idx, bank_idx, timestamp, m_memSpec.memTimingSpec.tRFCPB, counter);
}

void LPDDR4Core::handleSelfRefreshEntry(std::size_t rank_idx, timestamp_t timestamp) {
    // Issue implicit refresh
    handleRefAll(rank_idx, timestamp);
    // Handle self-refresh entry after tRFC
    auto timestampSelfRefreshStart = timestamp + m_memSpec.memTimingSpec.tRFC;
    m_implicitCommandHandler.addImplicitCommand(timestampSelfRefreshStart, [rank_idx, timestampSelfRefreshStart](LPDDR4Core& self) {
        auto& rank = self.m_ranks[rank_idx];
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

void LPDDR4Core::handlePowerDownActEntry(std::size_t rank_idx, timestamp_t timestamp) {
    auto& rank = m_ranks[rank_idx];
    auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
    auto entryTime = std::max(timestamp, earliestPossibleEntry);
    m_implicitCommandHandler.addImplicitCommand(entryTime, [rank_idx, entryTime](LPDDR4Core& self) {
        auto& rank = self.m_ranks[rank_idx];
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

void LPDDR4Core::handlePowerDownActExit(std::size_t rank_idx, timestamp_t timestamp) {
    auto& rank = m_ranks[rank_idx];
    auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
    auto exitTime = std::max(timestamp, earliestPossibleExit);

    m_implicitCommandHandler.addImplicitCommand(exitTime, [rank_idx, exitTime](LPDDR4Core& self) {
        auto& rank = self.m_ranks[rank_idx];
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

void LPDDR4Core::handlePowerDownPreEntry(std::size_t rank_idx, timestamp_t timestamp) {
    auto& rank = m_ranks[rank_idx];
    auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
    auto entryTime = std::max(timestamp, earliestPossibleEntry);
    m_implicitCommandHandler.addImplicitCommand(entryTime, [rank_idx, entryTime](LPDDR4Core& self) {
        auto& rank = self.m_ranks[rank_idx];
        rank.cycles.powerDownPre.start_interval(entryTime);
        rank.memState = MemState::PDN_PRE;
    });
}

void LPDDR4Core::handlePowerDownPreExit(std::size_t rank_idx, timestamp_t timestamp) {
    auto& rank = m_ranks[rank_idx];
    auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
    auto exitTime = std::max(timestamp, earliestPossibleExit);

    m_implicitCommandHandler.addImplicitCommand(exitTime, [rank_idx, exitTime](LPDDR4Core& self) {
        auto& rank = self.m_ranks[rank_idx];
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

void LPDDR4Core::handleReadAuto(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp) {
    auto& bank = m_ranks[rank_idx].banks[bank_idx];
    ++bank.counter.readAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    auto minReadActiveTime = timestamp + m_memSpec.prechargeOffsetRD;

    auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandHandler.addImplicitCommand(delayed_timestamp, [rank_idx, bank_idx, delayed_timestamp](LPDDR4Core& self) {
        auto& rank = self.m_ranks[rank_idx];
        auto& bank = rank.banks[bank_idx];
        self.handlePre(rank, bank, delayed_timestamp);
    });
}

void LPDDR4Core::handleWriteAuto(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp) {
    auto& bank = m_ranks[rank_idx].banks[bank_idx];
    ++bank.counter.writeAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    auto minWriteActiveTime = timestamp + m_memSpec.prechargeOffsetWR;

    auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandHandler.addImplicitCommand(delayed_timestamp, [rank_idx, bank_idx, delayed_timestamp](LPDDR4Core& self) {
        auto& rank = self.m_ranks[rank_idx];
        auto& bank = rank.banks[bank_idx];
        self.handlePre(rank, bank, delayed_timestamp);
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

void LPDDR4Core::getWindowStats(timestamp_t timestamp, SimulationStats &stats) {
    m_implicitCommandHandler.processImplicitCommandQueue(*this, timestamp, m_last_command_time);
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
    stream.write(reinterpret_cast<const char*>(&m_last_command_time), sizeof(m_last_command_time));
    for (const auto& rank : m_ranks) {
        rank.serialize(stream);
    }
}

void LPDDR4Core::deserialize(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&m_last_command_time), sizeof(m_last_command_time));
    for (auto& rank : m_ranks) {
        rank.deserialize(stream);
    }
}

} // namespace DRAMPower