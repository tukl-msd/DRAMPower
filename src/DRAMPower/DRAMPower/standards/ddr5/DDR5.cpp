#include "DDR5.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/ddr5/calculation_DDR5.h>
#include <DRAMPower/standards/ddr5/interface_calculation_DDR5.h>
#include <iostream>


namespace DRAMPower {

    DDR5::DDR5(const MemSpecDDR5 &memSpec) 
		: memSpec(memSpec)
		, ranks(memSpec.numberOfRanks, { (std::size_t)memSpec.numberOfBanks})
		, commandBus{14}
		, readBus{memSpec.bitWidth}
		, writeBus{memSpec.bitWidth},
          readDQS_c(2, true),
          readDQS_t(2, true),
          writeDQS_c(2, true),
          writeDQS_t(2, true)
	{
        this->registerPatterns();

        this->registerBankHandler<CmdType::ACT>(&DDR5::handleAct);
        this->registerBankHandler<CmdType::PRE>(&DDR5::handlePre);
        this->registerBankHandler<CmdType::RD>(&DDR5::handleRead);
        this->registerBankHandler<CmdType::RDA>(&DDR5::handleReadAuto);
        this->registerBankHandler<CmdType::WR>(&DDR5::handleWrite);
        this->registerBankHandler<CmdType::WRA>(&DDR5::handleWriteAuto);

        this->registerBankGroupHandler<CmdType::PRESB>(&DDR5::handlePreSameBank);
        this->registerBankGroupHandler<CmdType::REFSB>(&DDR5::handleRefSameBank);

        this->registerRankHandler<CmdType::REFA>(&DDR5::handleRefAll);
        this->registerRankHandler<CmdType::PREA>(&DDR5::handlePreAll);
        this->registerRankHandler<CmdType::SREFEN>(&DDR5::handleSelfRefreshEntry);
        this->registerRankHandler<CmdType::SREFEX>(&DDR5::handleSelfRefreshExit);
        this->registerRankHandler<CmdType::PDEA>(&DDR5::handlePowerDownActEntry);
        this->registerRankHandler<CmdType::PDEP>(&DDR5::handlePowerDownPreEntry);
        this->registerRankHandler<CmdType::PDXA>(&DDR5::handlePowerDownActExit);
        this->registerRankHandler<CmdType::PDXP>(&DDR5::handlePowerDownPreExit);

        routeCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
    };

    void DDR5::registerPatterns() {
        using namespace pattern_descriptor;

        // ---------------------------------:
        this->registerPattern<CmdType::ACT>({
            L, L, R0, R1, R2, R3, BA0, BA1, BG0, BG1, CID0, CID1, CID2,
            R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16, R17
        });
        this->registerPattern<CmdType::PRE>({
            H, H, L, H, H, CID3, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2
                                            });
        this->registerPattern<CmdType::PRESB>({
            H, H, L, H, L, CID3, BA0, BA1, V, V, H, CID0, CID1, CID2
                                            });
        this->registerPattern<CmdType::PREA>({
            H, H, L, H, L, CID3, V, V, V, V, L, CID0, CID1, CID2
                                             });
        this->registerPattern<CmdType::REFSB>({ // TODO: see NOTES 23 and 24
            H, H, L, L, H, CID3, BA0, BA1, V, V, H, CID0, CID1, CID2
                                             });
        this->registerPattern<CmdType::REFA>({
            H, H, L, L, H, CID3, V, V, V, L, L, CID0, CID1, CID2
                                             });
        this->registerPattern<CmdType::RD>({
            H, L, H, H, H, BL, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2, // TODO: see NOTE 15
            C2, C3, C4, C5, C6, C7, C8, C9, C10, V, H, V, V, CID3
                                           });
        this->registerPattern<CmdType::RDA>({
            H, L, H, H, H, BL, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2, // TODO: see NOTE 15
            C2, C3, C4, C5, C6, C7, C8, C9, C10, V, L, V, V, CID3
                                            });
        this->registerPattern<CmdType::WR>({ // TODO: see NOTES 12 and 15
            H, L, H, H, L, BL, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            V, C3, C4, C5, C6, C7, C8, C9, C10, V, H, H, V, CID3
                                           });
        this->registerPattern<CmdType::WRA>({ // TODO: see NOTE 12 and 15
            H, L, H, H, L, BL, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            V, C3, C4, C5, C6, C7, C8, C9, C10, V, L, H, V, CID3
                                            });
        this->registerPattern<CmdType::SREFEN>({
            H, H, H, L, H, V, V, V, V, H, L, V, V, V
                                               });

        this->registerPattern<CmdType::SREFEX>({    // From section 4.9 (page 152):
            H, H, H, H, H, V, V, V, V, V, V, V, V, V,      //      "Self Refresh entry is command based (SRE), while the
            H, H, H, H, H, V, V, V, V, V, V, V, V, V,      //      Self-Refresh Exit Command is defined by the transition of
            H, H, H, H, H, V, V, V, V, V, V, V, V, V,      //      CS_n LOW to HIGH with a defined pulse width tCSH_SRexit,
        });                                                //      followed by three or more NOP commands (tCSL_SRexit) to
                                                           //      ensure DRAM stability in recognizing the exit."

        this->registerPattern<CmdType::PDEA>({ // TODO: see NOTE 16
            H, H, H, L, H, V, V, V, V, V, H, L, V, V
                                             });
        this->registerPattern<CmdType::PDXA>({
            H, H, H, H, H, V, V, V, V, V, V, V, V, V
                                             });
        this->registerPattern<CmdType::PDEP>({ // TODO: see NOTE 16
            H, H, H, L, H, V, V, V, V, V, H, L, V, V

                                             });
        this->registerPattern<CmdType::PDXP>({
            H, H, H, H, H, V, V, V, V, V, V, V, V, V
                                             });
    }

    uint64_t DDR5::encode(const Command& cmd, const std::vector<pattern_descriptor::t>& pattern) const  {
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
                    bitset[n] = true;
                    break; // LPDDR4, // ToDo: Variabel machen
                case BL:
                    bitset[n] = true;
                    break; // ToDo: Variabel machen

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
                    bitset[n] = bank_group_bits[1];
                    break;
                case BG2:
                    bitset[n] = bank_group_bits[2];
                    break;

                    // Column bits
                case C3:
                    bitset[n] = column_bits[0];
                    break;
                case C4:
                    bitset[n] = column_bits[1];
                    break;
                case C5:
                    bitset[n] = column_bits[2];
                    break;
                case C6:
                    bitset[n] = column_bits[3];
                    break;
                case C7:
                    bitset[n] = column_bits[4];
                    break;
                case C8:
                    bitset[n] = column_bits[5];
                    break;
                case C9:
                    bitset[n] = column_bits[6];
                    break;
                case C10:
                    bitset[n] = column_bits[7];
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



                case CID0:
                case CID1:
                case CID2:
                case CID3:
                    bitset[n] = false;
                    break;


                default:
                    break;
            }

            --n;
        }

        return bitset.to_ullong();
    }

    void DDR5::handle_interface(const Command &cmd) {
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

    void DDR5::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
        bank.counter.act++;

        bank.cycles.act.start_interval(timestamp);

        if ( !rank.isActive(timestamp) ) {
            rank.cycles.act.start_interval(timestamp);
        };

        bank.bankState = Bank::BankState::BANK_ACTIVE;
    }

    void DDR5::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
        if (bank.bankState == Bank::BankState::BANK_PRECHARGED)
            return;

        bank.counter.pre++;
        bank.cycles.act.close_interval(timestamp);
        bank.latestPre = timestamp;
        bank.bankState = Bank::BankState::BANK_PRECHARGED;

        if ( !rank.isActive(timestamp) ) {
            rank.cycles.act.close_interval(timestamp);
        }
    }

    void DDR5::handlePreSameBank(Rank & rank, std::size_t bank_id, timestamp_t timestamp) {
        auto bank_id_inside_bg = bank_id % this->memSpec.banksPerGroup;
        for(auto bank_group = 0; bank_group < this->memSpec.numberOfBankGroups; bank_group++) {
            auto & bank = rank.banks[bank_group * this->memSpec.banksPerGroup + bank_id_inside_bg];
            handlePre(rank, bank, timestamp);
        }
    }

    void DDR5::handlePreAll(Rank &rank, timestamp_t timestamp) {
        for (auto &bank: rank.banks) {
            handlePre(rank, bank, timestamp);
        }
    }

    void DDR5::handleRefSameBank(Rank & rank, std::size_t bank_id, timestamp_t timestamp) {
        auto bank_id_inside_bg = bank_id % this->memSpec.banksPerGroup;
        for(auto bank_group = 0; bank_group < this->memSpec.numberOfBankGroups; bank_group++) {
            auto & bank = rank.banks[bank_group * this->memSpec.banksPerGroup + bank_id_inside_bg];
            handleRefreshOnBank(rank, bank, timestamp, memSpec.memTimingSpec.tRFCsb, bank.counter.refSameBank);
        }
    }

    void DDR5::handleRefAll(Rank &rank, timestamp_t timestamp) {
        for (auto& bank : rank.banks) {
            handleRefreshOnBank(rank, bank, timestamp, memSpec.memTimingSpec.tRFC, bank.counter.refAllBank);
        }

        rank.endRefreshTime = timestamp + memSpec.memTimingSpec.tRFC;
    }

    void DDR5::handleRefreshOnBank(Rank & rank, Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t & counter){
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

    void DDR5::handleRead(Rank &rank, Bank &bank, timestamp_t timestamp){
        ++bank.counter.reads;
    }

    void DDR5::handleReadAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.readAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
        auto minReadActiveTime = timestamp + this->memSpec.prechargeOffsetRD;

        auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void DDR5::handleWrite(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.writes;
    }

    void DDR5::handleWriteAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.writeAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
        auto minWriteActiveTime = timestamp + this->memSpec.prechargeOffsetWR;

        auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void DDR5::handleSelfRefreshEntry(Rank &rank, timestamp_t timestamp) {
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

    void DDR5::handleSelfRefreshExit(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::SREF);
        rank.cycles.sref.close_interval(timestamp);  // Duration between entry and exit
        rank.memState = MemState::NOT_IN_PD;
    }

    void DDR5::handlePowerDownActEntry(Rank &rank, timestamp_t timestamp) {
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

    void DDR5::handlePowerDownActExit(Rank &rank, timestamp_t timestamp) {
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

    void DDR5::handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp) {
        auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
        auto entryTime = std::max(timestamp, earliestPossibleEntry);

        addImplicitCommand(entryTime, [this, &rank, entryTime]() {
            rank.cycles.powerDownPre.start_interval(entryTime);
            rank.memState = MemState::PDN_PRE;
        });
    }

    void DDR5::handlePowerDownPreExit(Rank &rank, timestamp_t timestamp) {
        // TODO: Is this computation necessary?
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [this, &rank, exitTime]() {
            rank.memState = MemState::NOT_IN_PD;
            rank.cycles.powerDownPre.close_interval(exitTime);
        });
    }

    void DDR5::endOfSimulation(timestamp_t timestamp) {
        if (this->implicitCommandCount() > 0)
            std::cout << ("[WARN] End of simulation but still implicit commands left!");
    }

    energy_t DDR5::calcEnergy(timestamp_t timestamp) {
        Calculation_DDR5 calculation;

        return calculation.calcEnergy(timestamp, *this);
    }

    interface_energy_info_t DDR5::calcInterfaceEnergy(timestamp_t timestamp) {
		return {};
    }

    SimulationStats DDR5::getWindowStats(timestamp_t timestamp) {
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
        stats.total.cycles.deepSleepMode = rank.cycles.deepSleepMode.get_count_at(timestamp);
        stats.total.cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp);

        stats.commandBus = this->commandBus.get_stats(timestamp);

        return stats;
    }

    SimulationStats DDR5::getStats() {
        return getWindowStats(getLastCommandTime());
    }

}
