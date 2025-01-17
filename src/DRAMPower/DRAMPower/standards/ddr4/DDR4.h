#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4_H

#include <DRAMPower/util/bus.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/dram/Interface.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/memspec/MemSpecDDR4.h>

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
    MemSpecDDR4 memSpec;
    std::vector<Rank> ranks;
    util::Bus readBus;
    util::Bus writeBus;

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
    util::Bus commandBus;
    uint64_t prepostambleReadMinTccd;
    uint64_t prepostambleWriteMinTccd;
private:
    TogglingHandle togglingHandleRead;
    TogglingHandle togglingHandleWrite;

protected:
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

    void registerPatterns();
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
    void handle_interface_data_common(const Command& cmd, size_t length);
    void handle_interface_commandbus(const Command& cmd);
    void handle_interface(const Command& cmd) override;
    void handle_interface_toggleRate(const Command& cmd) override;
    void toggling_rate_enable(timestamp_t timestamp, timestamp_t enable_timestamp, DRAMPower::util::Bus &bus, DRAMPower::TogglingHandle &togglinghandle);
    void toggling_rate_disable(timestamp_t timestamp, timestamp_t disable_timestamp, DRAMPower::util::Bus &bus, DRAMPower::TogglingHandle &togglinghandle);
    timestamp_t toggling_rate_get_enable_time(timestamp_t timestamp);
    timestamp_t toggling_rate_get_disable_time(timestamp_t timestamp);
    timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) override;
    void handleInterfaceOverrides(size_t length, bool read);
    uint64_t getInitEncoderPattern() override;
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
