#include "LPDDR5.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/lpddr5/calculation_LPDDR5.h>
#include <DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.h>
#include <iostream>


namespace DRAMPower {

	LPDDR5::LPDDR5(const MemSpecLPDDR5 &memSpec)
		: memSpec(memSpec)
		, ranks(memSpec.numberOfRanks, { (std::size_t)memSpec.numberOfBanks })
		, commandBus{7}
		, readBus{memSpec.bitWidth}
		, writeBus{memSpec.bitWidth},
          readDQS_c(2, true),
          readDQS_t(2, true),
          writeDQS_c(2, true),
          writeDQS_t(2, true)
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
        if (this->memSpec.BGroupMode) {
            this->registerPattern<CmdType::ACT>({
                                                        H, H, H, R14, R15, R16, R17,
                                                        BA0, BA1, BG0, BG1, R11, R12, R13,
                                                        H, H, L, R7, R8, R9, R10,
                                                        R0, R1, R2, R3, R4, R5, R6});
            this->registerPattern<CmdType::PRE>({
                                                        L, L, L, H, H, H, H,
                                                        BA0, BA1, BG0, BG1, V, V, L
                                                });
            this->registerPattern<CmdType::RD>({
                                                        H, L, L, C0, C3, C4, C5,
                                                        BA0, BA1, BG0, BG1, C1, C2, L
                                               });
            this->registerPattern<CmdType::RDA>({
                                                        H, L, L, C0, C3, C4, C5,
                                                        BA0, BA1, BG0, BG1, C1, C2, H
                                                });
            this->registerPattern<CmdType::WR>({
                                                        L, H, H, C0, C3, C4, C5,
                                                        BA0, BA1, BG0, BG1, C1, C2, L
                                               });
            this->registerPattern<CmdType::WRA>({
                                                        L, H, H, C0, C3, C4, C5,
                                                        BA0, BA1, BG0, BG1, C1, C2, H
                                                });
            this->registerPattern<CmdType::REFB>({
                                                        L, L, L, H, H, H, L,
                                                        BA0, BA1, BG0, V, V, V, L // see NOTE 14 and section 7.7.5.1
                                                 });
            this->registerPattern<CmdType::REFP2B>({
                                                        L, L, L, H, H, H, L,
                                                        BA0, BA1, BG0, V, V, V, L // see NOTE 14 and section 7.7.5.1
                                                   });
            this->registerPattern<CmdType::REFA>({
                                                        L, L, L, H, H, H, L,
                                                        BA0, BA1, BG0, V, V, V, H // see NOTE 14 and section 7.7.5.1
                                                 });

        }
        else if (this->memSpec.numberOfBanks == 16){
            this->registerPattern<CmdType::ACT>({
                                                        H, H, H, R14, R15, R16, R17,
                                                        BA0, BA1, BA2, BA3, R11, R12, R13,
                                                        H, H, L, R7, R8, R9, R10,
                                                        R0, R1, R2, R3, R4, R5, R6});
            this->registerPattern<CmdType::PRE>({
                                                        L, L, L, H, H, H, H,
                                                        BA0, BA1, BA2, BA3, V, V, L
                                                });
            this->registerPattern<CmdType::RD>({
                                                        H, L, L, C0, C3, C4, C5,
                                                        BA0, BA1, BA2, BA3, C1, C2, L
                                               });
            this->registerPattern<CmdType::RDA>({
                                                        H, L, L, C0, C3, C4, C5,
                                                        BA0, BA1, BA2, BA3, C1, C2, H
                                                });
            this->registerPattern<CmdType::WR>({
                                                        L, H, H, C0, C3, C4, C5,
                                                        BA0, BA1, BA2, BA3, C1, C2, L
                                               });
            this->registerPattern<CmdType::WRA>({
                                                        L, H, H, C0, C3, C4, C5,
                                                        BA0, BA1, BA2, BA3, C1, C2, H
                                                });
            this->registerPattern<CmdType::REFB>({
                                                         L, L, L, H, H, H, L,
                                                         BA0, BA1, BA2, V, V, V, L // see NOTE 14 and section 7.7.5.1
                                                 });
            this->registerPattern<CmdType::REFP2B>({
                                                           L, L, L, H, H, H, L,
                                                           BA0, BA1, BA2, V, V, V, L // see NOTE 14 and section 7.7.5.1
                                                   });
            this->registerPattern<CmdType::REFA>({
                                                         L, L, L, H, H, H, L,
                                                         BA0, BA1, BA2, V, V, V, H // see NOTE 14 and section 7.7.5.1
                                                 });
        }
        else {
            this->registerPattern<CmdType::ACT>({
                                                        H, H, H, R14, R15, R16, R17,
                                                        BA0, BA1, BA2, V, R11, R12, R13,
                                                        H, H, L, R7, R8, R9, R10,
                                                        R0, R1, R2, R3, R4, R5, R6});
            this->registerPattern<CmdType::PRE>({
                                                        L, L, L, H, H, H, H,
                                                        BA0, BA1, BA2, V, V, V, L
                                                });
            this->registerPattern<CmdType::RD>({
                                                       H, L, L, C0, C3, C4, C5,
                                                       BA0, BA1, BA2, V, C1, C2, L
                                               });
            this->registerPattern<CmdType::RDA>({
                                                        H, L, L, C0, C3, C4, C5,
                                                        BA0, BA1, BA2, V, C1, C2, H
                                                });
            this->registerPattern<CmdType::WR>({
                                                       L, H, H, C0, C3, C4, C5,
                                                       BA0, BA1, BA2, V, C1, C2, L
                                               });
            this->registerPattern<CmdType::WRA>({
                                                        L, H, H, C0, C3, C4, C5,
                                                        BA0, BA1, BA2, V, C1, C2, H
                                                });
            this->registerPattern<CmdType::REFB>({
                                                         L, L, L, H, H, H, L,
                                                         BA0, BA1, BA2, V, V, V, L // see NOTE 14 and section 7.7.5.1
                                                 });
            this->registerPattern<CmdType::REFP2B>({
                                                           L, L, L, H, H, H, L,
                                                           BA0, BA1, BA2, V, V, V, L // see NOTE 14 and section 7.7.5.1
                                                   });
            this->registerPattern<CmdType::REFA>({
                                                         L, L, L, H, H, H, L,
                                                         BA0, BA1, BA2, V, V, V, H // see NOTE 14 and section 7.7.5.1
                                                 });
        }

        this->registerPattern<CmdType::PREA>({
                                                     L, L, L, H, H, H, H,
                                                     X, X, X, X, V, V, H
                                             });

        this->registerPattern<CmdType::SREFEN>({
                                                    L, L, L, H, L, H, H, // See NOTE 9
                                                    V, V, V, V, V, L, L
                                               });
        this->registerPattern<CmdType::SREFEX>({
                                                       L, L, L, H, L, H, L,
                                                       V, V, V, V, V, V, V
                                               });
        this->registerPattern<CmdType::PDEA>({
                                                    L, L, L, L, L, L, H,
                                                    X, X, X, X, X, X, X
                                             });
        this->registerPattern<CmdType::PDXA>({
                                                     X, X, X, X, X, X, X, // See section 7.5.7.1
                                                     X, X, X, X, X, X, X
                                             });
        this->registerPattern<CmdType::PDEP>({
                                                     L, L, L, L, L, L, H,
                                                     X, X, X, X, X, X, X
                                             });
        this->registerPattern<CmdType::PDXP>({
                                                     X, X, X, X, X, X, X, // See section 7.5.7.1
                                                     X, X, X, X, X, X, X
                                             });

        this->registerPattern<CmdType::DSMEN>({
                                                      L, L, L, H, L, H, H, // See NOTE 9
                                                      V, V, V, V, V, H, L
                                              });

        this->registerPattern<CmdType::DSMEX>({
                                                      L, L, L, H, L, H, L, // TODO: Assuming DSM exit command is the same as SREFEX
                                                      V, V, V, V, V, V, V
                                              });
    }

    uint64_t LPDDR5::encode(const Command& cmd, const std::vector<pattern_descriptor::t>& pattern) const  {
        using namespace pattern_descriptor;

        std::bitset<64> bitset(0);
        std::bitset<32> bank_group_bits(cmd.targetCoordinate.bankGroup);
        std::bitset<32> bank_bits(cmd.targetCoordinate.bank);
        std::bitset<32> row_bits(cmd.targetCoordinate.row);
        std::bitset<32> column_bits(cmd.targetCoordinate.column);

        std::size_t n = pattern.size() - 1;

        for (const auto descriptor : pattern) {
            assert(n >= 0);

            switch (descriptor) {
                case H:
                    bitset[n] = true;
                    break;
                case L:
                    bitset[n] = false;
                    break;
                case V:
                case X:
                    bitset[n] = false;
                    break;

                    // Bank bits
                case BA0:
                    bitset[n] = bank_bits[0];
                    break;
                case BA1:
                    bitset[n] = bank_bits[1];
                    break;
                case BA2:
                    bitset[n] = bank_bits[2];
                    break;
                case BA3:
                    bitset[n] = bank_bits[3];
                    break;

                    // Bank Group bits
                case BG0:
                    bitset[n] = bank_group_bits[0];
                    break;
                case BG1:
                    bitset[n] = bank_group_bits[1];
                    break;

                    // Column bits
                case C0:
                    bitset[n] = column_bits[0];
                    break;
                case C1:
                    bitset[n] = column_bits[1];
                    break;
                case C2:
                    bitset[n] = column_bits[2];
                    break;
                case C3:
                    bitset[n] = column_bits[3];
                    break;
                case C4:
                    bitset[n] = column_bits[4];
                    break;
                case C5:
                    bitset[n] = column_bits[5];
                    break;

                    // Row bits
                case R0:
                    bitset[n] = row_bits[0];
                    break;
                case R1:
                    bitset[n] = row_bits[1];
                    break;
                case R2:
                    bitset[n] = row_bits[2];
                    break;
                case R3:
                    bitset[n] = row_bits[3];
                    break;
                case R4:
                    bitset[n] = row_bits[4];
                    break;
                case R5:
                    bitset[n] = row_bits[5];
                    break;
                case R6:
                    bitset[n] = row_bits[6];
                    break;
                case R7:
                    bitset[n] = row_bits[7];
                    break;
                case R8:
                    bitset[n] = row_bits[8];
                    break;
                case R9:
                    bitset[n] = row_bits[9];
                    break;
                case R10:
                    bitset[n] = row_bits[10];
                    break;
                case R11:
                    bitset[n] = row_bits[11];
                    break;
                case R12:
                    bitset[n] = row_bits[12];
                    break;
                case R13:
                    bitset[n] = row_bits[13];
                    break;
                case R14:
                    bitset[n] = row_bits[14];
                    break;
                case R15:
                    bitset[n] = row_bits[15];
                    break;
                case R16:
                    bitset[n] = row_bits[16];
                    break;
                case R17:
                    bitset[n] = row_bits[17];
                    break;

                default:
                    break;
            }

            --n;
        }

        return bitset.to_ullong();
    }

    void LPDDR5::handle_interface(const Command &cmd) {
        auto pattern = this->getCommandPattern(cmd);
        auto length = this->getPattern(cmd.type).size() / commandBus.get_width();
        this->commandBus.load(cmd.timestamp, pattern, length); // command and address (if any)

        switch (cmd.type) {
            case CmdType::RD:
            case CmdType::RDA: {
                auto length = cmd.sz_bits / readBus.get_width();
                this->readBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);

                readDQS_c.start(cmd.timestamp);
                readDQS_c.stop(cmd.timestamp + length / this->memSpec.dataRateSpec.dqsBusRate);

                readDQS_t.start(cmd.timestamp);
                readDQS_t.stop(cmd.timestamp + length / this->memSpec.dataRateSpec.dqsBusRate);
            }
                break;
            case CmdType::WR:
            case CmdType::WRA: {
                auto length = cmd.sz_bits / writeBus.get_width();
                this->writeBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);

                writeDQS_c.start(cmd.timestamp);
                writeDQS_c.stop(cmd.timestamp + length / this->memSpec.dataRateSpec.dqsBusRate);

                writeDQS_t.start(cmd.timestamp);
                writeDQS_t.stop(cmd.timestamp + length / this->memSpec.dataRateSpec.dqsBusRate);
            }
                break;
        }
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
