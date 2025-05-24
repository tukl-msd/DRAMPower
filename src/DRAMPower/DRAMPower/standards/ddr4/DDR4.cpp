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
          }, DDR4Interface::cmdBusInitPattern)
        , memSpec(memSpec)
        , interface(memSpec, getImplicitCommandHandler().createInserter(), getPatternHandler())
        , core(memSpec, getImplicitCommandHandler().createInserter())
    {
        this->registerCommands();
        this->registerExtensions();
    }

    void DDR4Interface::handleDBIPinChange(const timestamp_t load_timestamp, timestamp_t chunk_timestamp, std::size_t pin, bool state, bool read) {
        assert(pin < m_dbiread.size() || pin < m_dbiwrite.size());
        auto updatePinCallback = [this, chunk_timestamp, pin, state, read](){
            if (read) {
                this->m_dbiread[pin].set(chunk_timestamp, state ? util::PinState::L : util::PinState::H, 1);
            } else {
                this->m_dbiwrite[pin].set(chunk_timestamp, state ? util::PinState::L : util::PinState::H, 1);
            }
        };

        if (chunk_timestamp > load_timestamp) {
            // Schedule the pin state change
            this->addImplicitCommand(chunk_timestamp / m_memSpec.dataRate, updatePinCallback);
        } else {
            // chunk_timestamp <= load_timestamp
            updatePinCallback();
        }
    }

    std::optional<const uint8_t *> DDR4Interface::handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read) {
        if (0 == n_bits || !data || !m_dbi.isEnabled()) {
            // No DBI or no data to process
            return std::nullopt;
        }
        timestamp_t virtual_time = timestamp * m_memSpec.dataRate;
        // updateDBI calls the given callback to handle pin changes
        auto dbiResult = m_dbi.updateDBI(virtual_time, n_bits, data, read);
        if (!dbiResult) {
            // No data to return
            return std::nullopt;
        }
        // Return the inverted data
        return dbiResult;
    }

    void DDR4::registerExtensions() {
        using namespace pattern_descriptor;
        // DRAMPowerExtensionDBI
        getExtensionManager().registerExtension<extensions::DBI>([this]([[maybe_unused]] const timestamp_t timestamp, const bool enable) {
            // NOTE: Assumption: the enabling of the DBI does not interleave with previous data on the bus
            interface.m_dbi.enable(enable);
        }, false);
    }


    void DDR4Interface::registerPatterns() {
        using namespace pattern_descriptor;
        m_patternHandler.registerPattern<CmdType::ACT>({
            L, L, R16, R15, R14, BG0, BG1, BA0, BA1,
            V, V, V, R12, R17, R13, R11, R10, R0,
            R1, R2, R3, R4, R5, R6, R7, R8, R9
        });
        // PRE
        m_patternHandler.registerPattern<CmdType::PRE>({
            L, H, L, H, L, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, L, V,
            V, V, V, V, V, V, V, V, V
        });
        // PREA
        m_patternHandler.registerPattern<CmdType::PREA>({
            L, H, L, H, L, V, V, V, V,
            V, V, V, V, V, V, V, H, V,
            V, V, V, V, V, V, V, V, V
        });
        // REFA
        m_patternHandler.registerPattern<CmdType::REFA>({
            L, H, L, L, H, V, V, V, V,
            V, V, V, V, V, V, V, V, V,
            V, V, V, V, V, V, V, V, V
        });
        // RD
        m_patternHandler.registerPattern<CmdType::RD>({
            L, H, H, L, H, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, L, C0,
            C1, C2, C3, C4, C5, C6, C7, C8, C9
        });
        // RDA
        m_patternHandler.registerPattern<CmdType::RDA>({
            L, H, H, L, H, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, H, C0,
            C1, C2, C3, C4, C5, C6, C7, C8, C9
        });
        // WR
        m_patternHandler.registerPattern<CmdType::WR>({
            L, H, H, L, L, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, L, C0,
            C1, C2, C3, C4, C5, C6, C7, C8, C9
        });
        // WRA
        m_patternHandler.registerPattern<CmdType::WRA>({
            L, H, H, L, L, BG0, BG1, BA0, BA1,
            V, V, V, V, V, V, V, H, C0,
            C1, C2, C3, C4, C5, C6, C7, C8, C9
        });
        // SREFEN
        m_patternHandler.registerPattern<CmdType::SREFEN>({
            L, H, L, L, H, V, V, V, V,
            V, V, V, V, V, V, V, V, V,
            V, V, V, V, V, V, V, V, V
        });
        // SREFEX
        m_patternHandler.registerPattern<CmdType::SREFEX>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,

            L, H, H, H, H, V, V, V, V,
            V, V, V, V, V, V, V, V, V,
            V, V, V, V, V, V, V, V, V
        });
        // PDEA
        m_patternHandler.registerPattern<CmdType::PDEA>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
        // PDEP
        m_patternHandler.registerPattern<CmdType::PDEP>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
        // PDXA
        m_patternHandler.registerPattern<CmdType::PDXA>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
        // PDXP
        m_patternHandler.registerPattern<CmdType::PDXP>({
            H, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
            X, X, X, X, X, X, X, X, X,
        });
    }

    void DDR4::registerCommands(){
        // ACT
        routeCoreCommand<CmdType::ACT>(core.getRegisterHelper().registerBankHandler(&DDR4Core::handleAct));
        routeInterfaceCommand<CmdType::ACT>(interface.getRegisterHelper().registerHandler(&DDR4Interface::handleCommandBus));
        // PRE
        routeCoreCommand<CmdType::PRE>(core.getRegisterHelper().registerBankHandler(&DDR4Core::handlePre));
        routeInterfaceCommand<CmdType::PRE>(interface.getRegisterHelper().registerHandler(&DDR4Interface::handleCommandBus));
        // PREA
        routeCoreCommand<CmdType::PREA>(core.getRegisterHelper().registerRankHandler(&DDR4Core::handlePreAll));
        routeInterfaceCommand<CmdType::PREA>(interface.getRegisterHelper().registerHandler(&DDR4Interface::handleCommandBus));
        // REFA
        routeCoreCommand<CmdType::REFA>(core.getRegisterHelper().registerRankHandler(&DDR4Core::handleRefAll));
        routeInterfaceCommand<CmdType::REFA>(interface.getRegisterHelper().registerHandler(&DDR4Interface::handleCommandBus));
        // RD
        routeCoreCommand<CmdType::RD>(core.getRegisterHelper().registerBankHandler(&DDR4Core::handleRead));
        routeInterfaceCommand<CmdType::RD>([this](const Command &command){interface.handleData(command, true);});
        // RDA
        routeCoreCommand<CmdType::RDA>(core.getRegisterHelper().registerBankHandler(&DDR4Core::handleReadAuto));
        routeInterfaceCommand<CmdType::RDA>([this](const Command &command){interface.handleData(command, true);});
        // WR
        routeCoreCommand<CmdType::WR>(core.getRegisterHelper().registerBankHandler(&DDR4Core::handleWrite));
        routeInterfaceCommand<CmdType::WR>([this](const Command &command){interface.handleData(command, false);});
        // WRA
        routeCoreCommand<CmdType::WRA>(core.getRegisterHelper().registerBankHandler(&DDR4Core::handleWriteAuto));
        routeInterfaceCommand<CmdType::WRA>([this](const Command &command){interface.handleData(command, false);});
        // SREFEN
        routeCoreCommand<CmdType::SREFEN>(core.getRegisterHelper().registerRankHandler(&DDR4Core::handleSelfRefreshEntry));
        routeInterfaceCommand<CmdType::SREFEN>(interface.getRegisterHelper().registerHandler(&DDR4Interface::handleCommandBus));
        // SREFEX
        routeCoreCommand<CmdType::SREFEX>(core.getRegisterHelper().registerRankHandler(&DDR4Core::handleSelfRefreshExit));
        routeInterfaceCommand<CmdType::SREFEX>(interface.getRegisterHelper().registerHandler(&DDR4Interface::handleCommandBus));
        // PDEA
        routeCoreCommand<CmdType::PDEA>(core.getRegisterHelper().registerRankHandler(&DDR4Core::handlePowerDownActEntry));
        routeInterfaceCommand<CmdType::PDEA>(interface.getRegisterHelper().registerHandler(&DDR4Interface::handleCommandBus));
        // PDEP
        routeCoreCommand<CmdType::PDEP>(core.getRegisterHelper().registerRankHandler(&DDR4Core::handlePowerDownPreEntry));
        routeInterfaceCommand<CmdType::PDEP>(interface.getRegisterHelper().registerHandler(&DDR4Interface::handleCommandBus));
        // PDXA
        routeCoreCommand<CmdType::PDXA>(core.getRegisterHelper().registerRankHandler(&DDR4Core::handlePowerDownActExit));
        routeInterfaceCommand<CmdType::PDXA>(interface.getRegisterHelper().registerHandler(&DDR4Interface::handleCommandBus));
        // PDXP
        routeCoreCommand<CmdType::PDXP>(core.getRegisterHelper().registerRankHandler(&DDR4Core::handlePowerDownPreExit));
        routeInterfaceCommand<CmdType::PDXP>(interface.getRegisterHelper().registerHandler(&DDR4Interface::handleCommandBus));
        // EOS
        routeCoreCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
        routeInterfaceCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
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
void DDR4Interface::enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp) {
    // Change from bus to toggling rate
    assert(enable_timestamp >= timestamp);
    if ( enable_timestamp > timestamp ) {
        // Schedule toggling rate enable
        this->addImplicitCommand(enable_timestamp, [this, enable_timestamp]() {
            m_dataBus.enableTogglingRate(enable_timestamp);
        });
    } else {
        m_dataBus.enableTogglingRate(enable_timestamp);
    }
}

void DDR4Interface::enableBus(timestamp_t timestamp, timestamp_t enable_timestamp) {
    // Change from toggling rate to bus
    assert(enable_timestamp >= timestamp);
    if ( enable_timestamp > timestamp ) {
        // Schedule toggling rate disable
        this->addImplicitCommand(enable_timestamp, [this, enable_timestamp]() {
            m_dataBus.enableBus(enable_timestamp);
        });
    } else {
        m_dataBus.enableBus(enable_timestamp);
    }
}

timestamp_t DDR4Interface::updateTogglingRate(timestamp_t timestamp, const std::optional<ToggleRateDefinition> &toggleratedefinition)
{
    if (toggleratedefinition) {
        m_dataBus.setTogglingRateDefinition(*toggleratedefinition);
        if (m_dataBus.isTogglingRate()) {
            // toggling rate already enabled
            return timestamp;
        }
        // Enable toggling rate
        timestamp_t enable_timestamp = std::max(timestamp, m_dataBus.lastBurst());
        enableTogglingHandle(timestamp, enable_timestamp);
        return enable_timestamp;
    } else {
        if (m_dataBus.isBus()) {
            // Bus already enabled
            return timestamp;
        }
        // Enable bus
        timestamp_t enable_timestamp = std::max(timestamp, m_dataBus.lastBurst());
        enableBus(timestamp, enable_timestamp);
        return enable_timestamp;
    }
    return timestamp;
}

    void DDR4Interface::handlePrePostamble(
        const timestamp_t   timestamp,
        const uint64_t      length,
        RankInterface       &rank,
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

    void DDR4Interface::handleOverrides(size_t length, bool read)
    {
        // Set command bus pattern overrides
        switch(length) {
            case 4:
                if(read)
                {
                    // Read
                    
                    m_patternHandler.getEncoder().settings.updateSettings({
                        {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C1, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C0, PatternEncoderBitSpec::L},
                    });
                }
                else
                {
                    // Write
                    m_patternHandler.getEncoder().settings.updateSettings({
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
                    m_patternHandler.getEncoder().settings.updateSettings({
                        {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C1, PatternEncoderBitSpec::L},
                        {pattern_descriptor::C0, PatternEncoderBitSpec::L},
                    });
                }
                else
                {
                    // Write
                    m_patternHandler.getEncoder().settings.updateSettings({
                        {pattern_descriptor::C2, PatternEncoderBitSpec::H},
                        {pattern_descriptor::C1, PatternEncoderBitSpec::H},
                        {pattern_descriptor::C0, PatternEncoderBitSpec::H},
                    });
                }
                break;
        }
    }

    void DDR4Interface::handleCommandBus(const Command &cmd) {
        auto pattern = m_patternHandler.getCommandPattern(cmd);
        auto ca_length = m_patternHandler.getPattern(cmd.type).size() / m_commandBus.get_width();
        this->m_commandBus.load(cmd.timestamp, pattern, ca_length);
    }

    void DDR4Interface::handleDQs(const Command &cmd, util::Clock &dqs, const size_t length) {
        dqs.start(cmd.timestamp);
        dqs.stop(cmd.timestamp + length / m_memSpec.dataRate);
    }

    void DDR4Interface::handleData(const Command &cmd, bool read) {
        auto loadfunc = read ? &databus_t::loadRead : &databus_t::loadWrite;
        util::Clock &dqs = read ? m_readDQS : m_writeDQS;
        size_t length = 0;
        if (0 == cmd.sz_bits) {
            // No data provided by command
            // Use default burst length
            if (m_dataBus.isTogglingRate()) {
                // If bus is enabled skip loading data
                length = m_memSpec.burstLength;
                (m_dataBus.*loadfunc)(cmd.timestamp, length * m_dataBus.getWidth(), nullptr);
            }
        } else {
            std::optional<const uint8_t *> dbi_data = std::nullopt;
            // Data provided by command
            if (m_dataBus.isBus() && m_dbi.isEnabled()) {
                // Only compute dbi for bus mode
                dbi_data = handleDBIInterface(cmd.timestamp, cmd.sz_bits, cmd.data, read);
            }
            length = cmd.sz_bits / (m_dataBus.getWidth());
            (m_dataBus.*loadfunc)(cmd.timestamp, cmd.sz_bits, dbi_data.value_or(cmd.data));
        }
        handleOverrides(length, read);
        handleDQs(cmd, dqs, length);
        handleCommandBus(cmd);
        assert(m_ranks.size()>cmd.targetCoordinate.rank);
        auto & rank = m_ranks[cmd.targetCoordinate.rank];
        handlePrePostamble(cmd.timestamp, length / m_memSpec.dataRate, rank, read);
    }

// Core
    void DDR4Core::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
        bank.counter.act++;
        bank.bankState = Bank::BankState::BANK_ACTIVE;

        bank.cycles.act.start_interval(timestamp);
        
        rank.cycles.act.start_interval_if_not_running(timestamp);
        //rank.cycles.pre.close_interval(timestamp);
    }

    void DDR4Core::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
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

    void DDR4Core::handlePreAll(Rank &rank, timestamp_t timestamp) {
        for (auto &bank: rank.banks) 
            handlePre(rank, bank, timestamp);
    }

    void DDR4Core::handleRefAll(Rank &rank, timestamp_t timestamp) {
        auto timestamp_end = timestamp + m_memSpec.memTimingSpec.tRFC;
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

    void DDR4Core::handleRead(Rank&, Bank &bank, timestamp_t) {
        ++bank.counter.reads;
    }

    void DDR4Core::handleReadAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.readAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
        auto minReadActiveTime = timestamp + m_memSpec.prechargeOffsetRD;

        auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void DDR4Core::handleWrite(Rank&, Bank &bank, timestamp_t) {
        ++bank.counter.writes;
    }

    void DDR4Core::handleWriteAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.writeAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRAS;
        auto minWriteActiveTime =  timestamp + m_memSpec.prechargeOffsetWR;

        auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);

        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void DDR4Core::handleSelfRefreshEntry(Rank &rank, timestamp_t timestamp) {
        // Issue implicit refresh
        handleRefAll(rank, timestamp);
        // Handle self-refresh entry after tRFC
        auto timestampSelfRefreshStart = timestamp + m_memSpec.memTimingSpec.tRFC;
        addImplicitCommand(timestampSelfRefreshStart, [&rank, timestampSelfRefreshStart]() {
            rank.counter.selfRefresh++;
            rank.cycles.sref.start_interval(timestampSelfRefreshStart);
            rank.memState = MemState::SREF;
        });
    }

    void DDR4Core::handleSelfRefreshExit(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::SREF);                                    // check for previous SelfRefreshEntry
        rank.cycles.sref.close_interval(timestamp);                                 // Duration between entry and exit
        rank.memState = MemState::NOT_IN_PD;
    }

    void DDR4Core::handlePowerDownActEntry(Rank &rank, timestamp_t timestamp) {
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

    void DDR4Core::handlePowerDownActExit(Rank &rank, timestamp_t timestamp) {
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

    void DDR4Core::handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp) {
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

    void DDR4Core::handlePowerDownPreExit(Rank &rank, timestamp_t timestamp) {
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

    timestamp_t DDR4Core::earliestPossiblePowerDownEntryTime(Rank & rank) const {
        timestamp_t entryTime = 0;

        for (const auto & bank : rank.banks) {
            entryTime = std::max({ entryTime,
                                   bank.counter.act == 0 ? 0 :  bank.cycles.act.get_start() + m_memSpec.memTimingSpec.tRCD,
                                   bank.counter.pre == 0 ? 0 : bank.latestPre + m_memSpec.memTimingSpec.tRP,
                                   bank.refreshEndTime
                                 });
        }

        return entryTime;
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
    void DDR4Core::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
        // resize banks and ranks
        stats.bank.resize(m_memSpec.numberOfBanks * m_memSpec.numberOfRanks);
        stats.rank_total.resize(m_memSpec.numberOfRanks);

        auto simulation_duration = timestamp;
        for (size_t i = 0; i < m_memSpec.numberOfRanks; ++i) {
            const Rank &rank = m_ranks[i];
            size_t bank_offset = i * m_memSpec.numberOfBanks;
            for (size_t j = 0; j < m_memSpec.numberOfBanks; ++j) {
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
        }
    }

    void DDR4Interface::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
        // Reset the DBI interface pins to idle state
        m_dbi.dispatchResetCallback(timestamp * m_memSpec.dataRate, true);

        // DDR4 x16 have 2 DQs differential pairs
        uint_fast8_t NumDQsPairs = 1;
        if(m_memSpec.bitWidth == 16) {
            NumDQsPairs = 2;
        }
        stats.rank_total.resize(m_memSpec.numberOfRanks);
        for (size_t i = 0; i < m_memSpec.numberOfRanks; ++i) {
            const RankInterface &rank_interface = m_ranks[i];
            stats.rank_total[i].prepos.readSeamless = rank_interface.seamlessPrePostambleCounter_read;
            stats.rank_total[i].prepos.writeSeamless = rank_interface.seamlessPrePostambleCounter_write;
            
            stats.rank_total[i].prepos.readMerged = rank_interface.mergedPrePostambleCounter_read;
            stats.rank_total[i].prepos.readMergedTime = rank_interface.mergedPrePostambleTime_read;
            stats.rank_total[i].prepos.writeMerged = rank_interface.mergedPrePostambleCounter_write;
            stats.rank_total[i].prepos.writeMergedTime = rank_interface.mergedPrePostambleTime_write;
        }

        stats.commandBus = m_commandBus.get_stats(timestamp);

        m_dataBus.get_stats(timestamp,
            stats.readBus,
            stats.writeBus,
            stats.togglingStats.read,
            stats.togglingStats.write
        );

        // single line stored in stats
        // differential power calculated in interface calculation
        stats.clockStats = 2u * m_clock.get_stats_at(timestamp);
        stats.readDQSStats = NumDQsPairs * 2u * m_readDQS.get_stats_at(timestamp);
        stats.writeDQSStats = NumDQsPairs * 2u * m_writeDQS.get_stats_at(timestamp);
        for (const auto &dbi_pin : m_dbiread) {
            stats.readDBI += dbi_pin.get_stats_at(timestamp, 2);
        }
        for (const auto &dbi_pin : m_dbiwrite) {
            stats.writeDBI += dbi_pin.get_stats_at(timestamp, 2);
        }
    }

    SimulationStats DDR4::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        processImplicitCommandQueue(timestamp);
        SimulationStats stats;
        core.getWindowStats(timestamp, stats);
        interface.getWindowStats(timestamp, stats);
        return stats;
    }

} // namespace DRAMPower
