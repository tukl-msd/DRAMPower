#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4_H

#include <DRAMPower/util/bus.h>
#include <DRAMPower/util/clock.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/memspec/MemSpecDDR4.h>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/util/cycle_stats.h>

#include <deque>
#include <algorithm>
#include <stdint.h>
#include <vector>

namespace DRAMPower {

    class DDR4 : public dram_base<CmdType>{
    public:
        DDR4(const MemSpecDDR4 &memSpec);

        virtual ~DDR4() = default;

    public:
        MemSpecDDR4 memSpec;
        std::vector<Rank> ranks;

        util::Clock clock;
        util::Clock clockInverted;

        util::Bus commandBus;
        util::Bus readBus;
        util::Bus writeBus;

        util::Clock readDQS_c;
        util::Clock readDQS_t;

        util::Clock writeDQS_c;
        util::Clock writeDQS_t;
    protected:
        template<dram_base::commandEnum_t Cmd, typename Func>
        void registerBankHandler(Func &&member_func) {
            this->routeCommand<Cmd>([this, member_func](const Command &command) {
                auto &rank = this->ranks[command.targetCoordinate.rank];
                auto &bank = rank.banks[command.targetCoordinate.bank];

                rank.commandCounter.inc(command.type);
                (this->*member_func)(rank, bank, command.timestamp);
            });
        };

        template<dram_base::commandEnum_t Cmd, typename Func>
        void registerRankHandler(Func &&member_func) {
            this->routeCommand<Cmd>([this, member_func](const Command &command) {
                auto &rank = this->ranks[command.targetCoordinate.rank];

                rank.commandCounter.inc(command.type);
                (this->*member_func)(rank, command.timestamp);
            });
        };

        template<dram_base::commandEnum_t Cmd, typename Func>
        void registerHandler(Func &&member_func) {
            this->routeCommand<Cmd>([this, member_func](const Command &command) {
                (this->*member_func)(command.timestamp);
            });
        };

        void registerPatterns();

        uint64_t encode(const Command &cmd, const std::vector<pattern_descriptor::t> &pattern) const override;
    public:
        timestamp_t earliestPossiblePowerDownEntryTime(Rank &rank) {
            timestamp_t entryTime = 0;

            for (const auto &bank: rank.banks) {
                entryTime = std::max({entryTime,
                                      bank.counter.act == 0 ? 0 : bank.cycles.act.get_start() +
                                                                  memSpec.memTimingSpec.tRCD,
                                      bank.counter.pre == 0 ? 0 : bank.latestPre + memSpec.memTimingSpec.tRP,
                                      bank.refreshEndTime
                                     });
            }

            return entryTime;
        };
    public:
        void handle_interface(const Command &cmd) override;

        void handleAct(Rank &rank, Bank &bank, timestamp_t timestamp);

        void handlePre(Rank &rank, Bank &bank, timestamp_t timestamp);

        void handlePreAll(Rank &rank, timestamp_t timestamp);

        void handleRefAll(Rank &rank, timestamp_t timestamp);

        void handleSelfRefreshEntry(Rank &rank, timestamp_t timestamp);

        void handleSelfRefreshExit(Rank &rank, timestamp_t timestamp);

        void handleRead(Rank &rank, Bank &bank, timestamp_t timestamp);

        void handleWrite(Rank &rank, Bank &bank, timestamp_t timestamp);

        void handleReadAuto(Rank &rank, Bank &bank, timestamp_t timestamp);

        void handleWriteAuto(Rank &rank, Bank &bank, timestamp_t timestamp);

        void handlePowerDownActEntry(Rank &rank, timestamp_t timestamp);

        void handlePowerDownActExit(Rank &rank, timestamp_t timestamp);

        void handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp);

        void handlePowerDownPreExit(Rank &rank, timestamp_t timestamp);

        void endOfSimulation(timestamp_t timestamp);

    public:
        energy_t calcEnergy(timestamp_t timestamp);

        interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp);

    public:
        SimulationStats getWindowStats(timestamp_t timestamp);

        SimulationStats getStats();
    };

};

#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4_H */
