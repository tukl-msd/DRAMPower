#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4_H

#include <DRAMPower/util/pin.h>
#include <DRAMPower/util/bus.h>
#include <DRAMPower/util/databus.h>
#include <DRAMPower/util/databus_presets.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/dram/Interface.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/memspec/MemSpecDDR4.h>
#include <DRAMPower/util/bus_extensions.h>

#include <DRAMUtils/config/toggling_rate.h>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/util/cycle_stats.h>
#include <DRAMPower/util/clock.h>

#include <deque>
#include <algorithm>
#include <stdint.h>
#include <vector>
#include <optional>

namespace DRAMPower {

class DDR4 : public dram_base<CmdType>{
public:
    DDR4(const MemSpecDDR4 &memSpec);
    virtual ~DDR4() = default;
public:
    using commandbus_t = util::Bus<27>;
    using databus_t =  util::databus_presets::databus_preset_t<util::bus_extensions::BusExtensionDBI>;
    MemSpecDDR4 memSpec;
    std::vector<Rank> ranks;

// commandBus dependes on cmdBusInitPattern and cmdBusWidth
// cmdBusInitPattern must be initialized before commandBus
// cmdBusWidth must be initialized before cmdBusInitPattern
// See order of execution in initializer list
private:
    std::size_t cmdBusWidth;
    uint64_t cmdBusInitPattern;
    util::Clock readDQS_;
    util::Clock writeDQS_;
    util::Clock clock;
public:
    commandbus_t commandBus;
    uint64_t prepostambleReadMinTccd;
    uint64_t prepostambleWriteMinTccd;
public:
    databus_t dataBus;
private:
    std::vector<util::Pin> dbiread;
    std::vector<util::Pin> dbiwrite;

protected:

    template<dram_base::commandEnum_t Cmd, typename Func>
    void registerInterfaceMember(Func && member_func) {
        this->routeInterfaceCommand<Cmd>([this, member_func](const Command & command) {
            (this->*member_func)(command);
        });
    }

    template<dram_base::commandEnum_t Cmd, typename Func>
    void registerBankHandler(Func && member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command & command) {
            assert(this->ranks.size()>command.targetCoordinate.rank);
            auto & rank = this->ranks.at(command.targetCoordinate.rank);

            assert(rank.banks.size()>command.targetCoordinate.bank);
            auto & bank = rank.banks.at(command.targetCoordinate.bank);

            rank.commandCounter.inc(command.type);
            (this->*member_func)(rank, bank, command.timestamp);
        });
    }

    template<dram_base::commandEnum_t Cmd, typename Func>
    void registerRankHandler(Func && member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command & command) {
            assert(this->ranks.size()>command.targetCoordinate.rank);
            auto & rank = this->ranks.at(command.targetCoordinate.rank);

            rank.commandCounter.inc(command.type);
            (this->*member_func)(rank, command.timestamp);
        });
    }

    template<dram_base::commandEnum_t Cmd, typename Func>
    void registerHandler(Func && member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command & command) {
            (this->*member_func)(command.timestamp);
        });
    }

public:
    timestamp_t earliestPossiblePowerDownEntryTime(Rank & rank) {
        timestamp_t entryTime = 0;

        for (const auto & bank : rank.banks) {
            entryTime = std::max({ entryTime,
                                   bank.counter.act == 0 ? 0 :  bank.cycles.act.get_start() + memSpec.memTimingSpec.tRCD,
                                   bank.counter.pre == 0 ? 0 : bank.latestPre + memSpec.memTimingSpec.tRP,
                                   bank.refreshEndTime
                                 });
        }

        return entryTime;
    };
private:
    void handleInterfaceDQs(const Command& cmd, util::Clock &dqs, size_t length);
    void handleInterfaceCommandBus(const Command& cmd);
    void handleInterfaceData(const Command &cmd, bool read);
    void enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp);
    void enableBus(timestamp_t timestamp, timestamp_t enable_timestamp);
    timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) override;
    void handleInterfaceOverrides(size_t length, bool read);
    uint64_t getInitEncoderPattern() override;
    void registerCommands();
    void registerExtensions();
public:
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;
    SimulationStats getStats() override;
    uint64_t getBankCount() override;
    uint64_t getRankCount() override;
    uint64_t getDeviceCount() override;

    void handleAct(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handlePre(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handlePreAll(Rank & rank, timestamp_t timestamp); 
    void handleRefAll(Rank & rank, timestamp_t timestamp);
    void handleSelfRefreshEntry(Rank & rank, timestamp_t timestamp);
    void handleSelfRefreshExit(Rank & rank, timestamp_t timestamp);
    void handleRead(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleWrite(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleReadAuto(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleWriteAuto(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handlePowerDownActEntry(Rank & rank, timestamp_t timestamp);
    void handlePowerDownActExit(Rank & rank, timestamp_t timestamp);
    void handlePowerDownPreEntry(Rank & rank, timestamp_t timestamp);
    void handlePowerDownPreExit(Rank & rank, timestamp_t timestamp);
    void endOfSimulation(timestamp_t timestamp);

private:
    void handlePrePostamble(
        const timestamp_t   timestamp,
        const uint64_t      length,
        Rank &              rank,
        bool                read
    );

public:
    SimulationStats getWindowStats(timestamp_t timestamp);

};

};

#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4_H */
