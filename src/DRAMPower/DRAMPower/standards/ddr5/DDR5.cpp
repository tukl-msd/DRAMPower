#include "DRAMPower/standards/ddr5/DDR5.h"

#include <algorithm>
#include <iostream>

#include "DRAMPower/command/Pattern.h"
#include "DRAMPower/standards/ddr5/core_calculation_DDR5.h"
#include "DRAMPower/standards/ddr5/interface_calculation_DDR5.h"

namespace DRAMPower {

    using namespace DRAMUtils::Config;

    DDR5::DDR5(const MemSpecDDR5 &memSpec)
        : dram_base<CmdType>(PatternEncoderOverrides{
            {pattern_descriptor::V, PatternEncoderBitSpec::H},
            {pattern_descriptor::X, PatternEncoderBitSpec::H}, // TODO high impedance ???
            {pattern_descriptor::C0, PatternEncoderBitSpec::H},
            {pattern_descriptor::C1, PatternEncoderBitSpec::H},
            // Default value for CID0-3 is H in Pattern.h
            // {pattern_descriptor::CID0, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID1, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID2, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID3, PatternEncoderBitSpec::H},
          })
        , memSpec(memSpec)
        , ranks(memSpec.numberOfRanks, {(std::size_t)memSpec.numberOfBanks})
        , dataBus{
            util::databus_presets::getDataBusPreset(
                memSpec.bitWidth * memSpec.numberOfDevices,
                util::DataBusConfig {
                    memSpec.bitWidth * memSpec.numberOfDevices,
                    memSpec.dataRate,
                    util::BusIdlePatternSpec::H,
                    util::BusInitPatternSpec::H,
                    DRAMUtils::Config::TogglingRateIdlePattern::H,
                    0.0,
                    0.0,
                    util::DataBusMode::Bus
                }
            )
        }
        , cmdBusWidth(14)
        , cmdBusInitPattern((1<<cmdBusWidth)-1)
        , commandBus(
            cmdBusWidth,
            1,
            util::BusIdlePatternSpec::H,
            util::BusInitPatternSpec::H
        )
        , readDQS(memSpec.dataRateSpec.dqsBusRate, true)
        , writeDQS(memSpec.dataRateSpec.dqsBusRate, true)
    {
        this->registerCommands();
    }

    void DDR5::registerCommands() {
        using namespace pattern_descriptor;
        // ACT
        this->registerBankHandler<CmdType::ACT>(&DDR5::handleAct);
        this->registerPattern<CmdType::ACT>({
            // note: CID3 is mutually exclusive with R17 and depends on usage mode
            L, L, R0, R1, R2, R3, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16, CID3
        });
        this->registerInterfaceMember<CmdType::ACT>(&DDR5::handleInterfaceCommandBus);
        // PRE
        this->registerBankHandler<CmdType::PRE>(&DDR5::handlePre);
        this->registerPattern<CmdType::PRE>({
            H, H, L, H, H, CID3, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2
        });
        this->registerInterfaceMember<CmdType::PRE>(&DDR5::handleInterfaceCommandBus);
        // RD
        this->registerBankHandler<CmdType::RD>(&DDR5::handleRead);
        this->registerPattern<CmdType::RD>({
            H, L, H, H, H, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            C2, C3, C4, C5, C6, C7, C8, C9, C10, V, H, V, V, CID3
        });
        this->routeInterfaceCommand<CmdType::RD>([this](const Command &command){this->handleInterfaceData(command, true);});
        // RDA
        this->registerBankHandler<CmdType::RDA>(&DDR5::handleReadAuto);
        this->registerPattern<CmdType::RDA>({
            H, L, H, H, H, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            C2, C3, C4, C5, C6, C7, C8, C9, C10, V, L, V, V, CID3
        });
        this->routeInterfaceCommand<CmdType::RDA>([this](const Command &command){this->handleInterfaceData(command, true);});
        // WR
        this->registerBankHandler<CmdType::WR>(&DDR5::handleWrite);
        this->registerPattern<CmdType::WR>({
            H, L, H, H, L, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            V, C3, C4, C5, C6, C7, C8, C9, C10, V, H, H, V, CID3
        });
        this->routeInterfaceCommand<CmdType::WR>([this](const Command &command){this->handleInterfaceData(command, false);});
        // WRA
        this->registerBankHandler<CmdType::WRA>(&DDR5::handleWriteAuto);
        this->registerPattern<CmdType::WRA>({
            H, L, H, H, L, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
            V, C3, C4, C5, C6, C7, C8, C9, C10, V, L, H, V, CID3
        });
        this->routeInterfaceCommand<CmdType::WRA>([this](const Command &command){this->handleInterfaceData(command, false);});
        // PRESB
        this->registerBankGroupHandler<CmdType::PRESB>(&DDR5::handlePreSameBank);
        this->registerPattern<CmdType::PRESB>({
            H, H, L, H, L, CID3, BA0, BA1, V, V, H, CID0, CID1, CID2
        });
        this->registerInterfaceMember<CmdType::PRESB>(&DDR5::handleInterfaceCommandBus);
        // REFSB
        this->registerBankGroupHandler<CmdType::REFSB>(&DDR5::handleRefSameBank);
        this->registerPattern<CmdType::REFSB>({
            H, H, L, L, H, CID3, BA0, BA1, V, V, H, CID0, CID1, CID2
        });
        this->registerInterfaceMember<CmdType::REFSB>(&DDR5::handleInterfaceCommandBus);
        // REFA
        this->registerRankHandler<CmdType::REFA>(&DDR5::handleRefAll);
        this->registerPattern<CmdType::REFA>({
            H, H, L, L, H, CID3, V, V, V, V, L, CID0, CID1, CID2
        });
        this->registerInterfaceMember<CmdType::REFA>(&DDR5::handleInterfaceCommandBus);
        // PREA
        this->registerRankHandler<CmdType::PREA>(&DDR5::handlePreAll);
        this->registerPattern<CmdType::PREA>({
            H, H, L, H, L, CID3, V, V, V, V, L, CID0, CID1, CID2
        });
        this->registerInterfaceMember<CmdType::PREA>(&DDR5::handleInterfaceCommandBus);
        // SREFEN
        this->registerRankHandler<CmdType::SREFEN>(&DDR5::handleSelfRefreshEntry);
        this->registerPattern<CmdType::SREFEN>({
            H, H, H, L, H, V, V, V, V, H, L, V, V, V
        });
        this->registerInterfaceMember<CmdType::SREFEN>(&DDR5::handleInterfaceCommandBus);
        // SREFEX
        this->registerRankHandler<CmdType::SREFEX>(&DDR5::handleSelfRefreshExit);
        // PDEA
        this->registerRankHandler<CmdType::PDEA>(&DDR5::handlePowerDownActEntry);
        this->registerPattern<CmdType::PDEA>({
            H, H, H, L, H, V, V, V, V, V, H, L, V, V
        });
        this->registerInterfaceMember<CmdType::PDEA>(&DDR5::handleInterfaceCommandBus);
        // PDEP
        this->registerRankHandler<CmdType::PDEP>(&DDR5::handlePowerDownPreEntry);
        this->registerPattern<CmdType::PDEP>({
            H, H, H, L, H, V, V, V, V, V, H, L, V, V
        });
        this->registerInterfaceMember<CmdType::PDEP>(&DDR5::handleInterfaceCommandBus);
        // PDXA
        this->registerRankHandler<CmdType::PDXA>(&DDR5::handlePowerDownActExit);
        this->registerPattern<CmdType::PDXA>({
            H, H, H, H, H, V, V, V, V, V, V, V, V, V
        });
        this->registerInterfaceMember<CmdType::PDXA>(&DDR5::handleInterfaceCommandBus);
        // PDXP
        this->registerRankHandler<CmdType::PDXP>(&DDR5::handlePowerDownPreExit);
        this->registerPattern<CmdType::PDXP>({
            H, H, H, H, H, V, V, V, V, V, V, V, V, V
        });
        this->registerInterfaceMember<CmdType::PDXP>(&DDR5::handleInterfaceCommandBus);
        // NOP
        this->registerPattern<CmdType::NOP>({
            H, H, H, H, H, V, V, V, V, V, V, V, V, V
        });
        this->registerInterfaceMember<CmdType::NOP>(&DDR5::handleInterfaceCommandBus);
        // EOS
        routeCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
        // ---------------------------------:

        // Power-down mode is different in DDR5. There is no distinct PDEA and PDEP but instead it depends on
        // bank state upon command issue. Check standard
    }

// Getters for CLI
    uint64_t DDR5::getBankCount() {
        return memSpec.numberOfBanks;
    }

    uint64_t DDR5::getRankCount() {
        return memSpec.numberOfRanks;
    }

    uint64_t DDR5::getDeviceCount() {
        return memSpec.numberOfDevices;
    }

// Update toggling rate
    void DDR5::enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp) {
        // Change from bus to toggling rate
        assert(enable_timestamp >= timestamp);
        if ( enable_timestamp > timestamp ) {
            // Schedule toggling rate enable
            this->addImplicitCommand(enable_timestamp, [this, enable_timestamp]() {
                dataBus.enableTogglingRate(enable_timestamp);
            });
        } else {
            dataBus.enableTogglingRate(enable_timestamp);
        }
    }

    void DDR5::enableBus(timestamp_t timestamp, timestamp_t enable_timestamp) {
        // Change from toggling rate to bus
        assert(enable_timestamp >= timestamp);
        if ( enable_timestamp > timestamp ) {
            // Schedule toggling rate disable
            this->addImplicitCommand(enable_timestamp, [this, enable_timestamp]() {
                dataBus.enableBus(enable_timestamp);
            });
        } else {
            dataBus.enableBus(enable_timestamp);
        }
    }

    timestamp_t DDR5::update_toggling_rate(timestamp_t timestamp, const std::optional<ToggleRateDefinition> &toggleratedefinition)
    {
        if (toggleratedefinition) {
            dataBus.setTogglingRateDefinition(*toggleratedefinition);
            if (dataBus.isTogglingRate()) {
                // toggling rate is already enabled
                return timestamp;
            }
            // Enable toggling rate
            timestamp_t enable_timestamp = std::max(timestamp, dataBus.lastBurst());
            enableTogglingHandle(timestamp, enable_timestamp);
            return enable_timestamp;
        } else {
            if (dataBus.isBus()) {
                // Bus already enabled
                return timestamp;
            }
            // Enable bus
            timestamp_t enable_timestamp = std::max(timestamp, dataBus.lastBurst());
            enableBus(timestamp, enable_timestamp);
            return enable_timestamp;
        }
        return timestamp;
    }

// Interface
    // Init pattern for command bus and patter encoder
    uint64_t DDR5::getInitEncoderPattern()
{
    return this->cmdBusInitPattern;
}

    void DDR5::handleInterfaceOverrides(size_t length, bool read)
    {
        // Set command bus pattern overrides
        switch(length) {
            case 8:
                this->encoder.settings.removeSetting(pattern_descriptor::C10);
                if(read)
                {
                    // Read
                    this->encoder.settings.updateSettings({
                        {pattern_descriptor::C3, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                    });
                }
                else
                {
                    // Write
                    this->encoder.settings.updateSettings({
                        {pattern_descriptor::C3, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C2, PatternEncoderBitSpec::H},
                    });
                }
                break;
            default:
                // Pull down
                // No interface power needed for PatternEncoderBitSpec::L
                // Defaults to burst length 16
            case 16:
                this->encoder.settings.removeSetting(pattern_descriptor::C10);
                if(read)
                {
                    // Read
                    this->encoder.settings.updateSettings({
                        {pattern_descriptor::C3, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                    });
                }
                else
                {
                    // Write
                    this->encoder.settings.updateSettings({
                        {pattern_descriptor::C3, PatternEncoderBitSpec::H},
                        {pattern_descriptor::C2, PatternEncoderBitSpec::H},
                    });
                }
                break;
            case 32:
                if(read)
                {
                    // Read
                    this->encoder.settings.updateSettings({
                        {pattern_descriptor::C10, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C3, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                    });
                }
                else
                {
                    // Write
                    this->encoder.settings.updateSettings({
                        {pattern_descriptor::C10, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C3, PatternEncoderBitSpec::H},
                        {pattern_descriptor::C2, PatternEncoderBitSpec::H},
                    });
                }
                break;
        }
    }

    void DDR5::handleInterfaceCommandBus(const Command& cmd) {
        auto pattern = this->getCommandPattern(cmd);
        auto ca_length = this->getPattern(cmd.type).size() / commandBus.get_width();
        this->commandBus.load(cmd.timestamp, pattern, ca_length);
    }

    void DDR5::handleInterfaceDQs(const Command& cmd, util::Clock &dqs, size_t length) {
        dqs.start(cmd.timestamp);
        dqs.stop(cmd.timestamp + length / memSpec.dataRate);
    }

    void DDR5::handleInterfaceData(const Command &cmd, bool read) {
        auto loadfunc = read ? &databus_t::loadRead : &databus_t::loadWrite;
        util::Clock &dqs = read ? readDQS : writeDQS;
        size_t length = 0;
        if (0 == cmd.sz_bits) {
            // No data provided by command
            // Use default burst length
            if (dataBus.isTogglingRate()) {
                length = memSpec.burstLength;
                (dataBus.*loadfunc)(cmd.timestamp, length * dataBus.getWidth(), nullptr);
            }
        } else {
            // Data provided by command
            length = cmd.sz_bits / dataBus.getWidth();
            (dataBus.*loadfunc)(cmd.timestamp, cmd.sz_bits, cmd.data);
        }
        handleInterfaceDQs(cmd, dqs, length);
        handleInterfaceOverrides(length, read);
        handleInterfaceCommandBus(cmd);
    }

// Core
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
        for(unsigned bank_group = 0; bank_group < this->memSpec.numberOfBankGroups; bank_group++) {
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
        for(unsigned bank_group = 0; bank_group < this->memSpec.numberOfBankGroups; bank_group++) {
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

    void DDR5::handleRead(Rank&, Bank &bank, timestamp_t){
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

    void DDR5::handleWrite(Rank&, Bank &bank, timestamp_t) {
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

    void DDR5::endOfSimulation(timestamp_t) {
        if (this->implicitCommandCount() > 0)
            std::cout << ("[WARN] End of simulation but still implicit commands left!") << std::endl;
    }

// Calculation
    energy_t DDR5::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_DDR5 calculation;

        return calculation.calcEnergy(timestamp, *this);
    }

    interface_energy_info_t DDR5::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_DDR5 calculation(memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

// Stats
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

        dataBus.get_stats(timestamp, 
            stats.readBus,
            stats.writeBus,
            stats.togglingStats.read,
            stats.togglingStats.write
        );

        stats.clockStats = 2 * clock.get_stats_at(timestamp);
        stats.readDQSStats = 2 * readDQS.get_stats_at(timestamp);
        stats.writeDQSStats = 2 * writeDQS.get_stats_at(timestamp);

        // x16 devices have two dqs pairs
        if(memSpec.bitWidth == 16)
        {
            stats.readDQSStats *= 2;
            stats.writeDQSStats *= 2;
        }

        return stats;
    }

    SimulationStats DDR5::getStats() {
        return getWindowStats(getLastCommandTime());
    }

// Core helpers
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
