#ifndef DRAMPOWER_STANDARDS_DDR5_DDR5_H
#define DRAMPOWER_STANDARDS_DDR5_DDR5_H

#include <cstdint>
#include <deque>
#include <vector>

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/data/energy.h"
#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/dram/dram_base.h"
#include "DRAMPower/memspec/MemSpec.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/cycle_stats.h"

namespace DRAMPower {

class DDR5 : public dram_base<CmdType> {
   public:
    MemSpecDDR5 memSpec;
    std::vector<Rank> ranks;
    util::Bus commandBus;
    util::Bus readBus;
    util::Bus writeBus;

    DDR5(const MemSpecDDR5& memSpec);
    virtual ~DDR5() = default;

    timestamp_t earliestPossiblePowerDownEntryTime(Rank& rank);

    void handle_interface(const Command& cmd) override;

    // Commands
    void handleAct(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handlePre(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handlePreSameBank(Rank& rank, std::size_t bank_id, timestamp_t timestamp);
    void handlePreAll(Rank& rank, timestamp_t timestamp);
    void handleRefSameBank(Rank& rank, std::size_t bank_id, timestamp_t timestamp);
    void handleRefAll(Rank& rank, timestamp_t timestamp);
    void handleRefreshOnBank(Rank& rank, Bank& bank, timestamp_t timestamp, uint64_t timing,
                             uint64_t& counter);
    void handleSelfRefreshEntry(Rank& rank, timestamp_t timestamp);
    void handleSelfRefreshExit(Rank& rank, timestamp_t timestamp);
    void handleRead(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleWrite(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleReadAuto(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleWriteAuto(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handlePowerDownActEntry(Rank& rank, timestamp_t timestamp);
    void handlePowerDownActExit(Rank& rank, timestamp_t timestamp);
    void handlePowerDownPreEntry(Rank& rank, timestamp_t timestamp);
    void handlePowerDownPreExit(Rank& rank, timestamp_t timestamp);
    void endOfSimulation(timestamp_t timestamp);

    // Calculations
    energy_t calcEnergy(timestamp_t timestamp);
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp);

    SimulationStats getWindowStats(timestamp_t timestamp);
    SimulationStats getStats();

   protected:
    template <dram_base::commandEnum_t Cmd, typename Func>
    void registerBankHandler(Func&& member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command& command) {
            auto& rank = this->ranks[command.targetCoordinate.rank];
            auto& bank = rank.banks[command.targetCoordinate.bank];
            rank.commandCounter.inc(command.type);
            (this->*member_func)(rank, bank, command.timestamp);
        });
    }

    template <dram_base::commandEnum_t Cmd, typename Func>
    void registerBankGroupHandler(Func&& member_func) {
        this->routeCommand<Cmd>([this, member_func](const Command& command) {
            auto& rank = this->ranks[command.targetCoordinate.rank];
            auto bank_id = command.targetCoordinate.bank;
            rank.commandCounter.inc(command.type);
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

   private:
    util::Clock CK_t_;
    util::Clock CK_c_;
    util::Clock readDQS_c_;
    util::Clock readDQS_t_;
    util::Clock writeDQS_c_;
    util::Clock writeDQS_t_;
};

}  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR5_DDR5_H */
