#ifndef DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H
#define DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H

#include <DRAMPower/util/bus.h>
#include <DRAMPower/util/databus.h>
#include <DRAMPower/util/clock.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/dram/Interface.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/memspec/MemSpecLPDDR4.h>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/util/cycle_stats.h>

#include <DRAMUtils/config/toggling_rate.h>

#include <deque>
#include <algorithm>
#include <stdint.h>
#include <vector>

namespace DRAMPower {

class LPDDR4 : public dram_base<CmdType>{
public:
    LPDDR4(const MemSpecLPDDR4& memSpec);
    virtual ~LPDDR4() = default;
public:
    MemSpecLPDDR4 memSpec;
    std::vector<Rank> ranks;
    util::Bus commandBus;
    util::DataBus dataBus;
    util::Clock readDQS;
    util::Clock writeDQS;

    util::Clock clock;

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

    void registerCommands();
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
public:
    SimulationStats getStats() override;
    uint64_t getBankCount() override;
    uint64_t getRankCount() override;
    uint64_t getDeviceCount() override;


    void handleAct(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handlePre(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handlePreAll(Rank & rank, timestamp_t timestamp);
    void handleRead(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleWrite(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleReadAuto(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleWriteAuto(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleRefAll(Rank & rank, timestamp_t timestamp);
    void handleRefPerBank(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleRefreshOnBank(Rank & rank, Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t & counter);
    void handleSelfRefreshEntry(Rank & rank, timestamp_t timestamp);
    void handleSelfRefreshExit(Rank & rank, timestamp_t timestamp);
    void handlePowerDownActEntry(Rank & rank, timestamp_t timestamp);
    void handlePowerDownActExit(Rank & rank, timestamp_t timestamp);
    void handlePowerDownPreEntry(Rank & rank, timestamp_t timestamp);
    void handlePowerDownPreExit(Rank & rank, timestamp_t timestamp);

    void endOfSimulation(timestamp_t timestamp);
private:
    timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) override;
    void enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp);
    void enableBus(timestamp_t timestamp, timestamp_t enable_timestamp);
    void handleInterfaceDQs(const Command& cmd, util::Clock &dqs, size_t length);
    void handleInterfaceOverrides(size_t length, bool read);
    void handleInterfaceCommandBus(const Command& cmd);
    void handleInterfaceData(const Command &cmd, bool read);
public:
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    SimulationStats getWindowStats(timestamp_t timestamp);
};

};

#endif /* DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H */
