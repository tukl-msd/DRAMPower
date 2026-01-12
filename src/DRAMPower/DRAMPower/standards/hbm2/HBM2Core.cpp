#include "HBM2Core.h"
#include "DRAMPower/dram/Rank.h"

namespace DRAMPower {

HBM2Core::HBM2Core(const MemSpecHBM2& memSpec, implicitCommandInserter_t&& implicitCommandInserter)
    : m_banks(memSpec.numberOfStacks * memSpec.numberOfBanks) 
    , m_memSpec(memSpec)
    , m_implicitCommandInserter(std::move(implicitCommandInserter))
{}

void HBM2Core::handleAct(Bank &bank, timestamp_t timestamp) {
    if (bank.bankState == Bank::BankState::BANK_ACTIVE) return;
    bank.bankState = Bank::BankState::BANK_ACTIVE;
    bank.counter.act++;
    bank.cycles.act.start_interval(timestamp);
    m_cycles.act.start_interval_if_not_running(timestamp);
}

void HBM2Core::handlePre_impl(Bank &bank, timestamp_t timestamp, uint64_t& counter) {
    if (bank.bankState == Bank::BankState::BANK_PRECHARGED) return;
    bank.bankState = Bank::BankState::BANK_PRECHARGED;
    counter++;
    bank.cycles.act.close_interval(timestamp);
    if ( !Rank::isActive_impl(timestamp, m_endRefreshTime, m_banks) ) {
        m_cycles.act.close_interval(timestamp);
    }
}

void HBM2Core::handlePre(Bank &bank, timestamp_t timestamp) {
    handlePre_impl(bank, timestamp, bank.counter.pre);
}

void HBM2Core::handlePreAll(timestamp_t timestamp) {
    for (auto &bank: m_banks) {
        handlePre(bank, timestamp);
    }
}

void HBM2Core::handleRefAll(timestamp_t timestamp) {
    auto timing = m_memSpec.memTimingSpec.tRFC;
    m_endRefreshTime = timestamp + timing;
    m_cycles.act.start_interval_if_not_running(timestamp);
    for (auto& bank : m_banks) {
        handleRefreshOnBank(bank, timestamp, timing, bank.counter.refAllBank); // TODO
    }
}

void HBM2Core::handleRefSingleBank(Bank & bank, timestamp_t timestamp) {
    handleRefreshOnBank(bank, timestamp, m_memSpec.memTimingSpec.tRFC, bank.counter.refPerBank); // TODO
}

void HBM2Core::handleRefreshOnBank(Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t& counter) {
    assert(bank.bankState == Bank::BankState::BANK_ACTIVE && "Bank must be in active state for a refresh command");
    // Refresh counter incremented at timestamp_end
    bank.cycles.act.start_interval_if_not_running(timestamp);

    // Execute implicit pre-charge at refresh end
    const auto timestamp_end = timestamp + timing;
    m_implicitCommandInserter.addImplicitCommand(timestamp_end, [this, &bank, timestamp_end, &counter]() {
        handlePre_impl(bank, timestamp_end, counter);
    });
}

void HBM2Core::handleRead(Bank &bank, timestamp_t) {
    ++bank.counter.reads;
}

void HBM2Core::handleReadAuto(Bank &bank, timestamp_t timestamp) {
    ++bank.counter.readAuto;

    const auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    const auto minReadActiveTime = timestamp + m_memSpec.prechargeOffsetRD;

    const auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandInserter.addImplicitCommand(delayed_timestamp, [this, &bank, delayed_timestamp]() {
        this->handlePre(bank, delayed_timestamp);
    });
}

void HBM2Core::handleWrite(Bank &bank, timestamp_t) {
    ++bank.counter.writes;
}

void HBM2Core::handleWriteAuto(Bank &bank, timestamp_t timestamp) {
    ++bank.counter.writeAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    auto minWriteActiveTime =  timestamp + m_memSpec.prechargeOffsetWR;

    auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandInserter.addImplicitCommand(delayed_timestamp, [this, &bank, delayed_timestamp]() {
        this->handlePre(bank, delayed_timestamp);
    });
}

void HBM2Core::handleSelfRefreshEntry(timestamp_t timestamp) {
    // Issue implicit refresh
    handleRefAll(timestamp);
    // Handle self-refresh entry after tRFC // TODO verify
    auto timestampSelfRefreshStart = timestamp + m_memSpec.memTimingSpec.tRFC;
    m_implicitCommandInserter.addImplicitCommand(timestampSelfRefreshStart, [this, timestampSelfRefreshStart]() {
        m_counter.selfRefresh++;
        m_cycles.sref.start_interval(timestampSelfRefreshStart);
        m_memState = MemState::SREF;
    });
}

void HBM2Core::handleSelfRefreshExit(timestamp_t timestamp) {
    assert(m_memState == MemState::SREF && "SelfRefreshExit is only valid if it is preceded by a SelfRefreshEntry command");
    m_cycles.sref.close_interval(timestamp);
    m_memState = MemState::NOT_IN_PD;
}

void HBM2Core::handlePowerDownActEntry(timestamp_t timestamp) {
    timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());
    m_implicitCommandInserter.addImplicitCommand(timestamp, [this, timestamp]() {
        m_memState = MemState::PDN_ACT;
        m_cycles.powerDownAct.start_interval(timestamp);
        m_cycles.act.close_interval(timestamp);
        for (auto & bank : m_banks) {
            bank.cycles.act.close_interval(timestamp);
        }
    });
}

void HBM2Core::handlePowerDownActExit(timestamp_t timestamp) {
    timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());
    assert(m_memState == MemState::PDN_ACT && "PowerDownActExit is only valid if it is preceded by a PowerDownActEntry command");
    
    m_implicitCommandInserter.addImplicitCommand(timestamp, [this, timestamp]() {
        m_memState = MemState::NOT_IN_PD;
        m_cycles.powerDownAct.close_interval(timestamp);

        // Activate banks that were active prior to PDA
        for (auto & bank : m_banks) {
            if (Bank::BankState::BANK_ACTIVE == bank.bankState) {
                bank.cycles.act.start_interval(timestamp);
            }
        }

        // Activate if at least one bank is active
        if (Rank::isActive_impl(timestamp, m_endRefreshTime, m_banks)) {
            m_cycles.act.start_interval(timestamp);
        }
    });
}

void HBM2Core::handlePowerDownPreEntry(timestamp_t timestamp) {
    timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());
    
    m_implicitCommandInserter.addImplicitCommand(timestamp, [this, timestamp]() {
        for (auto &bank : m_banks) {
            bank.cycles.act.close_interval(timestamp);
        }
        m_memState = MemState::PDN_PRE;
        m_cycles.powerDownPre.start_interval(timestamp);
        m_cycles.act.close_interval(timestamp);
    });
}

void HBM2Core::handlePowerDownPreExit(timestamp_t timestamp) {
    assert(m_memState == MemState::PDN_PRE && "PowerDownPreExit is only valid if it is preceded by a PowerDownPreEntry command");
    timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());
    
    m_implicitCommandInserter.addImplicitCommand(timestamp, [this, timestamp]() {
        m_memState = MemState::NOT_IN_PD;
        m_cycles.powerDownPre.close_interval(timestamp);

        // Precharge banks that were precharged prior to PDP
        for (auto & bank : m_banks)
        {
            if (Bank::BankState::BANK_ACTIVE == bank.bankState) {
                bank.cycles.act.start_interval(timestamp);
            }
        }
        if(Rank::isActive_impl(timestamp, m_endRefreshTime, m_banks)) { // TODO: this is not used in any other standard -> verify for correctness
           m_cycles.act.start_interval(timestamp);
        }
    });
}

timestamp_t HBM2Core::earliestPossiblePowerDownEntryTime() const {
    timestamp_t entryTime = 0;

    for (const auto &bank : m_banks) {
        entryTime = std::max(
            {entryTime,
                0 == bank.counter.act ? 0 : bank.cycles.act.get_start(), // TODO tRCD depends on read / write
                0 == bank.counter.pre ? 0 : bank.latestPre + m_memSpec.memTimingSpec.tRP,
                bank.refreshEndTime});
    }

    return entryTime;
}

void HBM2Core::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
    // resize banks and stacks
    stats.bank.resize(m_memSpec.numberOfBanks * m_memSpec.numberOfStacks);
    stats.rank_total.resize(1);

    auto simulation_duration = timestamp;
    for (size_t j = 0; j < m_memSpec.numberOfBanks * m_memSpec.numberOfStacks; ++j) {
        stats.bank[j].counter = m_banks[j].counter;
        stats.bank[j].cycles.act =
            m_banks[j].cycles.act.get_count_at(timestamp);
        stats.bank[j].cycles.selfRefresh =
            m_cycles.sref.get_count_at(timestamp);
        stats.bank[j].cycles.powerDownAct =
            m_cycles.powerDownAct.get_count_at(timestamp);
        stats.bank[j].cycles.powerDownPre =
            m_cycles.powerDownPre.get_count_at(timestamp);
        stats.bank[j].cycles.pre =
            simulation_duration - (stats.bank[j].cycles.act +
                                    m_cycles.powerDownAct.get_count_at(timestamp) +
                                    m_cycles.powerDownPre.get_count_at(timestamp) +
                                    m_cycles.sref.get_count_at(timestamp));
    }
    stats.rank_total[0].cycles.act = m_cycles.act.get_count_at(timestamp);
    stats.rank_total[0].cycles.powerDownAct = m_cycles.powerDownAct.get_count_at(timestamp);
    stats.rank_total[0].cycles.powerDownPre = m_cycles.powerDownPre.get_count_at(timestamp);
    stats.rank_total[0].cycles.selfRefresh = m_cycles.sref.get_count_at(timestamp);
    stats.rank_total[0].cycles.pre = simulation_duration - 
    (
        stats.rank_total[0].cycles.act +
        stats.rank_total[0].cycles.powerDownAct +
        stats.rank_total[0].cycles.powerDownPre +
        stats.rank_total[0].cycles.selfRefresh
    );
}

void HBM2Core::serialize(std::ostream& stream) const {
    m_commandCounter.serialize(stream);
    stream.write(reinterpret_cast<const char*>(&m_memState), sizeof(m_memState));
    stream.write(reinterpret_cast<const char* >(&m_counter.selfRefresh), sizeof(m_counter.selfRefresh));
    stream.write(reinterpret_cast<const char* >(&m_counter.deepSleepMode), sizeof(m_counter.deepSleepMode));
    stream.write(reinterpret_cast<const char* >(&m_endRefreshTime), sizeof(m_endRefreshTime));
    m_cycles.act.serialize(stream);
    m_cycles.sref.serialize(stream);
    m_cycles.powerDownAct.serialize(stream);
    m_cycles.powerDownPre.serialize(stream);
    m_cycles.deepSleepMode.serialize(stream);

    for (const auto& bank : m_banks) {
        bank.serialize(stream);
    }
}

void HBM2Core::deserialize(std::istream& stream) {
    m_commandCounter.deserialize(stream);
    stream.read(reinterpret_cast<char *>(&m_memState), sizeof(m_memState));
    stream.read(reinterpret_cast<char* >(&m_counter.selfRefresh), sizeof(m_counter.selfRefresh));
    stream.read(reinterpret_cast<char* >(&m_counter.deepSleepMode), sizeof(m_counter.deepSleepMode));
    stream.read(reinterpret_cast<char* >(&m_endRefreshTime), sizeof(m_endRefreshTime));
    m_cycles.act.deserialize(stream);
    m_cycles.sref.deserialize(stream);
    m_cycles.powerDownAct.deserialize(stream);
    m_cycles.powerDownPre.deserialize(stream);
    m_cycles.deepSleepMode.deserialize(stream);
    for (auto &bank : m_banks) {
        bank.deserialize(stream);
    }
}

} // namespace DRAMPower