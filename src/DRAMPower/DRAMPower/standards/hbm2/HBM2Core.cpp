#include "HBM2Core.h"
#include "DRAMPower/dram/Rank.h"

#include <algorithm>

namespace DRAMPower {

HBM2Core::HBM2Core(const MemSpecHBM2& memSpec, implicitCommandInserter_t&& implicitCommandInserter)
    : m_pseudoChannels(memSpec.numberOfPseudoChannels, {memSpec.numberOfStacks * memSpec.numberOfBanks}) 
    , m_memSpec(memSpec)
    , m_implicitCommandInserter(std::move(implicitCommandInserter))
{}

void HBM2Core::handleAct(PseudoChannel &pseudoChannel, Bank &bank, timestamp_t timestamp) {
    if (bank.bankState == Bank::BankState::BANK_ACTIVE) return;
    bank.bankState = Bank::BankState::BANK_ACTIVE;
    bank.counter.act++;
    bank.cycles.act.start_interval(timestamp);
    pseudoChannel.cycles.act.start_interval_if_not_running(timestamp);
}

void HBM2Core::handlePre_impl(PseudoChannel &pseudoChannel, Bank &bank, timestamp_t timestamp, uint64_t& counter) {
    if (bank.bankState == Bank::BankState::BANK_PRECHARGED) return;
    bank.bankState = Bank::BankState::BANK_PRECHARGED;
    counter++;
    bank.latestPre = timestamp;
    bank.cycles.act.close_interval(timestamp);
    if ( !pseudoChannel.isActive(timestamp) ) {
        pseudoChannel.cycles.act.close_interval(timestamp);
    }
}

void HBM2Core::handlePre(PseudoChannel &pseudoChannel, Bank &bank, timestamp_t timestamp) {
    handlePre_impl(pseudoChannel, bank, timestamp, bank.counter.pre);
}

void HBM2Core::handlePreAll(PseudoChannel &pseudoChannel, timestamp_t timestamp) {
    for (auto &bank: pseudoChannel.banks) {
        handlePre_impl(pseudoChannel, bank, timestamp, bank.counter.pre);
    }
}

void HBM2Core::handleRefAll(PseudoChannel &pseudoChannel, timestamp_t timestamp) {
    auto timing = m_memSpec.memTimingSpec.tRFC;
    pseudoChannel.endRefreshTime = timestamp + timing;
    for (auto& bank : pseudoChannel.banks) {
        // TODO: In contrast to the other standards the counter is increased when the precharge is executed
        handleRefreshOnBank(pseudoChannel, bank, timestamp, timing, bank.counter.refAllBank);
    }
}

void HBM2Core::handleRefSingleBank(PseudoChannel &pseudoChannel, Bank & bank, timestamp_t timestamp) {
    handleRefreshOnBank(pseudoChannel, bank, timestamp, m_memSpec.memTimingSpec.tRFCSB, bank.counter.refPerBank);
}

void HBM2Core::handleRefreshOnBank(PseudoChannel &pseudoChannel, Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t& counter) {
    assert(bank.bankState == Bank::BankState::BANK_ACTIVE && "Bank must be in active state for a refresh command");
    pseudoChannel.cycles.act.start_interval_if_not_running(timestamp);
    bank.cycles.act.start_interval_if_not_running(timestamp);

    // Refresh counter incremented at timestamp_end
    const auto timestamp_end = timestamp + timing;
    bank.refreshEndTime = timestamp_end;
    // Execute implicit pre-charge at refresh end
    m_implicitCommandInserter.addImplicitCommand(timestamp_end, [this, &bank, &pseudoChannel, timestamp_end, &counter]() {
        handlePre_impl(pseudoChannel, bank, timestamp_end, counter);
    });
}

void HBM2Core::handleRead(PseudoChannel& ,Bank &bank, timestamp_t) {
    ++bank.counter.reads;
}

void HBM2Core::handleReadAuto(PseudoChannel& pseudoChannel, Bank &bank, timestamp_t timestamp) {
    ++bank.counter.readAuto;

    const auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    const auto minReadActiveTime = timestamp + m_memSpec.prechargeOffsetRD;

    const auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

    bank.latestAutoPreFinished = delayed_timestamp;
    // Execute PRE after minimum active time
    m_implicitCommandInserter.addImplicitCommand(delayed_timestamp, [this, &pseudoChannel, &bank, delayed_timestamp]() {
        this->handlePre(pseudoChannel, bank, delayed_timestamp);
    });
}

void HBM2Core::handleWrite(PseudoChannel&, Bank &bank, timestamp_t) {
    ++bank.counter.writes;
}

void HBM2Core::handleWriteAuto(PseudoChannel& pseudoChannel, Bank &bank, timestamp_t timestamp) {
    ++bank.counter.writeAuto;

    const auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    const auto minWriteActiveTime =  timestamp + m_memSpec.prechargeOffsetWR;

    const auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

    bank.latestAutoPreFinished = delayed_timestamp;
    // Execute PRE after minimum active time
    m_implicitCommandInserter.addImplicitCommand(delayed_timestamp, [this, &pseudoChannel, &bank, delayed_timestamp]() {
        this->handlePre(pseudoChannel, bank, delayed_timestamp);
    });
}

void HBM2Core::handleSelfRefreshEntry(timestamp_t timestamp) {
    auto timestampSelfRefreshStart = timestamp + m_memSpec.memTimingSpec.tRFC;
    for (PseudoChannel pseudoChannel : m_pseudoChannels) {
        // Issue implicit refresh
        handleRefAll(pseudoChannel, timestamp);
        // Handle self-refresh entry after tRFC // TODO verify
        m_implicitCommandInserter.addImplicitCommand(timestampSelfRefreshStart, [&pseudoChannel]() {
            pseudoChannel.counter.selfRefresh++;
            pseudoChannel.memState = MemState::SREF;
        });
    }
    m_implicitCommandInserter.addImplicitCommand(timestampSelfRefreshStart, [this, timestampSelfRefreshStart](){
        m_cycles.sref.start_interval(timestampSelfRefreshStart);
    });
}

void HBM2Core::handleSelfRefreshExit(timestamp_t timestamp) {
    for (PseudoChannel pseudoChannel : m_pseudoChannels) {
        assert(pseudoChannel.memState == MemState::SREF && "SelfRefreshExit is only valid if it is preceded by a SelfRefreshEntry command");
        pseudoChannel.memState = MemState::NOT_IN_PD;
    }
    m_cycles.sref.close_interval(timestamp);
}

void HBM2Core::handlePowerDownActEntry(timestamp_t timestamp) {
    timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());
    for (PseudoChannel pseudoChannel : m_pseudoChannels) {
        m_implicitCommandInserter.addImplicitCommand(timestamp, [&pseudoChannel, timestamp]() {
            pseudoChannel.memState = MemState::PDN_ACT;
            pseudoChannel.cycles.act.close_interval(timestamp);
            for (auto & bank : pseudoChannel.banks) {
                bank.cycles.act.close_interval(timestamp);
            }
        });
    }
    m_implicitCommandInserter.addImplicitCommand(timestamp, [this, timestamp](){
        m_cycles.powerDownAct.start_interval(timestamp);
    });
}

void HBM2Core::handlePowerDownActExit(timestamp_t timestamp) {
    timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());
    for (PseudoChannel pseudoChannel : m_pseudoChannels) {
        assert(pseudoChannel.memState == MemState::PDN_ACT && "PowerDownActExit is only valid if it is preceded by a PowerDownActEntry command");
        
        m_implicitCommandInserter.addImplicitCommand(timestamp, [&pseudoChannel, timestamp]() {
            pseudoChannel.memState = MemState::NOT_IN_PD;

            // Activate banks that were active prior to PDA
            for (auto & bank : pseudoChannel.banks) {
                if (Bank::BankState::BANK_ACTIVE == bank.bankState) {
                    bank.cycles.act.start_interval(timestamp);
                }
            }

            // Activate if at least one bank is active
            if (pseudoChannel.isActive(timestamp)) {
                pseudoChannel.cycles.act.start_interval(timestamp);
            }
        });
    }
    m_implicitCommandInserter.addImplicitCommand(timestamp, [this, timestamp](){
        m_cycles.powerDownAct.close_interval(timestamp);
    });
}

void HBM2Core::handlePowerDownPreEntry(timestamp_t timestamp) {
    timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());
    for (PseudoChannel pseudoChannel : m_pseudoChannels) {
    
        m_implicitCommandInserter.addImplicitCommand(timestamp, [&pseudoChannel, timestamp]() {
            for (auto &bank : pseudoChannel.banks) {
                bank.cycles.act.close_interval(timestamp);
            }
            pseudoChannel.memState = MemState::PDN_PRE;
            pseudoChannel.cycles.act.close_interval(timestamp);
        });
    }
    m_implicitCommandInserter.addImplicitCommand(timestamp, [this, timestamp](){
        m_cycles.powerDownPre.start_interval(timestamp);
    });
}

void HBM2Core::handlePowerDownPreExit(timestamp_t timestamp) {
    for (PseudoChannel pseudoChannel : m_pseudoChannels) {
        assert(pseudoChannel.memState == MemState::PDN_PRE && "PowerDownPreExit is only valid if it is preceded by a PowerDownPreEntry command");
        timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());
        
        m_implicitCommandInserter.addImplicitCommand(timestamp, [&pseudoChannel, timestamp]() {
            pseudoChannel.memState = MemState::NOT_IN_PD;

            // Precharge banks that were precharged prior to PDP
            for (auto & bank : pseudoChannel.banks)
            {
                if (Bank::BankState::BANK_ACTIVE == bank.bankState) {
                    bank.cycles.act.start_interval(timestamp);
                }
            }
            if(pseudoChannel.isActive(timestamp)) { // TODO: this is not used in any other standard -> verify
                pseudoChannel.cycles.act.start_interval(timestamp);
            }
        });
    }
    m_implicitCommandInserter.addImplicitCommand(timestamp, [this, timestamp](){
        m_cycles.powerDownPre.close_interval(timestamp);
    });
}

timestamp_t HBM2Core::earliestPossiblePowerDownEntryTime() const {
    timestamp_t entryTime = 0;

    for (const auto &pseudoChannel : m_pseudoChannels) {
        for (const auto &bank : pseudoChannel.banks) {
            entryTime = std::max(
                {entryTime,
                    0 == bank.counter.act ? 0 : bank.cycles.act.get_start() + std::max(m_memSpec.memTimingSpec.tRCDRD, m_memSpec.memTimingSpec.tRCDWR), // TODO: verify
                    0 == bank.counter.pre ? 0 : bank.latestPre + m_memSpec.memTimingSpec.tRP,
                    (0 == bank.counter.readAuto && 0 == bank.counter.writeAuto) ? 0 : bank.latestAutoPreFinished,
                    bank.refreshEndTime});
        }
    }

    return entryTime;
}

void HBM2Core::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
    // resize banks and stacks
    stats.bank.resize(m_memSpec.numberOfPseudoChannels * m_memSpec.numberOfBanks * m_memSpec.numberOfStacks);
    stats.rank_total.resize(m_memSpec.numberOfPseudoChannels);

    auto simulation_duration = timestamp;
    for (size_t i = 0; i < m_memSpec.numberOfPseudoChannels; ++i) {
        const PseudoChannel &pseudoChannel = m_pseudoChannels[i];
        size_t bank_offset = i * m_memSpec.numberOfBanks * m_memSpec.numberOfStacks;
        for (size_t j = 0; j < m_memSpec.numberOfBanks * m_memSpec.numberOfStacks; ++j) {
            stats.bank[bank_offset + j].counter = pseudoChannel.banks[j].counter;
            stats.bank[bank_offset + j].cycles.act =
                pseudoChannel.banks[j].cycles.act.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.selfRefresh =
                m_cycles.sref.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.powerDownAct =
                m_cycles.powerDownAct.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.powerDownPre =
                m_cycles.powerDownPre.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.pre =
                simulation_duration - (stats.bank[bank_offset + j].cycles.act +
                                        m_cycles.powerDownAct.get_count_at(timestamp) +
                                        m_cycles.powerDownPre.get_count_at(timestamp) +
                                        m_cycles.sref.get_count_at(timestamp));
        }
        stats.rank_total[i].cycles.act = pseudoChannel.cycles.act.get_count_at(timestamp);
        stats.rank_total[i].cycles.powerDownAct = m_cycles.powerDownAct.get_count_at(timestamp);
        stats.rank_total[i].cycles.powerDownPre = m_cycles.powerDownPre.get_count_at(timestamp);
        stats.rank_total[i].cycles.selfRefresh = m_cycles.sref.get_count_at(timestamp);
        stats.rank_total[i].cycles.pre = simulation_duration - 
        (
            stats.rank_total[i].cycles.act +
            stats.rank_total[i].cycles.powerDownAct +
            stats.rank_total[i].cycles.powerDownPre +
            stats.rank_total[i].cycles.selfRefresh
        );
    }
}

void HBM2Core::serialize(std::ostream& stream) const {
    for (const auto &pseudoChannel : m_pseudoChannels) {
        pseudoChannel.serialize(stream);
    }
}

void HBM2Core::deserialize(std::istream& stream) {
    for (auto &pseudoChannel : m_pseudoChannels) {
        pseudoChannel.deserialize(stream);
    }
}

} // namespace DRAMPower