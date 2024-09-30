#ifndef DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H
#define DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H

#include <DRAMPower/util/bus.h>
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
    using commandbus_t = util::Bus<6>;
    using databus_8_t = util::Bus<8>;
    using databus_16_t = util::Bus<16>;
    MemSpecLPDDR4 memSpec;
    std::vector<Rank> ranks;
    commandbus_t commandBus;
    std::vector<databus_8_t> readBus_8_vec;
    std::vector<databus_8_t> writeBus_8_vec;
    std::vector<databus_16_t> readBus_16_vec;
    std::vector<databus_16_t> writeBus_16_vec;
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
    template <size_t N>
    void handle_interface_impl(
        const Command &cmd,
        std::vector<util::Bus<N>> &writeBus_vec,
        std::vector<util::Bus<N>> &readBus_vec
    ) {
        size_t length = 0;
        if (cmd.type == CmdType::RD || cmd.type == CmdType::RDA) {
            length = cmd.sz_bits / readBus_vec[0].get_width();
            if ( cmd.data != nullptr ) {
                for (auto &readBus : readBus_vec) {
                    readBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);
                }
            }
            handle_interface_data_common(cmd, length);
        } else if (cmd.type == CmdType::WR || cmd.type == CmdType::WRA) {
            length = cmd.sz_bits / writeBus_vec[0].get_width();
            if ( cmd.data != nullptr ) {
                for (auto &writeBus : writeBus_vec) {
                    writeBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);
                }
            }
            handle_interface_data_common(cmd, length);
        }
        handle_interface_commandbus(cmd);
    }

    template <class bus_t>
    void toggling_rate_enable(timestamp_t timestamp, timestamp_t enable_timestamp, std::vector<bus_t> &bus, DRAMPower::TogglingHandle &togglinghandle) {
        // Change from bus to toggling rate
        assert(enable_timestamp >= timestamp);
        if ( enable_timestamp > timestamp ) {
            // Schedule toggling rate enable
            this->addImplicitCommand(enable_timestamp, [this, &togglinghandle, &bus, enable_timestamp]() {
                for (auto &b : bus) {
                    b.disable(enable_timestamp);
                    togglinghandle.enable(enable_timestamp);
                }
            });
        } else {
            for (auto &b : bus) {
                b.disable(enable_timestamp);
                togglinghandle.enable(enable_timestamp);
            }
        }
    }

    template <class bus_t>
    void toggling_rate_disable(timestamp_t timestamp, timestamp_t disable_timestamp, std::vector<bus_t> &bus, DRAMPower::TogglingHandle &togglinghandle) {
        // Change from toggling rate to bus
        assert(disable_timestamp >= timestamp);
        if ( disable_timestamp > timestamp ) {
            // Schedule toggling rate disable
            this->addImplicitCommand(disable_timestamp, [this, &togglinghandle, &bus, disable_timestamp]() {
                for (auto &b : bus) {
                    b.enable(disable_timestamp);
                    togglinghandle.disable(disable_timestamp);
                }
            });
        } else {
            for (auto &b : bus) {
                b.enable(disable_timestamp);
                togglinghandle.disable(disable_timestamp);
            }
        }
    }

    void handle_interface(const Command& cmd) override;
    void handle_interface_toggleRate(const Command& cmd) override;
    timestamp_t toggling_rate_get_enable_time(timestamp_t timestamp);
    timestamp_t toggling_rate_get_disable_time(timestamp_t timestamp);
    timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) override;
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
