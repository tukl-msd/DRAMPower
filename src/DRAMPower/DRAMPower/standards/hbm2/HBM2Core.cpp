#include "HBM2Core.h"
#include "DRAMPower/dram/Rank.h"

namespace DRAMPower {

void HBM2Core::handleAct(Stack_t &stack, Bank &bank, timestamp_t timestamp) {
    if (bank.bankState == Bank::BankState::BANK_ACTIVE) return;
    bank.bankState = Bank::BankState::BANK_ACTIVE;
    bank.counter.act++;
    bank.cycles.act.start_interval(timestamp);
    stack.cycles.act.start_interval_if_not_running(timestamp);
}

void HBM2Core::handlePre_impl(Stack_t &stack, Bank &bank, timestamp_t timestamp, uint64_t& counter) {
    if (bank.bankState == Bank::BankState::BANK_PRECHARGED) return;
    bank.bankState = Bank::BankState::BANK_PRECHARGED;
    counter++;
    bank.cycles.act.close_interval(timestamp);
    if ( !stack.isActive(timestamp) ) stack.cycles.act.close_interval(timestamp);
}

void HBM2Core::handlePre(Stack_t &stack, Bank &bank, timestamp_t timestamp) {
    handlePre_impl(stack, bank, timestamp, bank.counter.pre);
}

void HBM2Core::handlePreAll(Stack_t &stack, timestamp_t timestamp) {
    for (auto &bank: stack.banks) {
        handlePre(stack, bank, timestamp);
    }
}

void HBM2Core::handleRefAll(Stack_t &stack, timestamp_t timestamp) {
    for (auto& bank : stack.banks) {
        handleRefreshOnBank(stack, bank, timestamp, m_memSpec.memTimingSpec.tRFC, bank.counter.refAllBank); // TODO
    }
}

void HBM2Core::handleRefSingleBank(Stack_t & stack, Bank & bank, timestamp_t timestamp) {
    handleRefreshOnBank(stack, bank, timestamp, m_memSpec.memTimingSpec.tRFC, bank.counter.refPerBank); // TODO
}

void HBM2Core::handleRefreshOnBank(Stack_t & stack, Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t& counter) {
    assert(bank.bankState == Bank::BankState::BANK_ACTIVE && "Bank must be in active state for a refresh command");
    // Refresh counter incremented at timestamp_end
    bank.cycles.act.start_interval_if_not_running(timestamp);
    stack.cycles.act.start_interval_if_not_running(timestamp);

    // Execute implicit pre-charge at refresh end
    const auto timestamp_end = timestamp + timing;
    m_implicitCommandInserter.addImplicitCommand(timestamp_end, [&stack, &bank, timestamp_end, &counter]() {
        handlePre_impl(stack, bank, timestamp_end, counter);
    });
}

void HBM2Core::handleRead(Stack_t&, Bank &bank, timestamp_t) {
    ++bank.counter.reads;
}

void HBM2Core::handleReadAuto(Stack_t &stack, Bank &bank, timestamp_t timestamp) {
    ++bank.counter.readAuto;

    const auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    const auto minReadActiveTime = timestamp + m_memSpec.prechargeOffsetRD;

    const auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandInserter.addImplicitCommand(delayed_timestamp, [this, &stack, &bank, delayed_timestamp]() {
        this->handlePre(stack, bank, delayed_timestamp);
    });
}

void HBM2Core::handleWrite(Stack_t&, Bank &bank, timestamp_t) {
    ++bank.counter.writes;
}

void HBM2Core::handleWriteAuto(Stack_t &stack, Bank &bank, timestamp_t timestamp) {
    ++bank.counter.writeAuto;

    auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
    auto minWriteActiveTime =  timestamp + m_memSpec.prechargeOffsetWR;

    auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

    // Execute PRE after minimum active time
    m_implicitCommandInserter.addImplicitCommand(delayed_timestamp, [this, &stack, &bank, delayed_timestamp]() {
        this->handlePre(stack, bank, delayed_timestamp);
    });
}

void HBM2Core::handleSelfRefreshEntry(Stack_t &stack, timestamp_t timestamp) {
    // Issue implicit refresh
    handleRefAll(stack, timestamp);
    // Handle self-refresh entry after tRFC // TODO verify
    auto timestampSelfRefreshStart = timestamp + m_memSpec.memTimingSpec.tRFC;
    m_implicitCommandInserter.addImplicitCommand(timestampSelfRefreshStart, [&stack, timestampSelfRefreshStart]() {
        stack.counter.selfRefresh++;
        stack.cycles.sref.start_interval(timestampSelfRefreshStart);
        stack.memState = MemState::SREF;
    });
}

void HBM2Core::handleSelfRefreshExit(Stack_t &stack, timestamp_t timestamp) {
    assert(stack.memState == MemState::SREF && "SelfRefreshExit is only valid if it is preceded by a SelfRefreshEntry command");
    stack.cycles.sref.close_interval(timestamp);
    stack.memState = MemState::NOT_IN_PD;
}

void HBM2Core::handlePowerDownActEntry(Stack_t &stack, timestamp_t timestamp) {
    stack.memState = MemState::PDN_ACT;
    stack.cycles.powerDownAct.start_interval(timestamp);
    stack.cycles.act.close_interval(timestamp);
    //stack.cycles.pre.close_interval(timestamp);
    for (auto & bank : stack.banks) {
        bank.cycles.act.close_interval(timestamp);
    }
}

void HBM2Core::handlePowerDownActExit(Stack_t &stack, timestamp_t timestamp) {
    assert(stack.memState == MemState::PDN_ACT && "PowerDownActExit is only valid if it is preceded by a PowerDownActEntry command");
    stack.memState = MemState::NOT_IN_PD;
    stack.cycles.powerDownAct.close_interval(timestamp);

    // Activate banks that were active prior to PDA
    for (auto & bank : stack.banks) {
        if (bank.bankState==Bank::BankState::BANK_ACTIVE) {
            bank.cycles.act.start_interval(timestamp);
        }
    }
    // Activate stack if at least one bank is active
    // TODO At least one bank must be active for PDA -> remove if statement?
    if(stack.isActive(timestamp)) {
        stack.cycles.act.start_interval(timestamp);
    }
}

void HBM2Core::handlePowerDownPreEntry(Stack_t &stack, timestamp_t timestamp) {
    for (auto &bank : stack.banks) {
        bank.cycles.act.close_interval(timestamp);
    }
    stack.memState = MemState::PDN_PRE;
    stack.cycles.powerDownPre.start_interval(timestamp);
    stack.cycles.act.close_interval(timestamp);
}

void HBM2Core::handlePowerDownPreExit(Stack_t &stack, timestamp_t timestamp) {
    assert(stack.memState == MemState::PDN_PRE && "PowerDownPreExit is only valid if it is preceded by a PowerDownPreEntry command");
    stack.memState = MemState::NOT_IN_PD;
    stack.cycles.powerDownPre.close_interval(timestamp);

    // Precharge banks that were precharged prior to PDP
    for (auto & bank : stack.banks)
    {
        if (Bank::BankState::BANK_ACTIVE == bank.bankState) {
            bank.cycles.act.start_interval(timestamp);
        }
    }
    if (stack.isActive(timestamp)) {
        stack.cycles.act.close_interval(timestamp);
    }
}

void HBM2Core::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
    // resize banks and stacks
    stats.bank.resize(m_memSpec.numberOfBanks * m_memSpec.numberOfStacks);
    stats.rank_total.resize(m_memSpec.numberOfStacks);

    auto simulation_duration = timestamp;
    for (size_t i = 0; i < m_memSpec.numberOfStacks; ++i) {
        const Stack_t &stack = m_stacks[i];
        size_t bank_offset = i * m_memSpec.numberOfBanks;
        for (size_t j = 0; j < m_memSpec.numberOfBanks; ++j) {
            stats.bank[bank_offset + j].counter = stack.banks[j].counter;
            stats.bank[bank_offset + j].cycles.act =
                stack.banks[j].cycles.act.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.selfRefresh =
                stack.cycles.sref.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.powerDownAct =
                stack.cycles.powerDownAct.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.powerDownPre =
                stack.cycles.powerDownPre.get_count_at(timestamp);
            stats.bank[bank_offset + j].cycles.pre =
                simulation_duration - (stats.bank[bank_offset + j].cycles.act +
                                        stack.cycles.powerDownAct.get_count_at(timestamp) +
                                        stack.cycles.powerDownPre.get_count_at(timestamp) +
                                        stack.cycles.sref.get_count_at(timestamp));
        }
        stats.rank_total[i].cycles.act = stack.cycles.act.get_count_at(timestamp);
        stats.rank_total[i].cycles.powerDownAct = stack.cycles.powerDownAct.get_count_at(timestamp);
        stats.rank_total[i].cycles.powerDownPre = stack.cycles.powerDownPre.get_count_at(timestamp);
        stats.rank_total[i].cycles.selfRefresh = stack.cycles.sref.get_count_at(timestamp);
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
    // Serialize the ranks
    for (const auto& rank : m_stacks) {
        rank.serialize(stream);
    }
}

void HBM2Core::deserialize(std::istream& stream) {
    // Deserialize the ranks
    for (auto &rank : m_stacks) {
        rank.deserialize(stream);
    }
}

} // namespace DRAMPower