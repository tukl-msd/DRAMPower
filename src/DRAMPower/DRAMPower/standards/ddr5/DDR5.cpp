#include "DRAMPower/standards/ddr5/DDR5.h"

#include <algorithm>
#include <iostream>

#include "DRAMPower/command/Pattern.h"
#include "DRAMPower/standards/ddr5/core_calculation_DDR5.h"
#include "DRAMPower/standards/ddr5/interface_calculation_DDR5.h"

namespace DRAMPower {

    DDR5::DDR5(const MemSpecDDR5 &memSpec)
        : memSpec(memSpec),
        ranks(memSpec.numberOfRanks, {(std::size_t)memSpec.numberOfBanks}),
        cmdBusWidth(14),
        cmdBusInitPattern((1<<cmdBusWidth)-1),
        commandBus(
            cmdBusWidth, 1,
            util::Bus::BusIdlePatternSpec::H,
            util::Bus::burst_t(cmdBusWidth, cmdBusInitPattern)
        ),
        readBus{memSpec.bitWidth * memSpec.numberOfDevices, memSpec.dataRate,
            util::Bus::BusIdlePatternSpec::H, util::Bus::BusInitPatternSpec::H},
        writeBus{memSpec.bitWidth * memSpec.numberOfDevices, memSpec.dataRate,
            util::Bus::BusIdlePatternSpec::H, util::Bus::BusInitPatternSpec::H},
        readDQS_t_(memSpec.dataRateSpec.dqsBusRate, true),
        readDQS_c_(memSpec.dataRateSpec.dqsBusRate, true),
        writeDQS_t_(memSpec.dataRateSpec.dqsBusRate, true),
        writeDQS_c_(memSpec.dataRateSpec.dqsBusRate, true),
        dram_base<CmdType>(PatternEncoderOverrides{
            {pattern_descriptor::V, PatternEncoderBitSpec::H},
            {pattern_descriptor::X, PatternEncoderBitSpec::H}, // TODO high impedance ???
            // Default value for CID0-3 is H in Pattern.h
            // {pattern_descriptor::CID0, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID1, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID2, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID3, PatternEncoderBitSpec::H},
        })
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

        routeCommand<CmdType::END_OF_SIMULATION>(
            [this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
    };

    uint64_t DDR5::getInitEncoderPattern()
    {
        return this->cmdBusInitPattern;
    }

    void DDR5::registerPatterns() {
        using namespace pattern_descriptor;

        // ---------------------------------:
        this->registerPattern<CmdType::ACT>({
            // note: CID3 is mutually exclusive with R17 and depends on usage mode
            L, L, R0, R1, R2, R3, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16, CID3
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
        this->registerPattern<CmdType::REFSB>({
            H, H, L, L, H, CID3, BA0, BA1, V, V, H, CID0, CID1, CID2
        });
        this->registerPattern<CmdType::REFA>({
            H, H, L, L, H, CID3, V, V, V, V, L, CID0, CID1, CID2
                                             });
        this->registerPattern<CmdType::RD>({
            H, L, H, H, H, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            C2, C3, C4, C5, C6, C7, C8, C9, C10, V, H, V, V, CID3
        });
        this->registerPattern<CmdType::RDA>({
            H, L, H, H, H, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            C2, C3, C4, C5, C6, C7, C8, C9, C10, V, L, V, V, CID3
        });
        this->registerPattern<CmdType::WR>({
            H, L, H, H, L, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            V, C3, C4, C5, C6, C7, C8, C9, C10, V, H, H, V, CID3
        });
        this->registerPattern<CmdType::WRA>({
            H, L, H, H, L, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            V, C3, C4, C5, C6, C7, C8, C9, C10, V, L, H, V, CID3
        });
        this->registerPattern<CmdType::SREFEN>({
            H, H, H, L, H, V, V, V, V, H, L, V, V, V
        });

        // Power-down mode is different in DDR5. There is no distinct PDEA and PDEP but instead it depends on
        // bank state upon command issue. Check standard
        this->registerPattern<CmdType::PDEA>({
            H, H, H, L, H, V, V, V, V, V, H, L, V, V
        });
        this->registerPattern<CmdType::PDXA>({
            H, H, H, H, H, V, V, V, V, V, V, V, V, V
        });
        this->registerPattern<CmdType::PDEP>({
            H, H, H, L, H, V, V, V, V, V, H, L, V, V
        });
        this->registerPattern<CmdType::PDXP>({
            H, H, H, H, H, V, V, V, V, V, V, V, V, V
        });
    }

    void DDR5::handle_interface(const Command &cmd) {
        auto pattern = getCommandPattern(cmd);
        auto ca_length = getPattern(cmd.type).size() / commandBus.get_width();
        commandBus.load(cmd.timestamp, pattern, ca_length);

        size_t length = 0;
        switch (cmd.type) {
            case CmdType::RD:
            case CmdType::RDA:
                length = cmd.sz_bits / readBus.get_width();
                readBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);

                readDQS_c_.start(cmd.timestamp);
                readDQS_c_.stop(cmd.timestamp + length / this->memSpec.dataRateSpec.dqsBusRate);

                readDQS_t_.start(cmd.timestamp);
                readDQS_t_.stop(cmd.timestamp + length / this->memSpec.dataRateSpec.dqsBusRate);
                break;
            case CmdType::WR:
            case CmdType::WRA:
                length = cmd.sz_bits / writeBus.get_width();
                writeBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);

                writeDQS_c_.start(cmd.timestamp);
                writeDQS_c_.stop(cmd.timestamp + length / this->memSpec.dataRateSpec.dqsBusRate);

                writeDQS_t_.start(cmd.timestamp);
                writeDQS_t_.stop(cmd.timestamp + length / this->memSpec.dataRateSpec.dqsBusRate);
                break;
        };
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
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [this, &rank, exitTime]() {
            rank.memState = MemState::NOT_IN_PD;
            rank.cycles.powerDownPre.close_interval(exitTime);
        });
    }

    void DDR5::endOfSimulation(timestamp_t timestamp) {
        if (this->implicitCommandCount() > 0)
            std::cout << ("[WARN] End of simulation but still implicit commands left!") << std::endl;
    }

    energy_t DDR5::calcEnergy(timestamp_t timestamp) {
        Calculation_DDR5 calculation;

        return calculation.calcEnergy(timestamp, *this);
    }

    interface_energy_info_t DDR5::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_DDR5 calculation(memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

    SimulationStats DDR5::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        processImplicitCommandQueue(timestamp);

        SimulationStats stats;
        stats.bank.resize(memSpec.numberOfBanks * memSpec.numberOfRanks);
        stats.rank_total.resize(memSpec.numberOfRanks);

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
            stats.rank_total[i].cycles.deepSleepMode =
                rank.cycles.deepSleepMode.get_count_at(timestamp);
            stats.rank_total[i].cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp);
        }

        stats.commandBus = commandBus.get_stats(timestamp);
        stats.readBus = readBus.get_stats(timestamp);
        stats.writeBus = writeBus.get_stats(timestamp);

        stats.clockStats = CK_t_.get_stats_at(timestamp) + CK_c_.get_stats_at(timestamp);
        stats.readDQSStats = readDQS_c_.get_stats_at(timestamp) + readDQS_t_.get_stats_at(timestamp);
        stats.writeDQSStats = writeDQS_c_.get_stats_at(timestamp) + writeDQS_t_.get_stats_at(timestamp);

        return stats;
    }

    SimulationStats DDR5::getStats() {
        return getWindowStats(getLastCommandTime());
    }

    timestamp_t DDR5::earliestPossiblePowerDownEntryTime(Rank &rank) {
        timestamp_t entryTime = 0;

        for (const auto &bank : rank.banks) {
            entryTime = std::max(
                {entryTime,
                 bank.counter.act == 0 ? 0
                                       : bank.cycles.act.get_start() + memSpec.memTimingSpec.tRCD,
                 bank.counter.pre == 0 ? 0 : bank.latestPre + memSpec.memTimingSpec.tRP,
                 bank.refreshEndTime});
        }

        return entryTime;
    };
}
