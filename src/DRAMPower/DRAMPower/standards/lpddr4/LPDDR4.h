#ifndef DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H
#define DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H

#include <DRAMPower/util/bus.h>
#include <DRAMPower/util/clock.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/memspec/MemSpecLPDDR4.h>

#include <DRAMPower/data/energy.h>

#include <DRAMPower/util/cycle_stats.h>

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
    util::Bus readBus;
    util::Bus writeBus;
    util::Clock readDQS;
    util::Clock writeDQS;

    util::Clock clock;
private:
    TogglingHandle togglingHandleRead;
    TogglingHandle togglingHandleWrite;


    //util::Bus dataBus;
protected:
    template<dram_base::commandEnum_t Cmd, typename Func>
    void registerBankHandler(Func && member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command & command) {
            auto & rank = this->ranks[command.targetCoordinate.rank];
            auto & bank = rank.banks[command.targetCoordinate.bank];
            rank.commandCounter.inc(command.type);
            (this->*member_func)(rank, bank, command.timestamp);
        });
    }

    template<dram_base::commandEnum_t Cmd, typename Func>
    void registerRankHandler(Func && member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command & command) {
            auto & rank = this->ranks[command.targetCoordinate.rank];

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
    void handle_interface(const Command& cmd) override;
    void handle_interface_toggleRate(const Command& cmd) override;
    void update_toggling_rate(const std::optional<ToggleRateDefinition> &toggleRateDefinition) override;
    void handleInterfaceOverrides(size_t length, bool read);
    void handle_interface_commandbus(const Command& cmd);
    void handle_interface_data_common(const Command &cmd, const size_t length);
public:
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    SimulationStats getWindowStats(timestamp_t timestamp);
};

};

#endif /* DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H */
