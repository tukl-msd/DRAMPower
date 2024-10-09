#ifndef DRAMPOWER_STANDARDS_DDR5_DDR5_H
#define DRAMPOWER_STANDARDS_DDR5_DDR5_H

#include <cstdint>
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
#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/cycle_stats.h"

namespace DRAMPower {

class DDR5 : public dram_base<CmdType> {

public:
    using commandbus_t = util::Bus<14>;
    using databus_8_t = util::Bus<8>;
    using databus_16_t = util::Bus<16>;
    MemSpecDDR5 memSpec;
    std::vector<Rank> ranks;
    std::variant<util::DatabusContainer<4>, util::DatabusContainer<8>, util::DatabusContainer<16>> databus;
private:
    std::size_t cmdBusWidth;
    uint64_t cmdBusInitPattern;
public:
    commandbus_t commandBus;
private:
    util::Clock readDQS;
    util::Clock writeDQS;
    util::Clock clock;
    TogglingHandle togglingHandleRead;
    TogglingHandle togglingHandleWrite;

public:
    DDR5(const MemSpecDDR5& memSpec);
    virtual ~DDR5() = default;

    timestamp_t earliestPossiblePowerDownEntryTime(Rank& rank);

    SimulationStats getStats() override;
    uint64_t getBankCount() override;
    uint64_t getRankCount() override;
    uint64_t getDeviceCount() override;


private:
    template <size_t N>
    void handle_interface_impl(const Command &cmd, util::DatabusContainer<N> &databus) {
        // databus shadows variant databus
        size_t length = 0;
        if (cmd.type == CmdType::RD || cmd.type == CmdType::RDA) {
            length = cmd.sz_bits / databus.readBus_vec[0].get_width();
            if ( cmd.data != nullptr ) {
                for (auto &readBus : databus.readBus_vec) {
                    readBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);
                }
            }
            handle_interface_data_common(cmd, length);
        } else if (cmd.type == CmdType::WR || cmd.type == CmdType::WRA) {
            length = cmd.sz_bits / databus.writeBus_vec[0].get_width();
            if ( cmd.data != nullptr ) {
                for (auto &writeBus : databus.writeBus_vec) {
                    writeBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);
                }
            }
            handle_interface_data_common(cmd, length);
        }
        handle_interface_commandbus(cmd);
    }
    void handle_interface(const Command& cmd) override;
    void handle_interface_toggleRate(const Command& cmd) override;
    void update_toggling_rate(const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) override;
    void handleInterfaceOverrides(size_t length, bool read);
    uint64_t getInitEncoderPattern() override;
public:
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;

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

    SimulationStats getWindowStats(timestamp_t timestamp);

private:
    void handle_interface_commandbus(const Command& cmd);
    void handle_interface_data_common(const Command &cmd, const size_t length);

protected:
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

    void registerPatterns();
};

}  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR5_DDR5_H */
