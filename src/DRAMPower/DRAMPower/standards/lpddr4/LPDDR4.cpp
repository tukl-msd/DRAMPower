#include "LPDDR4.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/lpddr4/core_calculation_LPDDR4.h>
#include <DRAMPower/standards/lpddr4/interface_calculation_LPDDR4.h>


namespace DRAMPower {

    using namespace DRAMUtils::Config;

    LPDDR4::LPDDR4(const MemSpecLPDDR4 &memSpec)
        : dram_base<CmdType>(PatternEncoderOverrides{
            {pattern_descriptor::C0, PatternEncoderBitSpec::L},
            {pattern_descriptor::C1, PatternEncoderBitSpec::L},
          }) 
        , memSpec(memSpec)
        , ranks(memSpec.numberOfRanks, {(std::size_t)memSpec.numberOfBanks})
        , commandBus{6, 1, util::Bus::BusIdlePatternSpec::L, util::Bus::BusInitPatternSpec::L}
        , readBus{memSpec.bitWidth * memSpec.numberOfDevices, memSpec.dataRate,
            util::Bus::BusIdlePatternSpec::L, util::Bus::BusInitPatternSpec::L
        }
        , writeBus{memSpec.bitWidth * memSpec.numberOfDevices, memSpec.dataRate,
            util::Bus::BusIdlePatternSpec::L, util::Bus::BusInitPatternSpec::L
        }
        , readDQS(memSpec.dataRate, true)
        , writeDQS(memSpec.dataRate, true)
    {
        togglingHandleRead.setWidth(memSpec.bitWidth * memSpec.numberOfDevices);
        togglingHandleWrite.setWidth(memSpec.bitWidth * memSpec.numberOfDevices);
        togglingHandleRead.setDataRate(memSpec.dataRate);
        togglingHandleWrite.setDataRate(memSpec.dataRate);
        this->registerPatterns();

        this->registerBankHandler<CmdType::ACT>(&LPDDR4::handleAct);
        this->registerBankHandler<CmdType::PRE>(&LPDDR4::handlePre);
        this->registerRankHandler<CmdType::PREA>(&LPDDR4::handlePreAll);
        this->registerBankHandler<CmdType::REFB>(&LPDDR4::handleRefPerBank);
        this->registerBankHandler<CmdType::RD>(&LPDDR4::handleRead);
        this->registerBankHandler<CmdType::RDA>(&LPDDR4::handleReadAuto);
        this->registerBankHandler<CmdType::WR>(&LPDDR4::handleWrite);
        this->registerBankHandler<CmdType::WRA>(&LPDDR4::handleWriteAuto);

        this->registerRankHandler<CmdType::REFA>(&LPDDR4::handleRefAll);
        this->registerRankHandler<CmdType::PDEA>(&LPDDR4::handlePowerDownActEntry);
        this->registerRankHandler<CmdType::PDXA>(&LPDDR4::handlePowerDownActExit);
        this->registerRankHandler<CmdType::PDEP>(&LPDDR4::handlePowerDownPreEntry);
        this->registerRankHandler<CmdType::PDXP>(&LPDDR4::handlePowerDownPreExit);
        this->registerRankHandler<CmdType::SREFEN>(&LPDDR4::handleSelfRefreshEntry);
        this->registerRankHandler<CmdType::SREFEX>(&LPDDR4::handleSelfRefreshExit);


        routeCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
    };

    void LPDDR4::toggling_rate_enable(timestamp_t timestamp, timestamp_t enable_timestamp, DRAMPower::util::Bus &bus, DRAMPower::TogglingHandle &togglinghandle) {
        // Change from bus to toggling rate
        assert(enable_timestamp >= timestamp);
        if ( enable_timestamp > timestamp ) {
            // Schedule toggling rate enable
            this->addImplicitCommand(enable_timestamp, [this, &togglinghandle, &bus, enable_timestamp]() {
                bus.disable(enable_timestamp);
                togglinghandle.enable(enable_timestamp);
            });
        } else {
            bus.disable(enable_timestamp);
            togglinghandle.enable(enable_timestamp);
        }
    }

    void LPDDR4::toggling_rate_disable(timestamp_t timestamp, timestamp_t disable_timestamp, DRAMPower::util::Bus &bus, DRAMPower::TogglingHandle &togglinghandle) {
        // Change from toggling rate to bus
        assert(disable_timestamp >= timestamp);
        if ( disable_timestamp > timestamp ) {
            // Schedule toggling rate disable
            this->addImplicitCommand(disable_timestamp, [this, &togglinghandle, &bus, disable_timestamp]() {
                bus.enable(disable_timestamp);
                togglinghandle.disable(disable_timestamp);
            });
        } else {
            bus.enable(disable_timestamp);
            togglinghandle.disable(disable_timestamp);
        }
    }

    timestamp_t LPDDR4::toggling_rate_get_enable_time(timestamp_t timestamp) {
        timestamp_t busdisabletimestamp = timestamp;
        busdisabletimestamp = std::max(this->readBus.get_lastburst_timestamp(), busdisabletimestamp);
        busdisabletimestamp = std::max(this->writeBus.get_lastburst_timestamp(), busdisabletimestamp);
        return busdisabletimestamp;
    }
    timestamp_t LPDDR4::toggling_rate_get_disable_time(timestamp_t timestamp) {
        timestamp_t busenabletimestamp = timestamp;
        busenabletimestamp = std::max(this->togglingHandleRead.get_lastburst_timestamp(), busenabletimestamp);
        busenabletimestamp = std::max(this->togglingHandleWrite.get_lastburst_timestamp(), busenabletimestamp);
        return busenabletimestamp;
    }

    timestamp_t LPDDR4::update_toggling_rate(timestamp_t timestamp, const std::optional<ToggleRateDefinition> &toggleratedefinition)
    {
        if (toggleratedefinition) {
            // Update toggling rate
            togglingHandleRead.setTogglingRateAndDutyCycle(
                toggleratedefinition->togglingRateRead,
                toggleratedefinition->dutyCycleRead,
                toggleratedefinition->idlePatternRead
            );
            togglingHandleWrite.setTogglingRateAndDutyCycle(
                toggleratedefinition->togglingRateWrite,
                toggleratedefinition->dutyCycleWrite,
                toggleratedefinition->idlePatternWrite
            );
            // toggling rate already enabled
            if (togglingHandleRead.isEnabled() && togglingHandleWrite.isEnabled()) {
                return timestamp;
            }
            // Enable toggling rate
            timestamp_t enable_timestamp = toggling_rate_get_enable_time(timestamp);
            toggling_rate_enable(timestamp, enable_timestamp, readBus, togglingHandleRead);
            toggling_rate_enable(timestamp, enable_timestamp, writeBus, togglingHandleWrite);
            return enable_timestamp;
        } else {
            // Toggling rate already disabled
            if (!togglingHandleRead.isEnabled() && !togglingHandleWrite.isEnabled()) {
                return timestamp;
            }
            // Disable toggling rate
            timestamp_t disable_timestamp = toggling_rate_get_disable_time(timestamp);
            toggling_rate_disable(timestamp, disable_timestamp, readBus, togglingHandleRead);
            toggling_rate_disable(timestamp, disable_timestamp, writeBus, togglingHandleWrite);
            return disable_timestamp;
        }
        return timestamp;
    }

    uint64_t LPDDR4::getBankCount() {
        return memSpec.numberOfBanks;
    }

    uint64_t LPDDR4::getRankCount() {
        return memSpec.numberOfRanks;
    }

    uint64_t LPDDR4::getDeviceCount() {
        return memSpec.numberOfDevices;
    }

    void LPDDR4::registerPatterns() {
        using namespace pattern_descriptor;

        // LPDDR4
        // ---------------------------------:
        this->registerPattern<CmdType::ACT>({
                                                    H, L, R12, R13, R14, R15,
                                                    BA0, BA1, BA2, R16, R10, R11,
                                                    R17, R18, R6, R7, R8, R9,
                                                    R0, R1, R2, R3, R4, R5,
                                            });
        this->registerPattern<CmdType::PRE>({
                                                    L, L, L, L, H, L,
                                                    BA0, BA1, BA2, V, V, V,
                                            });
        this->registerPattern<CmdType::PREA>({
                                                     L, L, L, L, H, H,
                                                     V, V, V, V, V, V,
                                             });
        this->registerPattern<CmdType::REFB>({
                                                     L, L, L, H, L, L,
                                                     BA0, BA1, BA2, V, V, V,
                                             });
        this->registerPattern<CmdType::REFA>({
                                                     L, L, L, H, L, H,
                                                     V, V, V, V, V, V,
                                             });
        this->registerPattern<CmdType::SREFEN>({
                                                       L, L, L, H, H, V,
                                                       V, V, V, V, V, V,
                                               });
        this->registerPattern<CmdType::SREFEX>({
                                                       L, L, H, L, H, V,
                                                       V, V, V, V, V, V,
                                               });
        this->registerPattern<CmdType::WR>({
                                                   L, L, H, L, L, BL,
                                                   BA0, BA1, BA2, V, C9, L,
                                                   L, H, L, L, H, C8,
                                                   C2, C3, C4, C5, C6, C7,
                                           });
        this->registerPattern<CmdType::RD>({
                                                   L, H, L, L, L, BL,
                                                   BA0, BA1, BA2, V, C9, L,
                                                   L, H, L, L, H, C8,
                                                   C2, C3, C4, C5, C6, C7
                                           });
        this->registerPattern<CmdType::WRA>({
                                                   L, L, H, L, L, BL,
                                                   BA0, BA1, BA2, V, C9, H,
                                                   L, H, L, L, H, C8,
                                                   C2, C3, C4, C5, C6, C7,
                                           });
        this->registerPattern<CmdType::RDA>({
                                                   L, H, L, L, L, BL,
                                                   BA0, BA1, BA2, V, C9, H,
                                                   L, H, L, L, H, C8,
                                                   C2, C3, C4, C5, C6, C7
                                           });
    };

    void LPDDR4::handleInterfaceOverrides(size_t length, bool /*read*/)
    {
        // Set command bus pattern overrides
        switch(length) {
            case 32:
                this->encoder.settings.updateSettings({
                    {pattern_descriptor::C4, PatternEncoderBitSpec::L},
                    {pattern_descriptor::C3, PatternEncoderBitSpec::L},
                    {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                    {pattern_descriptor::BL, PatternEncoderBitSpec::H},
                });
                break;
            default:
                // Pull down
                // No interface power needed for PatternEncoderBitSpec::L
                // Defaults to burst length 16
            case 16:
                this->encoder.settings.removeSetting(pattern_descriptor::C4);
                this->encoder.settings.updateSettings({
                    {pattern_descriptor::C3, PatternEncoderBitSpec::L},
                    {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                    {pattern_descriptor::BL, PatternEncoderBitSpec::L},
                });
                break;
        }
    }

    void LPDDR4::handle_interface_commandbus(const Command &cmd) {
        auto pattern = this->getCommandPattern(cmd);
        auto ca_length = this->getPattern(cmd.type).size() / commandBus.get_width();
        this->commandBus.load(cmd.timestamp, pattern, ca_length);
    }

    void LPDDR4::handle_interface_data_common(const Command &cmd, const size_t length) {
        if (cmd.type == CmdType::RD || cmd.type == CmdType::RDA) {
            readDQS.start(cmd.timestamp);
            readDQS.stop(cmd.timestamp + length / memSpec.dataRate);
            handleInterfaceOverrides(length, true);
        } else if (cmd.type == CmdType::WR || cmd.type == CmdType::WRA) {
            writeDQS.start(cmd.timestamp);
            writeDQS.stop(cmd.timestamp + length / memSpec.dataRate);
            handleInterfaceOverrides(length, false);
        }
    }

    void LPDDR4::handle_interface_toggleRate(const Command& cmd) {
        if (cmd.type == CmdType::RD || cmd.type == CmdType::RDA) {
            if (cmd.sz_bits == 0) {
                // Use default burst length
                this->togglingHandleRead.incCountBurstLength(cmd.timestamp, memSpec.burstLength);
            } else {
                this->togglingHandleRead.incCountBitLength(cmd.timestamp, cmd.sz_bits);
            }
            assert(cmd.sz_bits % togglingHandleRead.getWidth() == 0);
            handle_interface_data_common(cmd, cmd.sz_bits / togglingHandleRead.getWidth());
        } else if (cmd.type == CmdType::WR || cmd.type == CmdType::WRA) {
            if (cmd.sz_bits == 0) {
                // Use default burst length
                this->togglingHandleWrite.incCountBurstLength(cmd.timestamp, memSpec.burstLength);
            } else {
                this->togglingHandleWrite.incCountBitLength(cmd.timestamp, cmd.sz_bits);
            }
            assert(cmd.sz_bits % togglingHandleWrite.getWidth() == 0);
            handle_interface_data_common(cmd, cmd.sz_bits / togglingHandleWrite.getWidth());
        }
        handle_interface_commandbus(cmd);
    }

    void LPDDR4::handle_interface(const Command &cmd) {
        size_t length = 0;
        if (cmd.type == CmdType::RD || cmd.type == CmdType::RDA) {
            length = cmd.sz_bits / readBus.get_width();
            if ( cmd.data != nullptr ) {
                readBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);
            }
            handle_interface_data_common(cmd, length);
        } else if (cmd.type == CmdType::WR || cmd.type == CmdType::WRA) {
            length = cmd.sz_bits / writeBus.get_width();
            if ( cmd.data != nullptr ) {
                writeBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);
            }
            handle_interface_data_common(cmd, length);
        }
        handle_interface_commandbus(cmd);
    }

    void LPDDR4::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
        bank.counter.act++;

        bank.cycles.act.start_interval(timestamp);

        if (!rank.isActive(timestamp)) {
            rank.cycles.act.start_interval(timestamp);
        }

        bank.bankState = Bank::BankState::BANK_ACTIVE;
    }

    void LPDDR4::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
        if (bank.bankState == Bank::BankState::BANK_PRECHARGED)
            return;

        bank.counter.pre++;
        bank.cycles.act.close_interval(timestamp);
        bank.latestPre = timestamp;
        bank.bankState = Bank::BankState::BANK_PRECHARGED;

        if (!rank.isActive(timestamp)) {
            rank.cycles.act.close_interval(timestamp);
        }
    }

    void LPDDR4::handlePreAll(Rank &rank, timestamp_t timestamp) {
        for (auto &bank: rank.banks) {
            handlePre(rank, bank, timestamp);
        }
    }

    void
    LPDDR4::handleRefreshOnBank(Rank &rank, Bank &bank, timestamp_t timestamp, uint64_t timing, uint64_t &counter) {
        ++counter;

        if (!rank.isActive(timestamp)) {
            rank.cycles.act.start_interval(timestamp);
        }

        bank.bankState = Bank::BankState::BANK_ACTIVE;
        auto timestamp_end = timestamp + timing;
        bank.refreshEndTime = timestamp_end;
        if (!bank.cycles.act.is_open())
            bank.cycles.act.start_interval(timestamp);

        // Execute implicit pre-charge at refresh end
        addImplicitCommand(timestamp_end, [this, &bank, &rank, timestamp_end]() {
            bank.bankState = Bank::BankState::BANK_PRECHARGED;
            bank.cycles.act.close_interval(timestamp_end);

            if (!rank.isActive(timestamp_end)) {
                rank.cycles.act.close_interval(timestamp_end);
            }
        });
    }

    void LPDDR4::handleRefAll(Rank &rank, timestamp_t timestamp) {
        for (auto &bank: rank.banks) {
            handleRefreshOnBank(rank, bank, timestamp, memSpec.memTimingSpec.tRFC, bank.counter.refAllBank);
        }
        rank.endRefreshTime = timestamp + memSpec.memTimingSpec.tRFC;
    }

    void LPDDR4::handleRefPerBank(Rank &rank, Bank &bank, timestamp_t timestamp) {
        handleRefreshOnBank(rank, bank, timestamp, memSpec.memTimingSpec.tRFCPB, bank.counter.refPerBank);
    }

    void LPDDR4::handleSelfRefreshEntry(Rank &rank, timestamp_t timestamp) {
        // Issue implicit refresh
        handleRefAll(rank, timestamp);
        // Handle self-refresh entry after tRFC
        auto timestampSelfRefreshStart = timestamp + memSpec.memTimingSpec.tRFC;
        addImplicitCommand(timestampSelfRefreshStart, [this, &rank, timestampSelfRefreshStart]() {
            rank.counter.selfRefresh++;
            rank.cycles.sref.start_interval(timestampSelfRefreshStart);
            rank.memState = MemState::SREF;
        });
    }

    void LPDDR4::handleSelfRefreshExit(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::SREF);
        rank.cycles.sref.close_interval(timestamp);  // Duration start between entry and exit
        rank.memState = MemState::NOT_IN_PD;
    }

    void LPDDR4::handlePowerDownActEntry(Rank &rank, timestamp_t timestamp) {
        auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
        auto entryTime = std::max(timestamp, earliestPossibleEntry);
        addImplicitCommand(entryTime, [this, &rank, entryTime]() {
            rank.cycles.powerDownAct.start_interval(entryTime);
            rank.memState = MemState::PDN_ACT;
            if (rank.cycles.act.is_open()) {
                rank.cycles.act.close_interval(entryTime);
            }
            for (auto &bank: rank.banks) {
                if (bank.cycles.act.is_open()) {
                    bank.cycles.act.close_interval(entryTime);
                }
            }
        });
    }

    void LPDDR4::handlePowerDownActExit(Rank &rank, timestamp_t timestamp) {
        // TODO: Is this computation necessary?
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [this, &rank, exitTime]() {
            rank.memState = MemState::NOT_IN_PD;
            rank.cycles.powerDownAct.close_interval(exitTime);

            bool rank_active = false;

            for (auto & bank : rank.banks) {
                if (bank.counter.act != 0 && bank.cycles.act.get_end() == rank.cycles.powerDownAct.get_start()) {
                    rank_active = true;
                    bank.cycles.act.start_interval(exitTime);
                }
            }

            if (rank_active) {
                rank.cycles.act.start_interval(exitTime);
            }

        });
    };

    void LPDDR4::handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp) {
        auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
        auto entryTime = std::max(timestamp, earliestPossibleEntry);
        addImplicitCommand(entryTime, [this, &rank, entryTime]() {
            rank.cycles.powerDownPre.start_interval(entryTime);
            rank.memState = MemState::PDN_PRE;
        });
    }

    void LPDDR4::handlePowerDownPreExit(Rank &rank, timestamp_t timestamp) {
        // TODO: Is this computation necessary?
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [this, &rank, exitTime]() {
            rank.memState = MemState::NOT_IN_PD;
            rank.cycles.powerDownPre.close_interval(exitTime);
        });
    }

    void LPDDR4::handleRead(Rank&, Bank &bank, timestamp_t) {
        ++bank.counter.reads;
    }

    void LPDDR4::handleWrite(Rank&, Bank &bank, timestamp_t) {
        ++bank.counter.writes;
    }

    void LPDDR4::handleReadAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.readAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
        auto minReadActiveTime = timestamp + this->memSpec.prechargeOffsetRD;

        auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void LPDDR4::handleWriteAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.writeAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
        auto minWriteActiveTime = timestamp + this->memSpec.prechargeOffsetWR;

        auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void LPDDR4::endOfSimulation(timestamp_t) {
        if (this->implicitCommandCount() > 0)
			std::cout << ("[WARN] End of simulation but still implicit commands left!") << std::endl;
	}

    energy_t LPDDR4::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_LPDDR4 calculation;

        return calculation.calcEnergy(timestamp, *this);
    }

    interface_energy_info_t LPDDR4::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_LPDDR4 interface_calc(memSpec);
        return interface_calc.calculateEnergy(getWindowStats(timestamp));
    }

    SimulationStats LPDDR4::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        processImplicitCommandQueue(timestamp);

        SimulationStats stats;
        try {
        stats.bank.resize(memSpec.numberOfBanks * memSpec.numberOfRanks);
        stats.rank_total.resize(memSpec.numberOfRanks);
        }
        catch (const std::bad_alloc& e)
        {
            std::cout << "Memory allocation failed " << e.what() << '\n';
        }

        auto simulation_duration = timestamp;
        for (size_t i = 0; i < memSpec.numberOfRanks; ++i) {
            Rank &rank = ranks[i];
            size_t bank_offset = i * memSpec.numberOfBanks;

            for (std::size_t j = 0; j < memSpec.numberOfBanks; ++j) {
                stats.bank[bank_offset + j].counter = rank.banks[j].counter;
                stats.bank[bank_offset + j].cycles.act =
                    rank.banks[j].cycles.act.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.ref =
                    rank.banks[j].cycles.ref.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.selfRefresh =
                    rank.cycles.sref.get_count_at(timestamp) -
                    rank.cycles.deepSleepMode.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.deepSleepMode =
                    rank.cycles.deepSleepMode.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.powerDownAct =
                    rank.cycles.powerDownAct.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.powerDownPre =
                    rank.cycles.powerDownPre.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.pre =
                    simulation_duration - (stats.bank[bank_offset + j].cycles.act +
                                           rank.cycles.powerDownAct.get_count_at(timestamp) +
                                           rank.cycles.powerDownPre.get_count_at(timestamp) +
                                           rank.cycles.sref.get_count_at(timestamp));
            }

            stats.rank_total[i].cycles.pre =
                simulation_duration - (rank.cycles.act.get_count_at(timestamp) +
                                       rank.cycles.powerDownAct.get_count_at(timestamp) +
                                       rank.cycles.powerDownPre.get_count_at(timestamp) +
                                       rank.cycles.sref.get_count_at(timestamp));

            stats.rank_total[i].cycles.act = rank.cycles.act.get_count_at(timestamp);
            stats.rank_total[i].cycles.ref = rank.cycles.act.get_count_at(
                timestamp);  // TODO: I think this counter is never updated
            stats.rank_total[i].cycles.powerDownAct =
                rank.cycles.powerDownAct.get_count_at(timestamp);
            stats.rank_total[i].cycles.powerDownPre =
                rank.cycles.powerDownPre.get_count_at(timestamp);
            stats.rank_total[i].cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp);
        }

        stats.commandBus = commandBus.get_stats(timestamp);
        stats.readBus = readBus.get_stats(timestamp);
        stats.writeBus = writeBus.get_stats(timestamp);
        stats.togglingStats = {
            togglingHandleRead.get_stats(timestamp), // read
            togglingHandleWrite.get_stats(timestamp) // write
        };

        stats.clockStats = 2 * clock.get_stats_at(timestamp);
        stats.readDQSStats = 2 * readDQS.get_stats_at(timestamp);
        stats.writeDQSStats = 2 * writeDQS.get_stats_at(timestamp);

        return stats;
    }

    SimulationStats LPDDR4::getStats() {
        return getWindowStats(getLastCommandTime());
    }

}
