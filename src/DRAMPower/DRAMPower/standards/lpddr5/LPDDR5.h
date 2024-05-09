#ifndef DRAMPOWER_STANDARDS_LPDDR5_LPDDR5_H
#define DRAMPOWER_STANDARDS_LPDDR5_LPDDR5_H

#include <stdint.h>

#include <algorithm>
#include <deque>
#include <vector>

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/data/energy.h"
#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/dram/dram_base.h"
#include "DRAMPower/memspec/MemSpec.h"
#include "DRAMPower/memspec/MemSpecLPDDR5.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/cycle_stats.h"

namespace DRAMPower {
class LPDDR5 : public dram_base<CmdType> {
   public:
    LPDDR5(const MemSpecLPDDR5& memSpec);
    virtual ~LPDDR5() = default;

    void handleInterfaceOverrides(size_t length, bool read);

    SimulationStats getStats() override;
	energy_t calcEnergy(timestamp_t timestamp) override;
    uint64_t getBankCount() override;
    uint64_t getRankCount() override;
    uint64_t getDeviceCount() override;

    // Commands
    void handle_interface(const Command& cmd) override;
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

    // Calculations
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp);
    SimulationStats getWindowStats(timestamp_t timestamp);

    MemSpecLPDDR5 memSpec;
    std::vector<Rank> ranks;
    util::Bus commandBus;
    util::Bus readBus;
    util::Bus writeBus;

   private:
    template <dram_base::commandEnum_t Cmd, typename Func>
    void registerBankHandler(Func&& member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command& command) {
            auto& rank = this->ranks[command.targetCoordinate.rank];
            auto& bank = rank.banks[command.targetCoordinate.bank];
            rank.commandCounter.inc(command.type);
            (this->*member_func)(rank, bank, command.timestamp);
        });
    };

    template <dram_base::commandEnum_t Cmd, typename Func>
    void registerBankGroupHandler(Func&& member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command& command) {
            auto& rank = this->ranks[command.targetCoordinate.rank];
            auto& bank = rank.banks[command.targetCoordinate.bank];
            rank.commandCounter.inc(command.type);
            auto bank_id = command.targetCoordinate.bank;
            (this->*member_func)(rank, bank_id, command.timestamp);
        });
    }

    template <dram_base::commandEnum_t Cmd, typename Func>
    void registerRankHandler(Func&& member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command& command) {
            auto& rank = this->ranks[command.targetCoordinate.rank];

            rank.commandCounter.inc(command.type);
            (this->*member_func)(rank, command.timestamp);
        });
    };

    template <dram_base::commandEnum_t Cmd, typename Func>
    void registerHandler(Func&& member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command& command) {
            (this->*member_func)(command.timestamp);
        });
    };

    void registerPatterns();
    timestamp_t earliestPossiblePowerDownEntryTime(Rank& rank);

    util::Clock clock;
    util::Clock wck;
    util::Clock readDQS;
};
};  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR5_LPDDR5_H */
