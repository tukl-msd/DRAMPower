#include "HBM2Core.h"
#include "DRAMPower/dram/PseudoChannel.h"
#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/util/RegisterHelper.h"

#include <algorithm>

namespace DRAMPower {

HBM2Core::HBM2Core(const MemSpecHBM2& memSpec)
    : m_memSpec(memSpec)
    , m_helperMapping{memSpec.numberOfBanks}
    , m_pseudoChannels(memSpec.numberOfPseudoChannels, {memSpec.numberOfStacks * memSpec.numberOfBanks}) 
{}

void HBM2Core::doCommand(const HBM2Command& cmd) {
    assert(cmd.timestamp >= m_offset);
    m_implicitCommandHandler.processImplicitCommandQueue(*this, cmd.timestamp - m_offset, m_last_command_time);
    m_last_command_time = std::max(cmd.timestamp - m_offset, m_last_command_time);
    switch(cmd.type) {
        // Row commands
        case CmdType::ACT:
            util::coreHelpers::bankHandler(cmd, m_offset, m_pseudoChannels, m_helperMapping, &HBM2Core::handleAct, this);
            break;
        case CmdType::PRE:
            util::coreHelpers::bankHandler(cmd, m_offset, m_pseudoChannels, m_helperMapping, &HBM2Core::handlePre, this);
            break;
        case CmdType::PREA:
            util::coreHelpers::groupHandler(cmd, m_offset, m_pseudoChannels, m_helperMapping, &HBM2Core::handlePreAll, this);
            break;
        case CmdType::REFB:
            util::coreHelpers::bankHandlerIdx(cmd, m_offset, m_pseudoChannels, m_helperMapping, &HBM2Core::handleRefSingleBank, this);
            break;
        case CmdType::REFA:
            util::coreHelpers::groupHandlerIdx(cmd, m_offset, m_pseudoChannels, m_helperMapping, &HBM2Core::handleRefAll, this);
            break;
        case CmdType::PDEA:
            handlePowerDownActEntry(cmd.timestamp);
            break;
        case CmdType::PDEP:
            handlePowerDownPreEntry(cmd.timestamp);
            break;
        case CmdType::SREFEN:
            handleSelfRefreshEntry(cmd.timestamp);
            break;
        case CmdType::SREFEX:
            handleSelfRefreshExit(cmd.timestamp);
            break;
        case CmdType::PDXA:
            handlePowerDownActExit(cmd.timestamp);
            break;
        case CmdType::PDXP:
            handlePowerDownPreExit(cmd.timestamp);
            break;
        // Column commands
        case CmdType::RD:
            util::coreHelpers::bankHandler(cmd, m_offset, m_pseudoChannels, m_helperMapping, &HBM2Core::handleRead, this);
            break;
        case CmdType::RDA:
            util::coreHelpers::bankHandlerIdx(cmd, m_offset, m_pseudoChannels, m_helperMapping, &HBM2Core::handleReadAuto, this);
            break;
        case CmdType::WR:
            util::coreHelpers::bankHandler(cmd, m_offset, m_pseudoChannels, m_helperMapping, &HBM2Core::handleWrite, this);
            break;
        case CmdType::WRA:
            util::coreHelpers::bankHandlerIdx(cmd, m_offset, m_pseudoChannels, m_helperMapping, &HBM2Core::handleWriteAuto, this);
            break;
        case CmdType::END_OF_SIMULATION:
            break;
        default:
            assert(false && "Unsupported command");
            break;
    }
}

void HBM2Core::setSimulationTime(timestamp_t timestamp) {
    m_offset = timestamp;
}

void HBM2Core::reset() {
    for (auto& entry : m_pseudoChannels) {
        entry.reset();
    }
    m_implicitCommandHandler.reset();
    m_last_command_time = 0;
}

timestamp_t HBM2Core::getLastCommandTime() const {
    return m_last_command_time + m_offset;
}

bool HBM2Core::isSerializable() const {
    return 0 == m_implicitCommandHandler.implicitCommandCount();
}

void HBM2Core::handleAct(PseudoChannel &pseudoChannel, Bank &bank, timestamp_t timestamp) {
    if (bank.bankState == Bank::BankState::BANK_ACTIVE) return;
    bank.bankState = Bank::BankState::BANK_ACTIVE;
    bank.counter.act++;
    bank.cycles.act.start_interval(timestamp);
    pseudoChannel.cycles.act.start_interval_if_not_running(timestamp);
}

void HBM2Core::handlePre_impl(PseudoChannel &pseudoChannel, Bank &bank, timestamp_t timestamp) {
    if (bank.bankState == Bank::BankState::BANK_PRECHARGED) return;
    bank.bankState = Bank::BankState::BANK_PRECHARGED;
    bank.latestPre = timestamp;
    bank.cycles.act.close_interval(timestamp);
    if ( !pseudoChannel.isActive() ) {
        pseudoChannel.cycles.act.close_interval(timestamp);
    }
}

void HBM2Core::handlePre(PseudoChannel &pseudoChannel, Bank &bank, timestamp_t timestamp) {
    ++bank.counter.pre;
    handlePre_impl(pseudoChannel, bank, timestamp);
}

void HBM2Core::handlePreAll(PseudoChannel &pseudoChannel, timestamp_t timestamp) {
    for (auto &bank: pseudoChannel.banks) {
        ++bank.counter.pre;
        handlePre_impl(pseudoChannel, bank, timestamp);
    }
}

void HBM2Core::handleRefAll(std::size_t pc_idx, timestamp_t timestamp) {
    auto& pseudoChannel = m_pseudoChannels[pc_idx];
    auto timing = m_memSpec.tRFC;
    pseudoChannel.endRefreshTime = timestamp + timing;
    for (std::size_t bank_idx = 0; bank_idx < pseudoChannel.banks.size(); ++bank_idx) {
        auto& counter = pseudoChannel.banks[bank_idx].counter.refAllBank;
        handleRefreshOnBank(pc_idx, bank_idx, timestamp, timing, counter);
    }
}

void HBM2Core::handleRefSingleBank(std::size_t pc_idx, std::size_t bank_idx, timestamp_t timestamp) {
    auto& counter = m_pseudoChannels[pc_idx].banks[bank_idx].counter.refPerBank;
    handleRefreshOnBank(pc_idx, bank_idx, timestamp, m_memSpec.tRFCSB, counter);
}

void HBM2Core::handleRefreshOnBank(std::size_t pc_idx, std::size_t bank_idx, timestamp_t timestamp, uint64_t timing, uint64_t& counter) {
    ++counter;
    auto& pseudoChannel = m_pseudoChannels[pc_idx];
    auto& bank = pseudoChannel.banks[bank_idx];
    // assert(bank.bankState == Bank::BankState::BANK_ACTIVE && "Bank must be in active state for a refresh command"); // TODO validate
    pseudoChannel.cycles.act.start_interval_if_not_running(timestamp);
    bank.cycles.act.start_interval_if_not_running(timestamp);

    // Refresh counter incremented at timestamp_end
    const auto timestamp_end = timestamp + timing;
    bank.refreshEndTime = timestamp_end;
    // Execute implicit pre-charge at refresh end
    m_implicitCommandHandler.addImplicitCommand(timestamp_end, [bank_idx, pc_idx, timestamp_end](HBM2Core& self) {
        PseudoChannel& pseudoChannel = self.m_pseudoChannels[pc_idx];
        Bank bank = pseudoChannel.banks[bank_idx];
        self.handlePre_impl(pseudoChannel, bank, timestamp_end);
    });
}

void HBM2Core::handleRead(PseudoChannel&, Bank &bank, timestamp_t) {
    ++bank.counter.reads;
}

void HBM2Core::handleReadAuto(std::size_t pc_idx, std::size_t bank_idx, timestamp_t timestamp) {
    auto& bank = m_pseudoChannels[pc_idx].banks[bank_idx];
    ++bank.counter.readAuto;

    const auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.tRAS;
    const auto minReadActiveTime = timestamp + m_memSpec.prechargeOffsetRD;

    const auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

    bank.latestAutoPreFinished = delayed_timestamp;
    // Execute PRE after minimum active time
    m_implicitCommandHandler.addImplicitCommand(delayed_timestamp, [pc_idx, bank_idx, delayed_timestamp](HBM2Core& self) {
        auto& pseudoChannel = self.m_pseudoChannels[pc_idx];
        auto& bank = pseudoChannel.banks[bank_idx];
        self.handlePre(pseudoChannel, bank, delayed_timestamp);
    });
}

void HBM2Core::handleWrite(PseudoChannel&, Bank &bank, timestamp_t) {
    ++bank.counter.writes;
}

void HBM2Core::handleWriteAuto(std::size_t pc_idx, std::size_t bank_idx, timestamp_t timestamp) {
    auto& bank = m_pseudoChannels[pc_idx].banks[bank_idx];
    ++bank.counter.writeAuto;

    const auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.tRAS;
    const auto minWriteActiveTime =  timestamp + m_memSpec.prechargeOffsetWR;

    const auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

    bank.latestAutoPreFinished = delayed_timestamp;
    // Execute PRE after minimum active time
    m_implicitCommandHandler.addImplicitCommand(delayed_timestamp, [pc_idx, bank_idx, delayed_timestamp](HBM2Core& self) {
        auto& pseudoChannel = self.m_pseudoChannels[pc_idx];
        auto& bank = pseudoChannel.banks[bank_idx];
        self.handlePre(pseudoChannel, bank, delayed_timestamp);
    });
}

void HBM2Core::handleSelfRefreshEntry(timestamp_t timestamp) {
    auto timestampSelfRefreshStart = timestamp + m_memSpec.tRFC;
    for (std::size_t pc_idx = 0; pc_idx < m_pseudoChannels.size(); ++pc_idx) {
        // Issue implicit refresh
        handleRefAll(pc_idx, timestamp);
        // Handle self-refresh entry after tRFC // TODO verify
        m_implicitCommandHandler.addImplicitCommand(timestampSelfRefreshStart, [pc_idx](HBM2Core& self) {
            auto& pseudoChannel = self.m_pseudoChannels[pc_idx];
            pseudoChannel.counter.selfRefresh++;
            pseudoChannel.memState = MemState::SREF;
        });
    }
    m_implicitCommandHandler.addImplicitCommand(timestampSelfRefreshStart, [timestampSelfRefreshStart](HBM2Core& self){
        self.m_cycles.sref.start_interval(timestampSelfRefreshStart);
    });
}

void HBM2Core::handleSelfRefreshExit(timestamp_t timestamp) {
    for (PseudoChannel& pseudoChannel : m_pseudoChannels) {
        assert(pseudoChannel.memState == MemState::SREF && "SelfRefreshExit is only valid if it is preceded by a SelfRefreshEntry command");
        pseudoChannel.memState = MemState::NOT_IN_PD;
    }
    m_cycles.sref.close_interval(timestamp);
}

void HBM2Core::handlePowerDownActEntry(timestamp_t timestamp) {
    timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());
    for (std::size_t pc_idx = 0; pc_idx < m_pseudoChannels.size(); ++pc_idx) {
        m_implicitCommandHandler.addImplicitCommand(timestamp, [pc_idx, timestamp](HBM2Core& self) {
            PseudoChannel& pseudoChannel = self.m_pseudoChannels[pc_idx];
            pseudoChannel.memState = MemState::PDN_ACT;
            pseudoChannel.cycles.act.close_interval(timestamp);
            for (auto & bank : pseudoChannel.banks) {
                bank.cycles.act.close_interval(timestamp);
            }
        });
    }
    m_implicitCommandHandler.addImplicitCommand(timestamp, [timestamp](HBM2Core& self){
        self.m_cycles.powerDownAct.start_interval(timestamp);
    });
}

void HBM2Core::handlePowerDownActExit(timestamp_t timestamp) {
    timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());
    for (std::size_t pc_idx = 0; pc_idx < m_pseudoChannels.size(); ++pc_idx) {
        PseudoChannel& pseudoChannel = m_pseudoChannels[pc_idx];
        assert(pseudoChannel.memState == MemState::PDN_ACT && "PowerDownActExit is only valid if it is preceded by a PowerDownActEntry command");
        
        m_implicitCommandHandler.addImplicitCommand(timestamp, [pc_idx, timestamp](HBM2Core& self) {
            PseudoChannel& pseudoChannel = self.m_pseudoChannels[pc_idx];
            pseudoChannel.memState = MemState::NOT_IN_PD;

            // Activate banks that were active prior to PDA
            for (auto & bank : pseudoChannel.banks) {
                if (Bank::BankState::BANK_ACTIVE == bank.bankState) {
                    bank.cycles.act.start_interval(timestamp);
                }
            }

            // Activate if at least one bank is active
            if (pseudoChannel.isActive()) {
                pseudoChannel.cycles.act.start_interval(timestamp);
            }
        });
    }
    m_implicitCommandHandler.addImplicitCommand(timestamp, [timestamp](HBM2Core& self){
        self.m_cycles.powerDownAct.close_interval(timestamp);
    });
}

void HBM2Core::handlePowerDownPreEntry(timestamp_t timestamp) {
    timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());
    for (std::size_t pc_idx = 0; pc_idx < m_pseudoChannels.size(); ++pc_idx) {
    
        m_implicitCommandHandler.addImplicitCommand(timestamp, [pc_idx, timestamp](HBM2Core& self) {
            PseudoChannel& pseudoChannel = self.m_pseudoChannels[pc_idx];
            for (auto &bank : pseudoChannel.banks) {
                bank.cycles.act.close_interval(timestamp);
            }
            pseudoChannel.memState = MemState::PDN_PRE;
            pseudoChannel.cycles.act.close_interval(timestamp);
        });
    }
    m_implicitCommandHandler.addImplicitCommand(timestamp, [timestamp](HBM2Core& self){
        self.m_cycles.powerDownPre.start_interval(timestamp);
    });
}

void HBM2Core::handlePowerDownPreExit(timestamp_t timestamp) {
    for (std::size_t pc_idx = 0; pc_idx < m_pseudoChannels.size(); ++pc_idx) {
        PseudoChannel& pseudoChannel = m_pseudoChannels[pc_idx];
        assert(pseudoChannel.memState == MemState::PDN_PRE && "PowerDownPreExit is only valid if it is preceded by a PowerDownPreEntry command");
        timestamp = std::max(timestamp, this->earliestPossiblePowerDownEntryTime());

        m_implicitCommandHandler.addImplicitCommand(timestamp, [pc_idx](HBM2Core& self) {
            PseudoChannel& pseudoChannel = self.m_pseudoChannels[pc_idx];
            pseudoChannel.memState = MemState::NOT_IN_PD;
        });
    }
    m_implicitCommandHandler.addImplicitCommand(timestamp, [timestamp](HBM2Core& self){
        self.m_cycles.powerDownPre.close_interval(timestamp);
    });
}

timestamp_t HBM2Core::earliestPossiblePowerDownEntryTime() const {
    timestamp_t entryTime = 0;

    for (const auto &pseudoChannel : m_pseudoChannels) {
        for (const auto &bank : pseudoChannel.banks) {
            entryTime = std::max(
                {entryTime,
                    0 == bank.counter.act ? 0 : bank.cycles.act.get_start() + std::max(m_memSpec.tRCDRD, m_memSpec.tRCDWR), // TODO: verify
                    0 == bank.counter.pre ? 0 : bank.latestPre + m_memSpec.tRP,
                    (0 == bank.counter.readAuto && 0 == bank.counter.writeAuto) ? 0 : bank.latestAutoPreFinished,
                    bank.refreshEndTime});
        }
    }

    return entryTime;
}

void HBM2Core::getWindowStats(timestamp_t timestamp, SimulationStats &stats) {
    assert(timestamp >= m_offset);
    timestamp = timestamp - m_offset;
    m_implicitCommandHandler.processImplicitCommandQueue(*this, timestamp, m_last_command_time);
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
    stream.write(reinterpret_cast<const char*>(&m_last_command_time), sizeof(m_last_command_time));
    stream.write(reinterpret_cast<const char*>(&m_offset), sizeof(m_offset));
    for (const auto &pseudoChannel : m_pseudoChannels) {
        pseudoChannel.serialize(stream);
    }
}

void HBM2Core::deserialize(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&m_last_command_time), sizeof(m_last_command_time));
    stream.read(reinterpret_cast<char*>(&m_offset), sizeof(m_offset));
    for (auto &pseudoChannel : m_pseudoChannels) {
        pseudoChannel.deserialize(stream);
    }
}

} // namespace DRAMPower
