#ifndef DRAMPOWER_STANDARDS_LPDDR6_LPDDR6_H
#define DRAMPOWER_STANDARDS_LPDDR6_LPDDR6_H

#include <stdint.h>

#include <algorithm>
#include <deque>
#include <vector>
#include <stdexcept>

#include <DRAMUtils/config/toggling_rate.h>

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/data/energy.h"
#include "DRAMPower/dram/Rank.h"
#include <DRAMPower/dram/Interface.h>
#include "DRAMPower/dram/dram_base.h"
#include "DRAMPower/memspec/MemSpec.h"
#include "DRAMPower/memspec/MemSpecLPDDR5.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/databus.h"
#include "DRAMPower/util/databus_presets.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/cycle_stats.h"
#include "DRAMPower/util/databus.h"

namespace DRAMPower {

class LPDDR6Interface {
// Public type definitions
public:
    using commandbus_t = util::Bus<7>;
    using databus_t = util::databus_presets::databus_preset_t;

// Public constructor
public:
    LPDDR6Interface(const MemSpecLPDDR5 &memSpec)
    : m_commandBus{7, 2, // modelled with datarate 2
        util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L}
    , m_dataBus{
        util::databus_presets::getDataBusPreset(
            memSpec.bitWidth * memSpec.numberOfDevices,
            util::DataBusConfig {
                memSpec.bitWidth * memSpec.numberOfDevices,
                memSpec.dataRate,
                util::BusIdlePatternSpec::L,
                util::BusInitPatternSpec::L,
                DRAMUtils::Config::TogglingRateIdlePattern::L,
                0.0,
                0.0,
                util::DataBusMode::Bus
            }
        )
    }
    , m_readDQS(memSpec.dataRate, true)
    , m_wck(memSpec.dataRate / memSpec.memTimingSpec.WCKtoCK, !memSpec.wckAlwaysOnMode)
    , m_clock(2, false)
    {}

// Public members
public:
    commandbus_t m_commandBus;
    databus_t m_dataBus;
    util::Clock m_readDQS;
    util::Clock m_wck;
    util::Clock m_clock;
};

class LPDDR6 : public dram_base<CmdType> {

public:
    LPDDR6(const MemSpecLPDDR5& memSpec);
    virtual ~LPDDR6() = default;

public:
    MemSpecLPDDR5 memSpec;
    std::vector<Rank> ranks;
    using commandbus_t = util::Bus<7>;
    using databus_t =  util::databus_presets::databus_preset_t;
private:
    std::vector<LPDDR6Interface> interface;
    bool efficiencyMode;

public:

    SimulationStats getStats() override;
    uint64_t getBankCount() override;
    uint64_t getRankCount() override;
    uint64_t getDeviceCount() override;

    // Commands
    void handleAct(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handlePre(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handlePreAll(Rank& rank, timestamp_t timestamp);
    void handleRead(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleWrite(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleReadAuto(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleWriteAuto(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleRefAll(Rank& rank, timestamp_t timestamp);
    void handleRefPerBank(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleRefPerTwoBanks(Rank& rank, std::size_t bank_id, timestamp_t timestamp);
    void handleRefreshOnBank(Rank& rank, Bank& bank, timestamp_t timestamp, uint64_t timing,
                             uint64_t& counter);
    void handleSelfRefreshEntry(Rank& rank, timestamp_t timestamp);
    void handleSelfRefreshExit(Rank& rank, timestamp_t timestamp);
    void handlePowerDownActEntry(Rank& rank, timestamp_t timestamp);
    void handlePowerDownActExit(Rank& rank, timestamp_t timestamp);
    void handlePowerDownPreEntry(Rank& rank, timestamp_t timestamp);
    void handlePowerDownPreExit(Rank& rank, timestamp_t timestamp);
    void handleDSMEntry(Rank& rank, timestamp_t timestamp);
    void handleDSMExit(Rank& rank, timestamp_t timestamp);
    void endOfSimulation(timestamp_t timestamp);
    SimulationStats getWindowStats(timestamp_t timestamp);
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;
    
private:
    // Calculations
    timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) override;
    void enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp);
    void enableBus(timestamp_t timestamp, timestamp_t enable_timestamp);
    void handleInterfaceDQs(const Command& cmd, util::Clock &dqs, size_t length, uint64_t datarate);
    void handleInterfaceOverrides(size_t length, bool read);
    void handleInterfaceCommandBus(const Command& cmd);
    void handleInterfaceData(const Command &cmd, bool read);

private:

    template<dram_base::commandEnum_t Cmd, typename Func>
    void registerInterfaceMember(Func && member_func) {
        this->routeInterfaceCommand<Cmd>([this, member_func](const Command & command) {
            (this->*member_func)(command);
        });
    }

    template <dram_base::commandEnum_t Cmd, typename Func>
    void registerBankHandler(Func&& member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command& command) {
            assert(this->ranks.size()>command.targetCoordinate.rank);
            auto& rank = this->ranks.at(command.targetCoordinate.rank);

            assert(rank.banks.size()>command.targetCoordinate.bank);
            auto& bank = rank.banks.at(command.targetCoordinate.bank);
            
            rank.commandCounter.inc(command.type);
            (this->*member_func)(rank, bank, command.timestamp);
        });
    }

    template <dram_base::commandEnum_t Cmd, typename Func>
    void registerBankGroupHandler(Func&& member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command& command) {
            assert(this->ranks.size()>command.targetCoordinate.rank);
            auto& rank = this->ranks.at(command.targetCoordinate.rank);
            
            assert(rank.banks.size()>command.targetCoordinate.bank);
            if (command.targetCoordinate.bank >= rank.banks.size()) {
                throw std::invalid_argument("Invalid bank targetcoordinate");
            }
            auto bank_id = command.targetCoordinate.bank;
            
            rank.commandCounter.inc(command.type);
            (this->*member_func)(rank, bank_id, command.timestamp);
        });
    }

    template <dram_base::commandEnum_t Cmd, typename Func>
    void registerRankHandler(Func&& member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command& command) {
            assert(this->ranks.size()>command.targetCoordinate.rank);
            auto& rank = this->ranks.at(command.targetCoordinate.rank);

            rank.commandCounter.inc(command.type);
            (this->*member_func)(rank, command.timestamp);
        });
    }

    template <dram_base::commandEnum_t Cmd, typename Func>
    void registerHandler(Func&& member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command& command) {
            (this->*member_func)(command.timestamp);
        });
    }

    void registerCommands();
    timestamp_t earliestPossiblePowerDownEntryTime(Rank& rank);

};

};  // namespace DRAMPower


#endif /* DRAMPOWER_STANDARDS_LPDDR6_LPDDR6_H */
