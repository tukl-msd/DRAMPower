#include "LPDDR5Core.h"
#include "DRAMPower/Exceptions.h"
#include "DRAMPower/util/RegisterHelper.h"

namespace DRAMPower {

void LPDDR5Core::doCommand(const Command& cmd) {
    m_implicitCommandHandler.processImplicitCommandQueue(cmd.timestamp, m_last_command_time);
    m_last_command_time = std::max(cmd.timestamp, m_last_command_time);
    switch(cmd.type) {
        case CmdType::ACT:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &LPDDR5Core::handleAct);
            break;
        case CmdType::PRE:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &LPDDR5Core::handlePre);
            break;
        case CmdType::PREA:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR5Core::handlePreAll);
            break;
        case CmdType::REFB:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &LPDDR5Core::handleRefPerBank);
            break;
        case CmdType::RD:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &LPDDR5Core::handleRead);
            break;
        case CmdType::RDA:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &LPDDR5Core::handleReadAuto);
            break;
        case CmdType::WR:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &LPDDR5Core::handleWrite);
            break;
        case CmdType::WRA:
            util::coreHelpers::bankHandler(cmd, m_ranks, this, &LPDDR5Core::handleWriteAuto);
            break;
        case CmdType::REFP2B:
            if (m_memSpec.bank_arch != MemSpecLPDDR5::MBG && m_memSpec.bank_arch != MemSpecLPDDR5::M16B) {
                throw Exception(std::string("REFP2B command is not supported for this bank architecture: ") + CmdTypeUtil::to_string(CmdType::REFP2B));
            }
            util::coreHelpers::bankGroupHandler(cmd, m_ranks, this, &LPDDR5Core::handleRefPerTwoBanks);
            break;
        case CmdType::REFA:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR5Core::handleRefAll);
            break;
        case CmdType::SREFEN:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR5Core::handleSelfRefreshEntry);
            break;
        case CmdType::SREFEX:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR5Core::handleSelfRefreshExit);
            break;
        case CmdType::PDEA:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR5Core::handlePowerDownActEntry);
            break;
        case CmdType::PDEP:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR5Core::handlePowerDownPreEntry);
            break;
        case CmdType::PDXA:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR5Core::handlePowerDownActExit);
            break;
        case CmdType::PDXP:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR5Core::handlePowerDownPreExit);
            break;
        case CmdType::DSMEN:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR5Core::handleDSMEntry);
            break;
        case CmdType::DSMEX:
            util::coreHelpers::rankHandler(cmd, m_ranks, this, &LPDDR5Core::handleDSMExit);
            break;
        case CmdType::END_OF_SIMULATION:
            break;
        default:
            assert(false && "Unsupported command");
            break;
    }
}

timestamp_t LPDDR5Core::getLastCommandTime() const {
    return m_last_command_time;
}

bool LPDDR5Core::isSerializable() const {
    return 0 == m_implicitCommandHandler.implicitCommandCount();
}

void LPDDR5Core::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
    bank.counter.act++;

    bank.cycles.act.start_interval(timestamp);

    if ( !rank.isActive(timestamp) ) {
        rank.cycles.act.start_interval(timestamp);
    }

    bank.bankState = Bank::BankState::BANK_ACTIVE;
}

void LPDDR5Core::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
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

void LPDDR5Core::handlePreAll(Rank &rank, timestamp_t timestamp) {
    for (auto &bank: rank.banks) {
        handlePre(rank, bank, timestamp);
    }
}

void LPDDR5Core::handleRefPerBank(Rank & rank, Bank & bank, timestamp_t timestamp) {
    handleRefreshOnBank(rank, bank, timestamp, m_memSpec.memTimingSpec.tRFCPB, bank.counter.refPerBank);
}

void LPDDR5Core::handleRefPerTwoBanks(Rank & rank, std::size_t bank_id, timestamp_t timestamp) {
    Bank & bank_1 = rank.banks[bank_id];
    Bank & bank_2 = rank.banks[(bank_id + m_memSpec.perTwoBankOffset)%16];
    handleRefreshOnBank(rank, bank_1, timestamp, m_memSpec.memTimingSpec.tRFCPB, bank_1.counter.refPerTwoBanks);
    handleRefreshOnBank(rank, bank_2, timestamp, m_memSpec.memTimingSpec.tRFCPB, bank_2.counter.refPerTwoBanks);
}

void LPDDR5Core::handleRefAll(Rank &rank, timestamp_t timestamp) {
    for (auto& bank : rank.banks) {
        handleRefreshOnBank(rank, bank, timestamp, m_memSpec.memTimingSpec.tRFC, bank.counter.refAllBank);
    }
    rank.endRefreshTime = timestamp + m_memSpec.memTimingSpec.tRFC;
}

void LPDDR5Core::handleRefreshOnBank(Rank & rank, Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t & counter){
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
    m_implicitCommandHandler.addImplicitCommand(timestamp_end, [&bank, &rank, timestamp_end]() {
        bank.bankState = Bank::BankState::BANK_PRECHARGED;
        bank.cycles.act.close_interval(timestamp_end);

        if (!rank.isActive(timestamp_end)) {
            rank.cycles.act.close_interval(timestamp_end);
        }
    });
}

void LPDDR5Core::handleRead(Rank&, Bank &bank, timestamp_t) {
    ++bank.counter.reads;
}

void LPDDR5Core::handleReadAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
    ++bank.counter.readAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + this->m_memSpec.memTimingSpec.tRAS;
    auto minReadActiveTime = timestamp + this->m_memSpec.prechargeOffsetRD;

    auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandHandler.addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
        this->handlePre(rank, bank, delayed_timestamp);
    });
}

void LPDDR5Core::handleWrite(Rank&, Bank &bank, timestamp_t) {
    ++bank.counter.writes;
}

void LPDDR5Core::handleWriteAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
    ++bank.counter.writeAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + this->m_memSpec.memTimingSpec.tRAS;
    auto minWriteActiveTime = timestamp + this->m_memSpec.prechargeOffsetWR;

    auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);
    // Execute PRE after minimum active time
    m_implicitCommandHandler.addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
        this->handlePre(rank, bank, delayed_timestamp);
    });
}

void LPDDR5Core::handleSelfRefreshEntry(Rank &rank, timestamp_t timestamp) {
    // Issue implicit refresh
    handleRefAll(rank, timestamp);
    // Handle self-refresh entry after tRFC
    auto timestampSelfRefreshStart = timestamp + m_memSpec.memTimingSpec.tRFC;
    m_implicitCommandHandler.addImplicitCommand(timestampSelfRefreshStart, [&rank, timestampSelfRefreshStart]() {
        rank.counter.selfRefresh++;
        rank.cycles.sref.start_interval(timestampSelfRefreshStart);
        rank.memState = MemState::SREF;
    });
}

void LPDDR5Core::handleSelfRefreshExit(Rank &rank, timestamp_t timestamp) {
    assert(rank.memState == MemState::SREF);
    rank.cycles.sref.close_interval(timestamp);
    rank.memState = MemState::NOT_IN_PD;
}

void LPDDR5Core::handlePowerDownActEntry(Rank &rank, timestamp_t timestamp) {
    auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
    auto entryTime = std::max(timestamp, earliestPossibleEntry);
    m_implicitCommandHandler.addImplicitCommand(entryTime, [&rank, entryTime]() {
        rank.cycles.powerDownAct.start_interval(entryTime);
        rank.memState = MemState::PDN_ACT;
        if (rank.cycles.act.is_open()) {
            rank.cycles.act.close_interval(entryTime);
        }
        for (auto & bank : rank.banks) {
            if (bank.cycles.act.is_open()) {
                bank.cycles.act.close_interval(entryTime);
            }
        }
    });
}

void LPDDR5Core::handlePowerDownActExit(Rank &rank, timestamp_t timestamp) {
    auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
    auto exitTime = std::max(timestamp, earliestPossibleExit);

    m_implicitCommandHandler.addImplicitCommand(exitTime, [&rank, exitTime]() {
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

void LPDDR5Core::handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp) {
    auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
    auto entryTime = std::max(timestamp, earliestPossibleEntry);
    m_implicitCommandHandler.addImplicitCommand(entryTime, [&rank, entryTime]() {
        rank.cycles.powerDownPre.start_interval(entryTime);
        rank.memState = MemState::PDN_PRE;
    });
}

void LPDDR5Core::handlePowerDownPreExit(Rank &rank, timestamp_t timestamp) {
    auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
    auto exitTime = std::max(timestamp, earliestPossibleExit);

    m_implicitCommandHandler.addImplicitCommand(exitTime, [&rank, exitTime]() {
        rank.memState = MemState::NOT_IN_PD;
        rank.cycles.powerDownPre.close_interval(exitTime);
    });
}

void LPDDR5Core::handleDSMEntry(Rank &rank, timestamp_t timestamp) {
    assert(rank.memState == MemState::SREF);
    rank.cycles.deepSleepMode.start_interval(timestamp);
    rank.counter.deepSleepMode++;
    rank.memState = MemState::DSM;
}

void LPDDR5Core::handleDSMExit(Rank &rank, timestamp_t timestamp) {
    assert(rank.memState == MemState::DSM);
    rank.cycles.deepSleepMode.close_interval(timestamp);
    rank.memState = MemState::SREF;
}

timestamp_t LPDDR5Core::earliestPossiblePowerDownEntryTime(Rank & rank) const {
    timestamp_t entryTime = 0;

    for (const auto &bank : rank.banks) {
        entryTime = std::max(
            {entryTime,
                bank.counter.act == 0 ? 0
                                    : bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRCD,
                bank.counter.pre == 0 ? 0 : bank.latestPre + m_memSpec.memTimingSpec.tRP,
                bank.refreshEndTime});
    }

    return entryTime;
}

void LPDDR5Core::getWindowStats(timestamp_t timestamp, SimulationStats &stats) {
    m_implicitCommandHandler.processImplicitCommandQueue(timestamp, m_last_command_time);
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
        stats.rank_total[i].cycles.selfRefresh =
            rank.cycles.sref.get_count_at(timestamp) -
            rank.cycles.deepSleepMode.get_count_at(timestamp);
        stats.rank_total[i].cycles.deepSleepMode =
            rank.cycles.deepSleepMode.get_count_at(timestamp);
    }
}

void LPDDR5Core::serialize(std::ostream& stream) const {
    stream.write(reinterpret_cast<const char*>(&m_last_command_time), sizeof(m_last_command_time));
    for (const auto& rank : m_ranks) {
        rank.serialize(stream);
    }
}
void LPDDR5Core::deserialize(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&m_last_command_time), sizeof(m_last_command_time));
    for (auto& rank : m_ranks) {
        rank.deserialize(stream);
    }
}

} // namespace DRAMPower