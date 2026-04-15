#include "DDR5Core.h"
#include "DRAMPower/util/RegisterHelper.h"

namespace DRAMPower {

void DDR5Core::doCommand(const Command& cmd) {
    m_implicitCommandHandler.processImplicitCommandQueue(*this, cmd.timestamp, m_last_command_time);
    m_last_command_time = std::max(cmd.timestamp, m_last_command_time);
    switch(cmd.type) {
        case CmdType::ACT:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &DDR5Core::handleAct);
            break;
        case CmdType::PRE:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &DDR5Core::handlePre);
            break;
        case CmdType::RD:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &DDR5Core::handleRead);
            break;
        case CmdType::RDA:
            util::coreHelpers::bankHandlerIdx(cmd, m_ranks, this, &DDR5Core::handleReadAuto);
            break;
        case CmdType::WR:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &DDR5Core::handleWrite);
            break;
        case CmdType::WRA:
            util::coreHelpers::bankHandlerIdx(cmd, m_ranks, this, &DDR5Core::handleWriteAuto);
            break;
        case CmdType::PRESB:
            util::coreHelpers::bankGroupHandler(cmd, m_ranks, this, &DDR5Core::handlePreSameBank);
            break;
        case CmdType::REFSB:
            util::coreHelpers::bankGroupHandlerIdx(cmd, m_ranks, this, &DDR5Core::handleRefSameBank);
            break;
        case CmdType::REFA:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &DDR5Core::handleRefAll);
            break;
        case CmdType::PREA:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &DDR5Core::handlePreAll);
            break;
        case CmdType::SREFEN:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &DDR5Core::handleSelfRefreshEntry);
            break;
        case CmdType::SREFEX:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &DDR5Core::handleSelfRefreshExit);
            break;
        case CmdType::PDEA:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &DDR5Core::handlePowerDownActEntry);
            break;
        case CmdType::PDEP:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &DDR5Core::handlePowerDownPreEntry);
            break;
        case CmdType::PDXA:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &DDR5Core::handlePowerDownActExit);
            break;
        case CmdType::PDXP:
            util::coreHelpers::rankHandlerIdx(cmd, m_ranks, this, &DDR5Core::handlePowerDownPreExit);
            break;
        case CmdType::NOP:
        case CmdType::END_OF_SIMULATION:
            break;
        default:
            assert(false && "Unsupported command");
            break;
    }
}

timestamp_t DDR5Core::getLastCommandTime() const {
    return m_last_command_time;
}

bool DDR5Core::isSerializable() const {
    return 0 == m_implicitCommandHandler.implicitCommandCount();
}

void DDR5Core::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
    bank.counter.act++;

    bank.cycles.act.start_interval(timestamp);

    if ( !rank.isActive(timestamp) ) {
        rank.cycles.act.start_interval(timestamp);
    };

    bank.bankState = Bank::BankState::BANK_ACTIVE;
}

void DDR5Core::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
    if (bank.bankState == Bank::BankState::BANK_PRECHARGED)
        return;

    bank.counter.pre++;
    bank.cycles.act.close_interval(timestamp);
    bank.latestPre = timestamp;
    bank.bankState = Bank::BankState::BANK_PRECHARGED;

    if ( !rank.isActive(timestamp) ) {
        rank.cycles.act.close_interval(timestamp);
    }
}

void DDR5Core::handlePreSameBank(Rank & rank, std::size_t bank_id, timestamp_t timestamp) {
    auto bank_id_inside_bg = bank_id % m_memSpec.banksPerGroup;
    for(unsigned bank_group = 0; bank_group < m_memSpec.numberOfBankGroups; bank_group++) {
        auto & bank = rank.banks[bank_group * m_memSpec.banksPerGroup + bank_id_inside_bg];
        handlePre(rank, bank, timestamp);
    }
}

void DDR5Core::handlePreAll(Rank &rank, timestamp_t timestamp) {
    for (auto &bank: rank.banks) {
        handlePre(rank, bank, timestamp);
    }
}

void DDR5Core::handleRefSameBank(std::size_t rank_idx, std::size_t bank_id, timestamp_t timestamp) {
    auto bank_id_inside_bg = bank_id % m_memSpec.banksPerGroup;
    for(unsigned bank_group = 0; bank_group < m_memSpec.numberOfBankGroups; bank_group++) {
        std::size_t bank_idx = bank_group * m_memSpec.banksPerGroup + bank_id_inside_bg;
        auto& counter = m_ranks[rank_idx].banks[bank_idx].counter.refSameBank;
        handleRefreshOnBank(rank_idx, bank_idx, timestamp, m_memSpec.tRFCsb, counter);
    }
}

void DDR5Core::handleRefAll(std::size_t rank_idx, timestamp_t timestamp) {
    auto& rank = m_ranks[rank_idx];
    for (std::size_t bank_idx = 0; bank_idx < rank.banks.size(); ++bank_idx) {
        auto& counter = m_ranks[rank_idx].banks[bank_idx].counter.refAllBank;
        handleRefreshOnBank(rank_idx, bank_idx, timestamp, m_memSpec.tRFC, counter);
    }

    rank.endRefreshTime = timestamp + m_memSpec.tRFC;
}

void DDR5Core::handleRefreshOnBank(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp, uint64_t timing, uint64_t & counter){
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
    m_implicitCommandHandler.addImplicitCommand(timestamp_end, [rank_idx, bank_idx, timestamp_end](DDR5Core& self) {
        auto& rank = self.m_ranks[rank_idx];
        auto& bank = rank.banks[bank_idx];
        bank.bankState = Bank::BankState::BANK_PRECHARGED;
        bank.cycles.act.close_interval(timestamp_end);

        if (!rank.isActive(timestamp_end)) {
            rank.cycles.act.close_interval(timestamp_end);
        }
    });
}

void DDR5Core::handleRead(Rank&, Bank &bank, timestamp_t){
    ++bank.counter.reads;
}

void DDR5Core::handleReadAuto(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp) {
    auto& bank = m_ranks[rank_idx].banks[bank_idx];
    ++bank.counter.readAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.tRAS;
    auto minReadActiveTime = timestamp + m_memSpec.prechargeOffsetRD;

    auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandHandler.addImplicitCommand(delayed_timestamp, [rank_idx, bank_idx, delayed_timestamp](DDR5Core& self) {
        auto& rank = self.m_ranks[rank_idx];
        auto& bank = rank.banks[bank_idx];
        self.handlePre(rank, bank, delayed_timestamp);
    });
}

void DDR5Core::handleWrite(Rank&, Bank &bank, timestamp_t) {
    ++bank.counter.writes;
}

void DDR5Core::handleWriteAuto(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp) {
    auto& bank = m_ranks[rank_idx].banks[bank_idx];
    ++bank.counter.writeAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.tRAS;
    auto minWriteActiveTime = timestamp + m_memSpec.prechargeOffsetWR;

    auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandHandler.addImplicitCommand(delayed_timestamp, [rank_idx, bank_idx, delayed_timestamp](DDR5Core& self) {
        auto& rank = self.m_ranks[rank_idx];
        auto& bank = rank.banks[bank_idx];
        self.handlePre(rank, bank, delayed_timestamp);
    });
}

void DDR5Core::handleSelfRefreshEntry(std::size_t rank_idx, timestamp_t timestamp) {
    // Issue implicit refresh
    handleRefAll(rank_idx, timestamp);

    // Handle self-refresh entry after tRFC
    auto timestampSelfRefreshStart = timestamp + m_memSpec.tRFC;

    m_implicitCommandHandler.addImplicitCommand(timestampSelfRefreshStart, [rank_idx, timestampSelfRefreshStart](DDR5Core& self) {
        auto& rank = self.m_ranks[rank_idx];
        rank.counter.selfRefresh++;
        rank.cycles.sref.start_interval(timestampSelfRefreshStart);
        rank.memState = MemState::SREF;
    });
}

void DDR5Core::handleSelfRefreshExit(Rank &rank, timestamp_t timestamp) {
    assert(rank.memState == MemState::SREF);
    rank.cycles.sref.close_interval(timestamp);  // Duration between entry and exit
    rank.memState = MemState::NOT_IN_PD;
}

void DDR5Core::handlePowerDownActEntry(std::size_t rank_idx, timestamp_t timestamp) {
    auto& rank = m_ranks[rank_idx];
    auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
    auto entryTime = std::max(timestamp, earliestPossibleEntry);
    m_implicitCommandHandler.addImplicitCommand(entryTime, [rank_idx, entryTime](DDR5Core& self) {
        auto& rank = self.m_ranks[rank_idx];
        rank.cycles.powerDownAct.start_interval(entryTime);
        rank.memState = MemState::PDN_ACT;
        if (rank.cycles.act.is_open()) {
            rank.cycles.act.close_interval(entryTime);
        }
        for (auto & bank : rank.banks) {
            if (bank.cycles.act.is_open()) {
                bank.cycles.act.close_interval(entryTime);
            }
        };
    });
}

void DDR5Core::handlePowerDownActExit(std::size_t rank_idx, timestamp_t timestamp) {
    auto& rank = m_ranks[rank_idx];
    auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
    auto exitTime = std::max(timestamp, earliestPossibleExit);

    m_implicitCommandHandler.addImplicitCommand(exitTime, [rank_idx, exitTime](DDR5Core& self) {
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
}

void DDR5Core::handlePowerDownPreEntry(std::size_t rank_idx, timestamp_t timestamp) {
    auto& rank = m_ranks[rank_idx];
    auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
    auto entryTime = std::max(timestamp, earliestPossibleEntry);

    m_implicitCommandHandler.addImplicitCommand(entryTime, [rank_idx, entryTime](DDR5Core &self) {
        auto& rank = self.m_ranks[rank_idx];
        rank.cycles.powerDownPre.start_interval(entryTime);
        rank.memState = MemState::PDN_PRE;
    });
}

void DDR5Core::handlePowerDownPreExit(std::size_t rank_idx, timestamp_t timestamp) {
    auto& rank = m_ranks[rank_idx];
    auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
    auto exitTime = std::max(timestamp, earliestPossibleExit);

    m_implicitCommandHandler.addImplicitCommand(exitTime, [rank_idx, exitTime](DDR5Core& self) {
        auto& rank = self.m_ranks[rank_idx];
        rank.memState = MemState::NOT_IN_PD;
        rank.cycles.powerDownPre.close_interval(exitTime);
    });
}

timestamp_t DDR5Core::earliestPossiblePowerDownEntryTime(Rank &rank) {
    timestamp_t entryTime = 0;

    for (const auto &bank : rank.banks) {
        entryTime = std::max(
            {entryTime,
                bank.counter.act == 0 ? 0
                                    : bank.cycles.act.get_start() + m_memSpec.tRCD,
                bank.counter.pre == 0 ? 0 : bank.latestPre + m_memSpec.tRP,
                bank.refreshEndTime});
    }

    return entryTime;
}

void DDR5Core::getWindowStats(timestamp_t timestamp, SimulationStats &stats) {
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
        stats.rank_total[i].cycles.deepSleepMode =
            rank.cycles.deepSleepMode.get_count_at(timestamp);
        stats.rank_total[i].cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp);
    }
}

void DDR5Core::serialize(std::ostream& stream) const {
    stream.write(reinterpret_cast<const char*>(&m_last_command_time), sizeof(m_last_command_time));
    for (const auto& rank : m_ranks) {
        rank.serialize(stream);
    }
}

void DDR5Core::deserialize(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&m_last_command_time), sizeof(m_last_command_time));
    for (auto& rank : m_ranks) {
        rank.deserialize(stream);
    }
}

} // namespace DRAMPower
