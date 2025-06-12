#include "DDR4.h"
#include "DRAMPower/Types.h"
#include "DRAMPower/util/pin.h"
#include "DRAMPower/util/pin_types.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/ddr4/core_calculation_DDR4.h>
#include <DRAMPower/standards/ddr4/interface_calculation_DDR4.h>
#include <iostream>
#include <DRAMPower/util/extensions.h>
#include <optional>

namespace DRAMPower {

    using namespace DRAMUtils::Config;

    DDR4::DDR4(const MemSpecDDR4 &memSpec)
        : dram_base<CmdType>({
            {pattern_descriptor::V, PatternEncoderBitSpec::H},
            {pattern_descriptor::X, PatternEncoderBitSpec::H},
          })
        , memSpec(memSpec)
        , ranks(memSpec.numberOfRanks, {(std::size_t)memSpec.numberOfBanks})
        , cmdBusWidth(27)
        , cmdBusInitPattern((1<<cmdBusWidth)-1)
        , readDQS_(2, true)
        , writeDQS_(2, true)
        , commandBus(
            cmdBusWidth,
            1,
            util::BusIdlePatternSpec::H,
            util::BusInitPatternSpec::H
        )
        , prepostambleReadMinTccd(memSpec.prePostamble.readMinTccd)
        , prepostambleWriteMinTccd(memSpec.prePostamble.writeMinTccd)
        , dataBus(
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
        )
        , m_dbi(memSpec.numberOfDevices * memSpec.bitWidth, util::DBI::IdlePattern_t::H, 8,
            [this](timestamp_t load_timestamp, timestamp_t chunk_timestamp, std::size_t pin, bool inversion_state, bool read) {
            this->handleDBIPinChange(load_timestamp, chunk_timestamp, pin, inversion_state, read);
        }, false)
        , dbiread(m_dbi.getChunksPerWidth(), util::Pin{m_dbi.getIdlePattern()})
        , dbiwrite(m_dbi.getChunksPerWidth(), util::Pin{m_dbi.getIdlePattern()})
    {
        this->registerCommands();
        this->registerExtensions();
    }

    void DDR4::handleDBIPinChange(const timestamp_t load_timestamp, timestamp_t chunk_timestamp, std::size_t pin, bool state, bool read) {
        assert(pin < dbiread.size() || pin < dbiwrite.size());
        auto updatePinCallback = [this, chunk_timestamp, pin, state, read](){
            if (read) {
                this->dbiread[pin].set(chunk_timestamp, state ? util::PinState::L : util::PinState::H, 1);
            } else {
                this->dbiwrite[pin].set(chunk_timestamp, state ? util::PinState::L : util::PinState::H, 1);
            }
        };

        if (chunk_timestamp > load_timestamp) {
            // Schedule the pin state change
            this->addImplicitCommand(chunk_timestamp / memSpec.dataRate, updatePinCallback);
        } else {
            // chunk_timestamp <= load_timestamp
            updatePinCallback();
        }
    }

    std::optional<const uint8_t *> DDR4::handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read) {
        if (0 == n_bits || !data || !m_dbi.isEnabled()) {
            // No DBI or no data to process
            return std::nullopt;
        }
        timestamp_t virtual_time = timestamp * memSpec.dataRate;
        // updateDBI calls the given callback to handle pin changes
        auto dbiResult = m_dbi.updateDBI(virtual_time, n_bits, data, read);
        if (!dbiResult) {
            // No data to return
            return std::nullopt;
        }
        // TODO Schedule reset
        // Return the inverted data
        return dbiResult;
    }

    void DDR4::registerExtensions() {
        using namespace pattern_descriptor;
        // DRAMPowerExtensionDBI
        this->extensionManager.registerExtension<extensions::DBI>([this]([[maybe_unused]] const timestamp_t timestamp, const bool enable) {
            // NOTE: Assumption: the enabling of the DBI does not interleave with previous data on the bus
            m_dbi.enable(enable);
        }, false);
    }


    void DDR4::registerCommands(){
        using namespace pattern_descriptor;
        // ACT
        this->registerBankHandler<CmdType::ACT>(&DDR4::handleAct);
        this->registerPattern<CmdType::ACT>({
            L, L, R16, R15, R14, BG0, BG1, BA0, BA1,
            V, V, V, R12, R17, R13, R11, R10, R0,
            R1, R2, R3, R4, R5, R6, R7, R8, R9
        });
        this->registerInterfaceMember<CmdType::ACT>(&DDR4::handleInterfaceCommandBus);
        // PRE
        this->registerBankHandler<CmdType::PRE>(&DDR4::handlePre);
        this->registerPattern<CmdType::PRE>({
            L, H, L, H, L, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, L, V,
            V, V, V, V, V, V, V, V, V
        });
        this->registerInterfaceMember<CmdType::PRE>(&DDR4::handleInterfaceCommandBus);
        // PREA
        this->registerRankHandler<CmdType::PREA>(&DDR4::handlePreAll);
        this->registerPattern<CmdType::PREA>({
            L, H, L, H, L, V, V, V, V,
            V, V, V, V, V, V, V, H, V,
            V, V, V, V, V, V, V, V, V
        });
        this->registerInterfaceMember<CmdType::PREA>(&DDR4::handleInterfaceCommandBus);
        // REFA
        this->registerRankHandler<CmdType::REFA>(&DDR4::handleRefAll);
        this->registerPattern<CmdType::REFA>({
            L, H, L, L, H, V, V, V, V,
            V, V, V, V, V, V, V, V, V,
            V, V, V, V, V, V, V, V, V
        });
        this->registerInterfaceMember<CmdType::REFA>(&DDR4::handleInterfaceCommandBus);
        // RD
        this->registerBankHandler<CmdType::RD>(&DDR4::handleRead);
        this->registerPattern<CmdType::RD>({
            L, H, H, L, H, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, L, C0,
            C1, C2, C3, C4, C5, C6, C7, C8, C9
        });
        this->routeInterfaceCommand<CmdType::RD>([this](const Command &command){this->handleInterfaceData(command, true);});
        // RDA
        this->registerBankHandler<CmdType::RDA>(&DDR4::handleReadAuto);
        this->registerPattern<CmdType::RDA>({
            L, H, H, L, H, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, H, C0,
            C1, C2, C3, C4, C5, C6, C7, C8, C9
        });
        this->routeInterfaceCommand<CmdType::RDA>([this](const Command &command){this->handleInterfaceData(command, true);});
        // WR
        this->registerBankHandler<CmdType::WR>(&DDR4::handleWrite);
        this->registerPattern<CmdType::WR>({
            L, H, H, L, L, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, L, C0,
            C1, C2, C3, C4, C5, C6, C7, C8, C9
        });
        this->routeInterfaceCommand<CmdType::WR>([this](const Command &command){this->handleInterfaceData(command, false);});
        // WRA
        this->registerBankHandler<CmdType::WRA>(&DDR4::handleWriteAuto);
        this->registerPattern<CmdType::WRA>({
            L, H, H, L, L, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, H, C0,
            C1, C2, C3, C4, C5, C6, C7, C8, C9
        });
        this->routeInterfaceCommand<CmdType::WRA>([this](const Command &command){this->handleInterfaceData(command, false);});
        // SREFEN
        this->registerRankHandler<CmdType::SREFEN>(&DDR4::handleSelfRefreshEntry);
        this->registerPattern<CmdType::SREFEN>({
            L, H, L, L, H, V, V, V, V,
            V, V, V, V, V, V, V, V, V,
            V, V, V, V, V, V, V, V, V
        });
        this->registerInterfaceMember<CmdType::SREFEN>(&DDR4::handleInterfaceCommandBus);
        // SREFEX
        this->registerRankHandler<CmdType::SREFEX>(&DDR4::handleSelfRefreshExit);
        this->registerPattern<CmdType::SREFEX>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,

            L, H, H, H, H, V, V, V, V,
            V, V, V, V, V, V, V, V, V,
            V, V, V, V, V, V, V, V, V
        });
        this->registerInterfaceMember<CmdType::SREFEX>(&DDR4::handleInterfaceCommandBus);
        // PDEA
        this->registerRankHandler<CmdType::PDEA>(&DDR4::handlePowerDownActEntry);
        this->registerPattern<CmdType::PDEA>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
        this->registerInterfaceMember<CmdType::PDEA>(&DDR4::handleInterfaceCommandBus);
        // PDEP
        this->registerRankHandler<CmdType::PDEP>(&DDR4::handlePowerDownPreEntry);
        this->registerPattern<CmdType::PDEP>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
        this->registerInterfaceMember<CmdType::PDEP>(&DDR4::handleInterfaceCommandBus);
        // PDXA
        this->registerRankHandler<CmdType::PDXA>(&DDR4::handlePowerDownActExit);
        this->registerPattern<CmdType::PDXA>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
        this->registerInterfaceMember<CmdType::PDXA>(&DDR4::handleInterfaceCommandBus);
        // PDXP
        this->registerRankHandler<CmdType::PDXP>(&DDR4::handlePowerDownPreExit);
        this->registerPattern<CmdType::PDXP>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
        this->registerInterfaceMember<CmdType::PDXP>(&DDR4::handleInterfaceCommandBus);
        // EOS
        routeCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });

    }

// Getters for CLI
    uint64_t DDR4::getBankCount() {
        return memSpec.numberOfBanks;
    }

    uint64_t DDR4::getRankCount() {
        return memSpec.numberOfRanks;
    }

    uint64_t DDR4::getDeviceCount() {
        return memSpec.numberOfDevices;
    }

// Update toggling rate
void DDR4::enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp) {
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

void DDR4::enableBus(timestamp_t timestamp, timestamp_t enable_timestamp) {
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

timestamp_t DDR4::update_toggling_rate(timestamp_t timestamp, const std::optional<ToggleRateDefinition> &toggleratedefinition)
{
    if (toggleratedefinition) {
        dataBus.setTogglingRateDefinition(*toggleratedefinition);
        if (dataBus.isTogglingRate()) {
            // toggling rate already enabled
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
    // Init pattern for command bus and pattern encoder
    uint64_t DDR4::getInitEncoderPattern()
    {
        return this->cmdBusInitPattern;
    }

    void DDR4::handlePrePostamble(
        const timestamp_t   timestamp,
        const uint64_t      length,
        Rank                &rank,
        bool                read
    )
    {
        // TODO: If simulation finishes in read/write transaction the postamble needs to be substracted
        // uint64_t minTccd = prepostambleWriteMinTccd; // Todo use minTccd
        uint64_t *lastAccess = &rank.lastWriteEnd;
        uint64_t diff = 0;

        if(read)
        {
            lastAccess = &rank.lastReadEnd;
            // minTccd = prepostambleReadMinTccd;
        }
        
        assert(timestamp >= *lastAccess);
        if(timestamp < *lastAccess)
        {
            std::cout << "[Error] PrePostamble diff is negative. The last read/write transaction was not completed" << std::endl;
            return;
        }
        diff = timestamp - *lastAccess;
        *lastAccess = timestamp + length;
        
        //assert(diff >= 0);
        
        // Pre and Postamble seamless
        if(diff == 0)
        {
            // Seamless read or write
            if(read)
                rank.seamlessPrePostambleCounter_read++;
            else
                rank.seamlessPrePostambleCounter_write++;
        }
    }

    void DDR4::handleInterfaceOverrides(size_t length, bool read)
    {
        // Set command bus pattern overrides
        switch(length) {
            case 4:
                if(read)
                {
                    // Read
                    this->encoder.settings.updateSettings({
                        {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C1, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C0, PatternEncoderBitSpec::L},
                    });
                }
                else
                {
                    // Write
                    this->encoder.settings.updateSettings({
                        {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C1, PatternEncoderBitSpec::H},
                        {pattern_descriptor::C0, PatternEncoderBitSpec::H},
                    });
                }
                break;
            default:
                // Pull up
                // No interface power needed for PatternEncoderBitSpec::H
                // Defaults to burst length 8
            case 8:
                if(read)
                {
                    // Read
                    this->encoder.settings.updateSettings({
                        {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C1, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C0, PatternEncoderBitSpec::L},
                    });
                }
                else
                {
                    // Write
                    this->encoder.settings.updateSettings({
                        {pattern_descriptor::C2, PatternEncoderBitSpec::H},
                        {pattern_descriptor::C1, PatternEncoderBitSpec::H},
                        {pattern_descriptor::C0, PatternEncoderBitSpec::H},
                    });
                }
                break;
        }
    }

    void DDR4::handleInterfaceCommandBus(const Command &cmd) {
        auto pattern = this->getCommandPattern(cmd);
        auto ca_length = this->getPattern(cmd.type).size() / commandBus.get_width();
        this->commandBus.load(cmd.timestamp, pattern, ca_length);
    }

    void DDR4::handleInterfaceDQs(const Command &cmd, util::Clock &dqs, const size_t length) {
        dqs.start(cmd.timestamp);
        dqs.stop(cmd.timestamp + length / memSpec.dataRate);
    }

    void DDR4::handleInterfaceData(const Command &cmd, bool read) {
        auto loadfunc = read ? &databus_t::loadRead : &databus_t::loadWrite;
        util::Clock &dqs = read ? readDQS_ : writeDQS_;
        size_t length = 0;
        if (0 == cmd.sz_bits) {
            // No data provided by command
            // Use default burst length
            if (dataBus.isTogglingRate()) {
                // If bus is enabled skip loading data
                length = memSpec.burstLength;
                (dataBus.*loadfunc)(cmd.timestamp, length * dataBus.getWidth(), cmd.data);
            }
        } else {
            std::optional<const uint8_t *> dbi_data = std::nullopt;
            // Data provided by command
            if (dataBus.isBus() && m_dbi.isEnabled()) {
                // Only compute dbi for bus mode
                dbi_data = handleDBIInterface(cmd.timestamp, cmd.sz_bits, cmd.data, read);
            }
            length = cmd.sz_bits / (dataBus.getWidth());
            (dataBus.*loadfunc)(cmd.timestamp, cmd.sz_bits, dbi_data.value_or(cmd.data));
        }
        assert(this->ranks.size()>cmd.targetCoordinate.rank);
        auto & rank = this->ranks[cmd.targetCoordinate.rank];
        handleInterfaceDQs(cmd, dqs, length);
        handlePrePostamble(cmd.timestamp, length / memSpec.dataRate, rank, read);
        handleInterfaceOverrides(length, read);
        handleInterfaceCommandBus(cmd);
    }

// Core
    void DDR4::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
        bank.counter.act++;
        bank.bankState = Bank::BankState::BANK_ACTIVE;

        bank.cycles.act.start_interval(timestamp);
        
        rank.cycles.act.start_interval_if_not_running(timestamp);
        //rank.cycles.pre.close_interval(timestamp);
    }

    void DDR4::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
        // If statement necessary for core power calculation
        // bank.counter.pre doesn't correspond to the number of pre commands
        // It corresponds to the number of state transisitons to the pre state
        if (bank.bankState == Bank::BankState::BANK_PRECHARGED) return;
        bank.counter.pre++;

        bank.bankState = Bank::BankState::BANK_PRECHARGED;
        bank.cycles.act.close_interval(timestamp);
        bank.latestPre = timestamp;                                                // used for earliest power down calculation
        if ( !rank.isActive(timestamp) )                                            // stop rank active interval if no more banks active
        {
            // active counter increased if at least 1 bank is active, precharge counter increased if all banks are precharged
            rank.cycles.act.close_interval(timestamp);
            //rank.cycles.pre.start_interval_if_not_running(timestamp);             
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
        //rank.cycles.pre.close_interval(timestamp);
        // TODO loop over all banks and implicit commands for all banks can be simplified
        for (auto& bank : rank.banks) {
            bank.bankState = Bank::BankState::BANK_ACTIVE;
            
            ++bank.counter.refAllBank;
            bank.cycles.act.start_interval_if_not_running(timestamp);


            bank.refreshEndTime = timestamp_end;                                    // used for earliest power down calculation

            // Execute implicit pre-charge at refresh end
            addImplicitCommand(timestamp_end, [&bank, &rank, timestamp_end]() {
                bank.bankState = Bank::BankState::BANK_PRECHARGED;
                bank.cycles.act.close_interval(timestamp_end);
                // stop rank active interval if no more banks active
                if (!rank.isActive(timestamp_end))                                  // stop rank active interval if no more banks active
                {
                    rank.cycles.act.close_interval(timestamp_end);
                    //rank.cycles.pre.start_interval(timestamp_end);
                }
            });
        }

        // Required for precharge power-down
    }

    void DDR4::handleRead(Rank&, Bank &bank, timestamp_t) {
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

    void DDR4::handleWrite(Rank&, Bank &bank, timestamp_t) {
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
        addImplicitCommand(timestampSelfRefreshStart, [&rank, timestampSelfRefreshStart]() {
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
        addImplicitCommand(entryTime, [&rank, entryTime]() {
            rank.memState = MemState::PDN_ACT;
            rank.cycles.powerDownAct.start_interval(entryTime);
            rank.cycles.act.close_interval(entryTime);
            //rank.cycles.pre.close_interval(entryTime);
            for (auto & bank : rank.banks) {
                bank.cycles.act.close_interval(entryTime);
            }
        });
    }

    void DDR4::handlePowerDownActExit(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::PDN_ACT);
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [&rank, exitTime]() {
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
            // At least one bank must be active for PDA -> remove if statement?
            if(rank.isActive(exitTime))
                rank.cycles.act.start_interval(exitTime); 

        });
    }

    void DDR4::handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp) {
        auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
        auto entryTime = std::max(timestamp, earliestPossibleEntry);

        addImplicitCommand(entryTime, [&rank, entryTime]() {
            for (auto &bank : rank.banks)
                bank.cycles.act.close_interval(entryTime);
            rank.memState = MemState::PDN_PRE;
            rank.cycles.powerDownPre.start_interval(entryTime);
            //rank.cycles.pre.close_interval(entryTime);
            rank.cycles.act.close_interval(entryTime);
        });
    }

    void DDR4::handlePowerDownPreExit(Rank &rank, timestamp_t timestamp) {
        // The computation is necessary to exit at the earliest timestamp (upon entry)
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [&rank, exitTime]() {
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
            // At least one bank must be precharged for PDP -> remove if statement?
            // If statement ensures right state diagramm traversal
            //if(!rank.isActive(exitTime))
            //    rank.cycles.pre.start_interval(exitTime); 
        });
    }

    void DDR4::endOfSimulation(timestamp_t) {
        assert(this->implicitCommandCount() == 0);
	}

// Calculation
    energy_t DDR4::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_DDR4 calculation;
        return calculation.calcEnergy(timestamp, *this);
    }

    interface_energy_info_t DDR4::calcInterfaceEnergy(timestamp_t timestamp) {
        // TODO add tests
        InterfaceCalculation_DDR4 calculation(memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

// Stats
    SimulationStats DDR4::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        processImplicitCommandQueue(timestamp);

        // Reset the DBI interface pins to idle state
        m_dbi.dispatchResetCallback(timestamp * memSpec.dataRate, false);
        m_dbi.dispatchResetCallback(timestamp * memSpec.dataRate, true);

        // DDR4 x16 have 2 DQs differential pairs
        uint_fast8_t NumDQsPairs = 1;
        if(memSpec.bitWidth == 16)
            NumDQsPairs = 2;

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
            stats.rank_total[i].cycles.pre = simulation_duration - 
            (
                stats.rank_total[i].cycles.act +
                stats.rank_total[i].cycles.powerDownAct +
                stats.rank_total[i].cycles.powerDownPre +
                stats.rank_total[i].cycles.selfRefresh
            );
            //stats.rank_total[i].cycles.pre = rank.cycles.pre.get_count_at(timestamp);

            stats.rank_total[i].prepos.readSeamless = rank.seamlessPrePostambleCounter_read;
            stats.rank_total[i].prepos.writeSeamless = rank.seamlessPrePostambleCounter_write;
            
            stats.rank_total[i].prepos.readMerged = rank.mergedPrePostambleCounter_read;
            stats.rank_total[i].prepos.readMergedTime = rank.mergedPrePostambleTime_read;
            stats.rank_total[i].prepos.writeMerged = rank.mergedPrePostambleCounter_write;
            stats.rank_total[i].prepos.writeMergedTime = rank.mergedPrePostambleTime_write;
        }
        stats.commandBus = commandBus.get_stats(timestamp);

        dataBus.get_stats(timestamp,
            stats.readBus,
            stats.writeBus,
            stats.togglingStats.read,
            stats.togglingStats.write
        );

        // single line stored in stats
        // differential power calculated in interface calculation
        stats.clockStats = 2u * clock.get_stats_at(timestamp);
        stats.readDQSStats = NumDQsPairs * 2u * readDQS_.get_stats_at(timestamp);
        stats.writeDQSStats = NumDQsPairs * 2u * writeDQS_.get_stats_at(timestamp);
        for (const auto &dbi_pin : dbiread) {
            stats.readDBI += dbi_pin.get_stats_at(timestamp, 2);
        }
        for (const auto &dbi_pin : dbiwrite) {
            stats.writeDBI += dbi_pin.get_stats_at(timestamp, 2);
        }

        return stats;
    }

    SimulationStats DDR4::getStats() {
        return getWindowStats(getLastCommandTime());
    }

}
