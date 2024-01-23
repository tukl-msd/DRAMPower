#include "DDR4.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/ddr4/core_calculation_DDR4.h>
#include <DRAMPower/standards/ddr4/interface_calculation_DDR4.h>
#include <iostream>


namespace DRAMPower {

    DDR4::DDR4(const MemSpecDDR4 &memSpec)
		: memSpec(memSpec)
		, ranks(memSpec.numberOfRanks, {(std::size_t)memSpec.numberOfBanks})
        , readBus(memSpec.bitWidth)
        , writeBus(memSpec.bitWidth)
        , commandBus(27)
        , readDQS_(2, true)
        , writeDQS_(2, true)
	{
        for (auto &rank : ranks) {
            rank.cycles.pre.start_interval(0);
        }
        this->registerPatterns();

        this->registerBankHandler<CmdType::ACT>(&DDR4::handleAct);
        this->registerBankHandler<CmdType::PRE>(&DDR4::handlePre);
        this->registerRankHandler<CmdType::PREA>(&DDR4::handlePreAll);
        this->registerRankHandler<CmdType::REFA>(&DDR4::handleRefAll);
        this->registerBankHandler<CmdType::RD>(&DDR4::handleRead);
        this->registerBankHandler<CmdType::RDA>(&DDR4::handleReadAuto);
        this->registerBankHandler<CmdType::WR>(&DDR4::handleWrite);
        this->registerBankHandler<CmdType::WRA>(&DDR4::handleWriteAuto);
        this->registerRankHandler<CmdType::SREFEN>(&DDR4::handleSelfRefreshEntry);
        this->registerRankHandler<CmdType::SREFEX>(&DDR4::handleSelfRefreshExit);
        this->registerRankHandler<CmdType::PDEA>(&DDR4::handlePowerDownActEntry);
        this->registerRankHandler<CmdType::PDEP>(&DDR4::handlePowerDownPreEntry);
        this->registerRankHandler<CmdType::PDXA>(&DDR4::handlePowerDownActExit);
        this->registerRankHandler<CmdType::PDXP>(&DDR4::handlePowerDownPreExit);

        routeCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
    };

    void DDR4::registerPatterns() {
        using namespace pattern_descriptor;
        // DDR4
        // ---------------------------------:
        this->registerPattern<CmdType::ACT>({
            L, L, A16, A15, A14, BG0, BG1, BA0, BA1,
            V, V, V, A12, A17, A13, A11, A10, A0,
            A1, A2, A3, A4, A5, A6, A7, A8, A9
        });
        this->registerPattern<CmdType::PRE>({
            L, H, L, H, L, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, V, V,
            V, V, V, V, V, V, V, V, V
        });
        this->registerPattern<CmdType::PREA>({
            L, H, L, H, L, V, V, V, V,
            V, V, V, V, V, V, V, H, V,
            V, V, V, V, V, V, V, V, V
        });
        this->registerPattern<CmdType::REFA>({
            L, H, L, L, H, V, V, V, V,
            V, V, V, V, V, V, V, V, V,
            V, V, V, V, V, V, V, V, V
        });
        this->registerPattern<CmdType::RD>({
            L, H, H, L, H, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, L, A0,
            A1, A2, A3, A4, A5, A6, A7, A8, A9
        });
        this->registerPattern<CmdType::RDA>({
            L, H, H, L, H, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, H, A0,
            A1, A2, A3, A4, A5, A6, A7, A8, A9
        });
        this->registerPattern<CmdType::WR>({
            L, H, H, L, L, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, L, A0,
            A1, A2, A3, A4, A5, A6, A7, A8, A9
        });
        this->registerPattern<CmdType::WRA>({
            L, H, H, L, L, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, H, A0,
            A1, A2, A3, A4, A5, A6, A7, A8, A9
        });
        this->registerPattern<CmdType::SREFEN>({
            L, H, L, L, H, V, V, V, V,
            V, V, V, V, V, V, V, V, V,
            V, V, V, V, V, V, V, V, V
        });
        // two cycle command
        this->registerPattern<CmdType::SREFEX>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,

            L, H, H, H, H, V, V, V, V,
            V, V, V, V, V, V, V, V, V,
            V, V, V, V, V, V, V, V, V
        });
        this->registerPattern<CmdType::PDEA>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
        this->registerPattern<CmdType::PDXA>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
        this->registerPattern<CmdType::PDEP>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
        this->registerPattern<CmdType::PDXP>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
    }

    void DDR4::handle_interface(const Command &cmd) {
        // TODO add tests and debug
        // TODO segfault for invalid cmd -> check for invalid commands ???
        if (cmd.type == CmdType::END_OF_SIMULATION) {
            return;
        }
        auto pattern = this->getCommandPattern(cmd);
        // length needed for dual cycle SREFEX
        auto ca_length = getPattern(cmd.type).size() / commandBus.get_width();
        // Segfault for End of Simulation
        this->commandBus.load(cmd.timestamp, pattern, ca_length);

        switch (cmd.type) {
            case CmdType::RD:
            case CmdType::RDA: {
                auto length = cmd.sz_bits / readBus.get_width();
                if (length == 0) {
                    // TODO assert
                    std::cout << "[Error] invalid read length. Interface calculation skipped" << std::endl;
                    return;
                }
                this->readBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);

                readDQS_.start(cmd.timestamp);
                readDQS_.stop(cmd.timestamp + length / this->memSpec.dataRate);
            }
                break;
            case CmdType::WR:
            case CmdType::WRA: {
                auto length = cmd.sz_bits / writeBus.get_width();
                if (length == 0) {
                    // TODO assert
                    std::cout << "[Error] invalid write length. Interface calculation skipped" << std::endl;
                    return;
                }
                this->writeBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);

                writeDQS_.start(cmd.timestamp);
                writeDQS_.stop(cmd.timestamp + length / this->memSpec.dataRate);
            }
                break;
        };
    }

    void DDR4::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
        bank.counter.act++;
        bank.bankState = Bank::BankState::BANK_ACTIVE;

        bank.cycles.act.start_interval(timestamp);
        
        rank.cycles.act.start_interval_if_not_running(timestamp);
        rank.cycles.pre.close_interval(timestamp);
    }

    void DDR4::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
        if (bank.bankState == Bank::BankState::BANK_PRECHARGED) return;             // TODO Isn't then the bank.counter.pre invalid
        bank.counter.pre++;

        bank.bankState = Bank::BankState::BANK_PRECHARGED;
        bank.cycles.act.close_interval(timestamp);
        bank.latestPre = timestamp;                                                 // used for earliest power down calculation
        if ( !rank.isActive(timestamp) )                                            // stop rank active interval if no more banks active
        {
            rank.cycles.act.close_interval(timestamp);
            rank.cycles.pre.start_interval_if_not_running(timestamp);               // TODO active counter increased if at least 1 back active, precharge counter increased if all banks precharged
        }
    }

    void DDR4::handlePreAll(Rank &rank, timestamp_t timestamp) {
        for (auto &bank: rank.banks) 
            handlePre(rank, bank, timestamp);
    }

    void DDR4::handleRefAll(Rank &rank, timestamp_t timestamp) {
        auto timestamp_end = timestamp + memSpec.memTimingSpec.tRFC;
        rank.endRefreshTime = timestamp_end;
        rank.cycles.act.start_interval_if_not_running(timestamp);
        rank.cycles.pre.close_interval(timestamp);
        for (auto& bank : rank.banks) {
            bank.bankState = Bank::BankState::BANK_ACTIVE;
            
            ++bank.counter.refAllBank;                                              // TODO check calculation -> value inserted for all banks
            bank.cycles.act.start_interval_if_not_running(timestamp);


            bank.refreshEndTime = timestamp_end;                                    // used for earliest power down calculation

            // Execute implicit pre-charge at refresh end
            addImplicitCommand(timestamp_end, [this, &bank, &rank, timestamp_end]() {
                bank.bankState = Bank::BankState::BANK_PRECHARGED;
                bank.cycles.act.close_interval(timestamp_end);
                // stop rank active interval if no more banks active
                // TODO if not necessary all banks will be precharged at timestamp_end
                if (!rank.isActive(timestamp_end))                                  // stop rank active interval if no more banks active
                {
                    rank.cycles.act.close_interval(timestamp_end);
                    rank.cycles.pre.start_interval(timestamp_end);
                }
            });
        }

        // Required for precharge power-down
    }

    void DDR4::handleRead(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.reads;
    }

    void DDR4::handleReadAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.readAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
        auto minReadActiveTime = timestamp + this->memSpec.prechargeOffsetRD;

        auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void DDR4::handleWrite(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.writes;
    }

    void DDR4::handleWriteAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.writeAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
        auto minWriteActiveTime =  timestamp + this->memSpec.prechargeOffsetWR;

        auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void DDR4::handleSelfRefreshEntry(Rank &rank, timestamp_t timestamp) {
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

    void DDR4::handleSelfRefreshExit(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::SREF);                                    // check for previous SelfRefreshEntry
        rank.cycles.sref.close_interval(timestamp);                                 // Duration between entry and exit
        rank.memState = MemState::NOT_IN_PD;
    }

    void DDR4::handlePowerDownActEntry(Rank &rank, timestamp_t timestamp) {
        auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
        auto entryTime = std::max(timestamp, earliestPossibleEntry);
        addImplicitCommand(entryTime, [this, &rank, entryTime]() {
            rank.memState = MemState::PDN_ACT;
            rank.cycles.powerDownAct.start_interval(entryTime);
            rank.cycles.act.close_interval(entryTime);
            rank.cycles.pre.close_interval(entryTime);
            for (auto & bank : rank.banks) {
                bank.cycles.act.close_interval(entryTime);
            }
        });
    }

    void DDR4::handlePowerDownActExit(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::PDN_ACT);
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [this, &rank, exitTime]() {
            rank.memState = MemState::NOT_IN_PD;
            rank.cycles.powerDownAct.close_interval(exitTime);

            // Activate banks that were active prior to PDA
            for (auto & bank : rank.banks)
            {
                if (bank.bankState==Bank::BankState::BANK_ACTIVE)
                {
                    bank.cycles.act.start_interval(exitTime);
                }
            }
            // Activate rank if at least one bank is active
            // TODO At least one bank must be active for PDA -> remove if statement?
            if(rank.isActive(exitTime))
                rank.cycles.act.start_interval(exitTime); 

        });
    }

    void DDR4::handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp) {
        auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
        auto entryTime = std::max(timestamp, earliestPossibleEntry);

        addImplicitCommand(entryTime, [this, &rank, entryTime]() {
            for (auto &bank : rank.banks)
                bank.cycles.act.close_interval(entryTime);
            rank.memState = MemState::PDN_PRE;
            rank.cycles.powerDownPre.start_interval(entryTime);
            rank.cycles.pre.close_interval(entryTime);
            rank.cycles.act.close_interval(entryTime);
        });
    }

    void DDR4::handlePowerDownPreExit(Rank &rank, timestamp_t timestamp) {
        // TODO: can the device exit in the same clock cycle as the entry?
        // The computation is necessary to exit at the earliest upon entry
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [this, &rank, exitTime]() {
            rank.memState = MemState::NOT_IN_PD;
            rank.cycles.powerDownPre.close_interval(exitTime);

            // Precharge banks that were precharged prior to PDP
            for (auto & bank : rank.banks)
            {
                if (bank.bankState==Bank::BankState::BANK_ACTIVE)
                {
                    bank.cycles.act.start_interval(exitTime);
                }
            }
            // Precharge rank if all banks are precharged
            // TODO At least one bank must be precharged for PDP -> remove if statement?
            if(!rank.isActive(exitTime))
                rank.cycles.pre.start_interval(exitTime); 
        });
    }

    void DDR4::endOfSimulation(timestamp_t timestamp) {
        assert(this->implicitCommandCount() == 0);
	}

    energy_t DDR4::calcEnergy(timestamp_t timestamp) {
        Calculation_DDR4 calculation;
        return calculation.calcEnergy(timestamp, *this);
    }

    interface_energy_info_t DDR4::calcInterfaceEnergy(timestamp_t timestamp) {
        // TODO add test and debug
        InterfaceCalculation_DDR4 calculation(memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

    SimulationStats DDR4::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        processImplicitCommandQueue(timestamp);

        SimulationStats stats;
        stats.bank.resize(memSpec.numberOfBanks * memSpec.numberOfRanks);
        stats.rank_total.resize(memSpec.numberOfRanks);

        auto simulation_duration = timestamp;
        for (size_t i = 0; i < memSpec.numberOfRanks; ++i) {
            Rank &rank = ranks[i];
            size_t bank_offset = i * memSpec.numberOfBanks;

            for (size_t j = 0; j < memSpec.numberOfBanks; ++j) {
                stats.bank[bank_offset + j].counter = rank.banks[j].counter;
                stats.bank[bank_offset + j].cycles.act =
                    rank.banks[j].cycles.act.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.ref =
                    rank.banks[j].cycles.ref.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.selfRefresh =
                    rank.cycles.sref.get_count_at(timestamp);
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

            stats.rank_total[i].cycles.act = rank.cycles.act.get_count_at(timestamp);
            stats.rank_total[i].cycles.powerDownAct = rank.cycles.powerDownAct.get_count_at(timestamp);
            stats.rank_total[i].cycles.powerDownPre = rank.cycles.powerDownPre.get_count_at(timestamp);
            stats.rank_total[i].cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp);
            // TODO: check if this is correct
            // idle time not taken into account
            stats.rank_total[i].cycles.pre = simulation_duration - 
            (
                stats.rank_total[i].cycles.act +
                stats.rank_total[i].cycles.powerDownAct +
                stats.rank_total[i].cycles.powerDownPre +
                stats.rank_total[i].cycles.selfRefresh
            );
            // TODO remove
            uint64_t pre = rank.cycles.pre.get_count_at(timestamp);
            if ( pre != stats.rank_total[i].cycles.pre)
                std::cout << "[Error] invalid rank.cycles.pre calculation" << std::endl;
        }

        stats.commandBus = commandBus.get_stats(timestamp);
        stats.readBus = readBus.get_stats(timestamp);
        stats.writeBus = writeBus.get_stats(timestamp);

        // differential pair -> one line always active
        stats.clockStats = clock.get_stats_at(timestamp);
        stats.readDQSStats = readDQS_.get_stats_at(timestamp);
        stats.writeDQSStats = writeDQS_.get_stats_at(timestamp);
        if (memSpec.bitWidth == 16) {
            // two 2 differential pairs for upper and lower byte
            // TODO is there a special case ??? -> check thesis
            stats.readDQSStats *= 2;
            stats.writeDQSStats *= 2;
        }

        return stats;
    }

    SimulationStats DDR4::getStats() {
        return getWindowStats(getLastCommandTime());
    }

}
