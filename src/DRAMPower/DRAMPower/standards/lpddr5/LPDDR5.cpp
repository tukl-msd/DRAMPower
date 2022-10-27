#include "LPDDR5.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/lpddr5/calculation_LPDDR5.h>
#include <DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.h>
#include <iostream>


namespace DRAMPower {

	LPDDR5::LPDDR5(const MemSpecLPDDR5 &memSpec)
		: memSpec(memSpec)
		, ranks(memSpec.numberOfRanks, { (std::size_t)memSpec.numberOfBanks })
		, commandBus{6}
		, readBus{6}
		, writeBus{6}
	{
        this->registerPatterns();

        this->registerBankHandler<CmdType::ACT>(&LPDDR5::handleAct);
        this->registerBankHandler<CmdType::PRE>(&LPDDR5::handlePre);
        this->registerRankHandler<CmdType::PREA>(&LPDDR5::handlePreAll);
        this->registerBankHandler<CmdType::REFB>(&LPDDR5::handleRefPerBank);
        this->registerBankHandler<CmdType::RD>(&LPDDR5::handleRead);
        this->registerBankHandler<CmdType::RDA>(&LPDDR5::handleReadAuto);
        this->registerBankHandler<CmdType::WR>(&LPDDR5::handleWrite);
        this->registerBankHandler<CmdType::WRA>(&LPDDR5::handleWriteAuto);

        this->registerBankGroupHandler<CmdType::REFP2B>(&LPDDR5::handleRefPerTwoBanks);

        this->registerRankHandler<CmdType::REFA>(&LPDDR5::handleRefAll);
        this->registerRankHandler<CmdType::SREFEN>(&LPDDR5::handleSelfRefreshEntry);
        this->registerRankHandler<CmdType::SREFEX>(&LPDDR5::handleSelfRefreshExit);
        this->registerRankHandler<CmdType::PDEA>(&LPDDR5::handlePowerDownActEntry);
        this->registerRankHandler<CmdType::PDEP>(&LPDDR5::handlePowerDownPreEntry);
        this->registerRankHandler<CmdType::PDXA>(&LPDDR5::handlePowerDownActExit);
        this->registerRankHandler<CmdType::PDXP>(&LPDDR5::handlePowerDownPreExit);
        this->registerRankHandler<CmdType::DSMEN>(&LPDDR5::handleDSMEntry);
        this->registerRankHandler<CmdType::DSMEX>(&LPDDR5::handleDSMExit);


        routeCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
    };

    void LPDDR5::registerPatterns() {
        using namespace pattern_descriptor;

        // ---------------------------------:
        this->registerPattern<CmdType::ACT>({

                                            });
        this->registerPattern<CmdType::PRE>({

                                            });
        this->registerPattern<CmdType::PRESB>({

                                              });
        this->registerPattern<CmdType::PREA>({

                                             });
        this->registerPattern<CmdType::REFSB>({

                                              });
        this->registerPattern<CmdType::REFA>({

                                             });
        this->registerPattern<CmdType::RD>({

                                           });
        this->registerPattern<CmdType::RDA>({

                                            });
        this->registerPattern<CmdType::WR>({

                                           });
        this->registerPattern<CmdType::WRA>({

                                            });
        this->registerPattern<CmdType::SREFEN>({

                                               });
        this->registerPattern<CmdType::PDEA>({

                                             });
        this->registerPattern<CmdType::PDXA>({

                                             });
        this->registerPattern<CmdType::PDEP>({

                                             });
        this->registerPattern<CmdType::PDXP>({

                                             });
    }

    void LPDDR5::handle_interface(const Command &cmd) {
        auto pattern = this->getCommandPattern(cmd);
        auto length = this->getPattern(cmd.type).size() / commandBus.get_width();
        this->commandBus.load(cmd.timestamp, pattern, length);
    }

    void LPDDR5::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
        bank.counter.act++;

        bank.cycles.act.start_interval(timestamp);

        if ( !rank.isActive(timestamp) ) {
            rank.cycles.act.start_interval(timestamp);
        }

        bank.bankState = Bank::BankState::BANK_ACTIVE;
    }

    void LPDDR5::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
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

    void LPDDR5::handlePreAll(Rank &rank, timestamp_t timestamp) {
        for (auto &bank: rank.banks) {
            handlePre(rank, bank, timestamp);
        }
    }

    void LPDDR5::handleRefPerBank(Rank & rank, Bank & bank, timestamp_t timestamp) {
        handleRefreshOnBank(rank, bank, timestamp, memSpec.memTimingSpec.tRFCPB, bank.counter.refPerBank);
    }

    void LPDDR5::handleRefPerTwoBanks(Rank & rank, std::size_t bank_id, timestamp_t timestamp) {
        Bank & bank_1 = rank.banks[bank_id];
        Bank & bank_2 = rank.banks[(bank_id + memSpec.perTwoBankOffset)%16];
        handleRefreshOnBank(rank, bank_1, timestamp, memSpec.memTimingSpec.tRFCPB, bank_1.counter.refPerTwoBanks);
        handleRefreshOnBank(rank, bank_2, timestamp, memSpec.memTimingSpec.tRFCPB, bank_2.counter.refPerTwoBanks);
    }

    void LPDDR5::handleRefAll(Rank &rank, timestamp_t timestamp) {
        for (auto& bank : rank.banks) {
            handleRefreshOnBank(rank, bank, timestamp, memSpec.memTimingSpec.tRFC, bank.counter.refAllBank);
        }
        rank.endRefreshTime = timestamp + memSpec.memTimingSpec.tRFC;
    }

    void LPDDR5::handleRefreshOnBank(Rank & rank, Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t & counter){
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

    void LPDDR5::handleRead(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.reads;
    }

    void LPDDR5::handleReadAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.readAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
        auto minReadActiveTime = timestamp + this->memSpec.prechargeOffsetRD;

        auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void LPDDR5::handleWrite(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.writes;
    }

    void LPDDR5::handleWriteAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.writeAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
        auto minWriteActiveTime = timestamp + this->memSpec.prechargeOffsetWR;

        auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);
        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void LPDDR5::handleSelfRefreshEntry(Rank &rank, timestamp_t timestamp) {
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

    void LPDDR5::handleSelfRefreshExit(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::SREF);
        rank.cycles.sref.close_interval(timestamp);
        rank.memState = MemState::NOT_IN_PD;
    }

    void LPDDR5::handlePowerDownActEntry(Rank &rank, timestamp_t timestamp) {
        auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
        auto entryTime = std::max(timestamp, earliestPossibleEntry);
        addImplicitCommand(entryTime, [this, &rank, entryTime]() {
            rank.cycles.powerDownAct.start_interval(entryTime);
            rank.memState = MemState::PDN_ACT;
            if (rank.cycles.act.is_open()) {
                rank.cycles.act.close_interval(entryTime);
            }
            for (auto & bank : rank.banks) {
                if (bank.cycles.act.is_open()) {
                    bank.cycles.act.close_interval(entryTime);
                }
            }
        });
    }

    void LPDDR5::handlePowerDownActExit(Rank &rank, timestamp_t timestamp) {
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
    }

    void LPDDR5::handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp) {
        auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
        auto entryTime = std::max(timestamp, earliestPossibleEntry);
        addImplicitCommand(entryTime, [this, &rank, entryTime]() {
            rank.cycles.powerDownPre.start_interval(entryTime);
            rank.memState = MemState::PDN_PRE;
        });
    }

    void LPDDR5::handlePowerDownPreExit(Rank &rank, timestamp_t timestamp) {
        // TODO: Is this computation necessary?
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [this, &rank, exitTime]() {
            rank.memState = MemState::NOT_IN_PD;
            rank.cycles.powerDownPre.close_interval(exitTime);
        });
    }

    void LPDDR5::handleDSMEntry(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::SREF);
        rank.cycles.deepSleepMode.start_interval(timestamp);
        rank.counter.deepSleepMode++;
        rank.memState = MemState::DSM;
    }

    void LPDDR5::handleDSMExit(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::DSM);
        rank.cycles.deepSleepMode.close_interval(timestamp);
        rank.memState = MemState::SREF;
    }

    void LPDDR5::endOfSimulation(timestamp_t timestamp) {
        if (this->implicitCommandCount() > 0)
			std::cout << ("[WARN] End of simulation but still implicit commands left!");
	}

    energy_t LPDDR5::calcEnergy(timestamp_t timestamp) {
        Calculation_LPDDR5 calculation;
        return calculation.calcEnergy(timestamp, *this);
    }

    interface_energy_info_t LPDDR5::calcInterfaceEnergy(timestamp_t timestamp) {
		return {};
    }

    SimulationStats LPDDR5::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        this->processImplicitCommandQueue(timestamp);

        SimulationStats stats;
        stats.bank.resize(this->memSpec.numberOfBanks);

        auto & rank = this->ranks[0];
        auto simulation_duration = timestamp;

        for (std::size_t i = 0; i < this->memSpec.numberOfBanks; ++i) {
            stats.bank[i].counter = rank.banks[i].counter;
            stats.bank[i].cycles.act = rank.banks[i].cycles.act.get_count_at(timestamp);
            stats.bank[i].cycles.ref = rank.banks[i].cycles.ref.get_count_at(timestamp);
            stats.bank[i].cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp) -
                                               rank.cycles.deepSleepMode.get_count_at(timestamp);
            stats.bank[i].cycles.deepSleepMode =  rank.cycles.deepSleepMode.get_count_at(timestamp);
            stats.bank[i].cycles.powerDownAct = rank.cycles.powerDownAct.get_count_at(timestamp);
            stats.bank[i].cycles.powerDownPre = rank.cycles.powerDownPre.get_count_at(timestamp);
            stats.bank[i].cycles.pre = simulation_duration - (stats.bank[i].cycles.act +
                                                              rank.cycles.powerDownAct.get_count_at(timestamp) +
                                                              rank.cycles.powerDownPre.get_count_at(timestamp) +
                                                              rank.cycles.sref.get_count_at(timestamp));
        }

        stats.total.cycles.pre = simulation_duration - (rank.cycles.act.get_count_at(timestamp) +
                                                        rank.cycles.powerDownAct.get_count_at(timestamp) +
                                                        rank.cycles.powerDownPre.get_count_at(timestamp) +
                                                        rank.cycles.sref.get_count_at(timestamp));

        stats.total.cycles.act = rank.cycles.act.get_count_at(timestamp);
        stats.total.cycles.ref = rank.cycles.act.get_count_at(timestamp); //TODO: I think this counter is never updated
        stats.total.cycles.powerDownAct = rank.cycles.powerDownAct.get_count_at(timestamp);
        stats.total.cycles.powerDownPre = rank.cycles.powerDownPre.get_count_at(timestamp);
        stats.total.cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp) -
                                         rank.cycles.deepSleepMode.get_count_at(timestamp);
        stats.total.cycles.deepSleepMode = rank.cycles.deepSleepMode.get_count_at(timestamp);

        stats.commandBus = this->commandBus.get_stats(timestamp);

        return stats;
    }

    SimulationStats LPDDR5::getStats() {
        return getWindowStats(getLastCommandTime());
    }

}
