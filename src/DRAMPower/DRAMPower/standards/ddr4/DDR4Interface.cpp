#include "DDR4Interface.h"

namespace DRAMPower {

    DDR4Interface::DDR4Interface(const std::shared_ptr<const MemSpecDDR4>& memSpec, implicitCommandInserter_t&& implicitCommandInserter)
    : m_commandBus{cmdBusWidth, 1,
        util::BusIdlePatternSpec::H, util::BusInitPatternSpec::H}
    , m_dataBus{
        util::databus_presets::getDataBusPreset(
            memSpec->bitWidth * memSpec->numberOfDevices,
            util::DataBusConfig {
                memSpec->bitWidth * memSpec->numberOfDevices,
                memSpec->dataRate,
                util::BusIdlePatternSpec::H,
                util::BusInitPatternSpec::H,
                DRAMUtils::Config::TogglingRateIdlePattern::H,
                0.0,
                0.0,
                util::DataBusMode::Bus
            }
        )
    }
    , m_readDQS(memSpec->dataRate, true)
    , m_writeDQS(memSpec->dataRate, true)
    , m_clock(2, false)
    , m_dbi(memSpec->numberOfDevices * memSpec->bitWidth, util::DBI::IdlePattern_t::H, 8,
        [this](timestamp_t load_timestamp, timestamp_t chunk_timestamp, std::size_t pin, bool inversion_state, bool read) {
        this->handleDBIPinChange(load_timestamp, chunk_timestamp, pin, inversion_state, read);
    }, false)
    , m_dbiread(m_dbi.getChunksPerWidth(), util::Pin{m_dbi.getIdlePattern()})
    , m_dbiwrite(m_dbi.getChunksPerWidth(), util::Pin{m_dbi.getIdlePattern()})
    , prepostambleReadMinTccd(memSpec->prePostamble.readMinTccd)
    , prepostambleWriteMinTccd(memSpec->prePostamble.writeMinTccd)
    , m_ranks(memSpec->numberOfRanks)
    , m_memSpec(memSpec)
    , m_patternHandler(PatternEncoderOverrides {
            {pattern_descriptor::V, PatternEncoderBitSpec::H},
            {pattern_descriptor::X, PatternEncoderBitSpec::H},
        }, cmdBusInitPattern)
    , m_implicitCommandInserter(std::move(implicitCommandInserter))
    {
        registerPatterns();
    }

    void DDR4Interface::registerPatterns() {
        using namespace pattern_descriptor;
        // ACT
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

    // Update toggling rate
    void DDR4Interface::enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp) {
        // Change from bus to toggling rate
        assert(enable_timestamp >= timestamp);
        if ( enable_timestamp > timestamp ) {
            // Schedule toggling rate enable
            m_implicitCommandInserter.addImplicitCommand(enable_timestamp, [this, enable_timestamp]() {
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
            m_implicitCommandInserter.addImplicitCommand(enable_timestamp, [this, enable_timestamp]() {
                m_dataBus.enableBus(enable_timestamp);
            });
        } else {
            m_dataBus.enableBus(enable_timestamp);
        }
    }

    timestamp_t DDR4Interface::updateTogglingRate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleratedefinition)
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
            m_implicitCommandInserter.addImplicitCommand(chunk_timestamp / m_memSpec->dataRate, updatePinCallback);
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
        timestamp_t virtual_time = timestamp * m_memSpec->dataRate;
        // updateDBI calls the given callback to handle pin changes
        return m_dbi.updateDBI(virtual_time, n_bits, data, read);
    }

    void DDR4Interface::handlePrePostamble(
        const timestamp_t   timestamp,
        const uint64_t      length,
        RankInterface       &rank,
        bool                read
    )
    {
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
        dqs.stop(cmd.timestamp + length / m_memSpec->dataRate);
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
                length = m_memSpec->burstLength;
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
        handlePrePostamble(cmd.timestamp, length / m_memSpec->dataRate, rank, read);
    }

    void DDR4Interface::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
        // Reset the DBI interface pins to idle state
        m_dbi.dispatchResetCallback(timestamp * m_memSpec->dataRate, true);

        // DDR4 x16 have 2 DQs differential pairs
        uint_fast8_t NumDQsPairs = 1;
        if(m_memSpec->bitWidth == 16) {
            NumDQsPairs = 2;
        }
        stats.rank_total.resize(m_memSpec->numberOfRanks);
        for (size_t i = 0; i < m_memSpec->numberOfRanks; ++i) {
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

} // namespace DRAMPower
