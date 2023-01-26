#include "DDR4.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/ddr4/calculation_DDR4.h>
#include <DRAMPower/standards/ddr4/interface_calculation_DDR4.h>
#include <iostream>


namespace DRAMPower {

    DDR4::DDR4(const MemSpecDDR4 &memSpec)
		: memSpec(memSpec)
		, ranks(memSpec.numberOfRanks, {(std::size_t)memSpec.numberOfBanks})
		, commandBus{27}
		, readBus{memSpec.bitWidth}
		, writeBus{memSpec.bitWidth},
          readDQS_c(2, true),
          readDQS_t(2, true),
          writeDQS_c(2, true),
          writeDQS_t(2, true)
	{
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
                                                    V, V, V,
                                                    A12,
                                                    A17, A13, A11,
                                                    A10,
                                                    A0, A1, A2, A3, A4, A5, A6, A7, A8, A9
                                            });
        this->registerPattern<CmdType::PRE>({
                                                    L, H, L, H, L, BG0, BG1, BA0, BA1,
                                                    V, V, V,
                                                    V,
                                                    V, V, V,
                                                    V,
                                                    V, V, V, V, V, V, V, V, V, V
                                            });
        this->registerPattern<CmdType::PREA>({
                                                     L, H, L, H, L, V, V, V, V,
                                                     V, V, V,
                                                     V,
                                                     V, V, V,
                                                     H,
                                                     V, V, V, V, V, V, V, V, V, V
                                             });
        this->registerPattern<CmdType::REFA>({
                                                     L, H, L, L, H, V, V, V, V,
                                                     V, V, V,
                                                     V,
                                                     V, V, V,
                                                     V,
                                                     V, V, V, V, V, V, V, V, V, V
                                             });
        this->registerPattern<CmdType::RD>({
                                                   L, H, H, L, H, BG0, BG1, BA0, BA1,
                                                   V, V, V,
                                                   V,
                                                   V, V, V,
                                                   L,
                                                   A0, A1, A2, A3, A4, A5, A6, A7, A8, A9
                                           });
        this->registerPattern<CmdType::RDA>({
                                                    L, H, H, L, H, BG0, BG1, BA0, BA1,
                                                    V, V, V,
                                                    V,
                                                    V, V, V,
                                                    H,
                                                    A0, A1, A2, A3, A4, A5, A6, A7, A8, A9
                                            });
        this->registerPattern<CmdType::WR>({
                                                   L, H, H, L, L, BG0, BG1, BA0, BA1,
                                                   V, V, V,
                                                   V,
                                                   V, V, V,
                                                   L,
                                                   A0, A1, A2, A3, A4, A5, A6, A7, A8, A9
                                           });
        this->registerPattern<CmdType::WRA>({
                                                    L, H, H, L, L, BG0, BG1, BA0, BA1,
                                                    V, V, V,
                                                    V,
                                                    V, V, V,
                                                    H,
                                                    A0, A1, A2, A3, A4, A5, A6, A7, A8, A9
                                            });
        this->registerPattern<CmdType::SREFEN>({
                                                       L, H, L, L, H, V, V, V, V,
                                                       V, V, V,
                                                       V,
                                                       V, V, V,
                                                       V,
                                                       V, V, V, V, V, V, V, V, V, V
                                               });
        this->registerPattern<CmdType::SREFEX>({
                                                       H, X, X, X, X, X, X, X, X,
                                                       X, X, X,
                                                       X,
                                                       X, X, X,
                                                       X,
                                                       X, X, X, X, X, X, X, X, X, X,
                                                       L, H, H, H, H, V, V, V, V,
                                                       V, V, V,
                                                       V,
                                                       V, V, V,
                                                       V,
                                                       V, V, V, V, V, V, V, V, V, V
                                               });
        this->registerPattern<CmdType::PDEA>({
                                                     H, X, X, X, X, X, X, X, X,
                                                     X, X, X,
                                                     X,
                                                     X, X, X,
                                                     X,
                                                     X, X, X, X, X, X, X, X, X, X,
                                             });
        this->registerPattern<CmdType::PDXA>({
                                                     H, X, X, X, X, X, X, X, X,
                                                     X, X, X,
                                                     X,
                                                     X, X, X,
                                                     X,
                                                     X, X, X, X, X, X, X, X, X, X,
                                             });
        this->registerPattern<CmdType::PDEP>({
                                                     H, X, X, X, X, X, X, X, X,
                                                     X, X, X,
                                                     X,
                                                     X, X, X,
                                                     X,
                                                     X, X, X, X, X, X, X, X, X, X,
                                             });
        this->registerPattern<CmdType::PDXP>({
                                                     H, X, X, X, X, X, X, X, X,
                                                     X, X, X,
                                                     X,
                                                     X, X, X,
                                                     X,
                                                     X, X, X, X, X, X, X, X, X, X,
                                             });
    }

    uint64_t DDR4::encode(const Command& cmd, const std::vector<pattern_descriptor::t>& pattern) const
    {
        using namespace pattern_descriptor;

        std::bitset<64> bitset(0);
        std::bitset<32> bank_group_bits(cmd.targetCoordinate.bankGroup);
        std::bitset<32> bank_bits(cmd.targetCoordinate.bank);
        std::bitset<32> row_bits(cmd.targetCoordinate.row);
        std::bitset<32> column_bits(cmd.targetCoordinate.column);

        std::size_t n = pattern.size() - 1;
        bool is_act = (cmd.type == CmdType::ACT);
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
                    bitset[n] = true;
                    break;  // ToDo: Variabel machen

                    // Bank bits
                case BA0:
                    bitset[n] = bank_bits[0];
                    break;
                case BA1:
                    bitset[n] = bank_bits[1];
                    break;

                    // Bank Group bits
                case BG0:
                    bitset[n] = bank_group_bits[0];
                    break;
                case BG1:
                    if(this->memSpec.bitWidth == 16)
                        bitset[n] = true; // BG1 is only defined for x4 and x8 configurations
                    else
                        bitset[n] = bank_group_bits[1];
                    break;

                    // Row/Column bits
                case A0:
                    bitset[n] = is_act ? row_bits[0] : column_bits[0];
                    break;
                case A1:
                    bitset[n] = is_act ? row_bits[1] : column_bits[1];
                    break;
                case A2:
                    bitset[n] = is_act ? row_bits[2] : column_bits[2];
                    break;
                case A3:
                    bitset[n] = is_act ? row_bits[3] : column_bits[3];
                    break;
                case A4:
                    bitset[n] = is_act ? row_bits[4] : column_bits[4];
                    break;
                case A5:
                    bitset[n] = is_act ? row_bits[5] : column_bits[5];
                    break;
                case A6:
                    bitset[n] = is_act ? row_bits[6] : column_bits[6];
                    break;
                case A7:
                    bitset[n] = is_act ? row_bits[7] : column_bits[7];
                    break;
                case A8:
                    bitset[n] = is_act ? row_bits[8] : column_bits[8];
                    break;
                case A9:
                    bitset[n] = is_act ? row_bits[9] : column_bits[9];
                    break;
                case A10:
                    bitset[n] = row_bits[10];
                    break;
                case A11:
                    bitset[n] = row_bits[11];
                    break;
                case A12:
                    bitset[n] = row_bits[12];
                    break;
                case A13:
                    bitset[n] = row_bits[13];
                    break;
                case A14:
                    bitset[n] = row_bits[14];
                    break;
                case A15:
                    bitset[n] = row_bits[15];
                    break;
                case A16:
                    bitset[n] = row_bits[16];
                    break;
                case A17:
                    if(this->memSpec.bitWidth == 4)
                        bitset[n] = row_bits[17]; // A17 is only defined for x4 configuration
                    else
                        bitset[n] = true;
                    break;
                default:
                    break;
            }

            --n;
        }

        return bitset.to_ullong();
    }

    void DDR4::handle_interface(const Command &cmd) {
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
                // TODO: Compute CRC code
                /*
                * 1. Data is known
                *      - Calculate CRC
                *      - Append to the sequence of bits
                * 2. Data is not known
                *
                */
                auto length = cmd.sz_bits / writeBus.get_width();
                this->writeBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);

                writeDQS_c.start(cmd.timestamp);
                writeDQS_c.stop(cmd.timestamp + length / this->memSpec.dataRateSpec.dqsBusRate);

                writeDQS_t.start(cmd.timestamp);
                writeDQS_t.stop(cmd.timestamp + length / this->memSpec.dataRateSpec.dqsBusRate);
            }
                break;
            default:
                break;
        }
    }

    void DDR4::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
        bank.counter.act++;
        bank.cycles.act.start_interval(timestamp);
        if ( !rank.isActive(timestamp) ) rank.cycles.act.start_interval(timestamp);
        bank.bankState = Bank::BankState::BANK_ACTIVE;
    }

    void DDR4::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
        if (bank.bankState == Bank::BankState::BANK_PRECHARGED) return;
        bank.counter.pre++;
        bank.cycles.act.close_interval(timestamp);
        bank.latestPre = timestamp;
        bank.bankState = Bank::BankState::BANK_PRECHARGED;
        if ( !rank.isActive(timestamp) )rank.cycles.act.close_interval(timestamp);
    }

    void DDR4::handlePreAll(Rank &rank, timestamp_t timestamp) {
        for (auto &bank: rank.banks) handlePre(rank, bank, timestamp);
    }

    void DDR4::handleRefAll(Rank &rank, timestamp_t timestamp) {
        for (auto& bank : rank.banks) {
            ++bank.counter.refAllBank;
            if (!rank.isActive(timestamp)) rank.cycles.act.start_interval(timestamp);
            bank.bankState = Bank::BankState::BANK_ACTIVE;
            auto timestamp_end = timestamp + memSpec.memTimingSpec.tRFC;
            bank.refreshEndTime = timestamp_end;
            if (!bank.cycles.act.is_open()) bank.cycles.act.start_interval(timestamp);

            // Execute implicit pre-charge at refresh end
            addImplicitCommand(timestamp_end, [this, &bank, &rank, timestamp_end]() {
                bank.bankState = Bank::BankState::BANK_PRECHARGED;
                bank.cycles.act.close_interval(timestamp_end);
                if (!rank.isActive(timestamp_end)) rank.cycles.act.close_interval(timestamp_end);
            });
        }

        // Required for precharge power-down
        rank.endRefreshTime = timestamp + memSpec.memTimingSpec.tRFC;
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
        assert(rank.memState == MemState::SREF);
        rank.cycles.sref.close_interval(timestamp);  // Duration between entry and exit
        rank.memState = MemState::NOT_IN_PD;
    }

    void DDR4::handlePowerDownActEntry(Rank &rank, timestamp_t timestamp) {
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
            };
        });
    }

    void DDR4::handlePowerDownActExit(Rank &rank, timestamp_t timestamp) {
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

    void DDR4::handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp) {
        auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
        auto entryTime = std::max(timestamp, earliestPossibleEntry);

        addImplicitCommand(entryTime, [this, &rank, entryTime]() {
            rank.cycles.powerDownPre.start_interval(entryTime);
            rank.memState = MemState::PDN_PRE;
        });
    }

    void DDR4::handlePowerDownPreExit(Rank &rank, timestamp_t timestamp) {
        // TODO: Is this computation necessary?
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [this, &rank, exitTime]() {
            rank.memState = MemState::NOT_IN_PD;
            rank.cycles.powerDownPre.close_interval(exitTime);
        });
    }

    void DDR4::endOfSimulation(timestamp_t timestamp) {
        if (this->implicitCommandCount() > 0) 
			std::cout << ("[WARN] End of simulation but still implicit commands left!");
	}

    energy_t DDR4::calcEnergy(timestamp_t timestamp) {
        Calculation_DDR4 calculation;
        return calculation.calcEnergy(timestamp, *this);
    }

    interface_energy_info_t DDR4::calcInterfaceEnergy(timestamp_t timestamp) {
        return {};
    }

    SimulationStats DDR4::getWindowStats(timestamp_t timestamp) {
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
            stats.bank[i].cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp);
            stats.bank[i].cycles.powerDownAct = rank.cycles.powerDownAct.get_count_at(timestamp);
            stats.bank[i].cycles.powerDownPre = rank.cycles.powerDownPre.get_count_at(timestamp);
            stats.bank[i].cycles.pre = simulation_duration - (stats.bank[i].cycles.act + rank.cycles.powerDownAct.get_count_at(timestamp) + rank.cycles.powerDownPre.get_count_at(timestamp) + + rank.cycles.sref.get_count_at(timestamp));
        }

        stats.total.cycles.pre = simulation_duration - (rank.cycles.act.get_count_at(timestamp) + rank.cycles.powerDownAct.get_count_at(timestamp) + rank.cycles.powerDownPre.get_count_at(timestamp) + rank.cycles.sref.get_count_at(timestamp));
        stats.total.cycles.act = rank.cycles.act.get_count_at(timestamp);
        stats.total.cycles.ref = rank.cycles.ref.get_count_at(timestamp); //TODO: I think this counter is never updated
        stats.total.cycles.powerDownAct = rank.cycles.powerDownAct.get_count_at(timestamp);
        stats.total.cycles.powerDownPre = rank.cycles.powerDownPre.get_count_at(timestamp);
        stats.total.cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp);

        return stats;
    }

    SimulationStats DDR4::getStats() {
        return getWindowStats(getLastCommandTime());
    }

}