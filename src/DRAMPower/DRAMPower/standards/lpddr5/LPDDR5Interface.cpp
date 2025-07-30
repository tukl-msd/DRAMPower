#include "LPDDR5Interface.h"

namespace DRAMPower {

LPDDR5Interface::LPDDR5Interface(const std::shared_ptr<const MemSpecLPDDR5>& memSpec, implicitCommandInserter_t&& implicitCommandInserter)
    : m_commandBus{cmdBusWidth, 2, // modelled with datarate 2
        util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L}
    , m_dataBus{
        util::databus_presets::getDataBusPreset(
            memSpec->bitWidth * memSpec->numberOfDevices,
            util::DataBusConfig {
                memSpec->bitWidth * memSpec->numberOfDevices,
                memSpec->dataRate,
                util::BusIdlePatternSpec::L,
                util::BusInitPatternSpec::L,
                DRAMUtils::Config::TogglingRateIdlePattern::L,
                0.0,
                0.0,
                util::DataBusMode::Bus
            }
        )
    }
    , m_readDQS(memSpec->dataRate, true)
    , m_wck(memSpec->dataRate / memSpec->memTimingSpec.WCKtoCK, !memSpec->wckAlwaysOnMode)
    , m_dbi(memSpec->numberOfDevices * memSpec->bitWidth, util::DBI::IdlePattern_t::L, 8,
        [this](timestamp_t load_timestamp, timestamp_t chunk_timestamp, std::size_t pin, bool inversion_state, bool read) {
        this->handleDBIPinChange(load_timestamp, chunk_timestamp, pin, inversion_state, read);
    }, false)
    , m_dbiread(m_dbi.getChunksPerWidth(), util::Pin{m_dbi.getIdlePattern()})
    , m_dbiwrite(m_dbi.getChunksPerWidth(), util::Pin{m_dbi.getIdlePattern()})
    , m_memSpec(memSpec)
    , m_patternHandler(PatternEncoderOverrides{}) // No overrides
    , m_implicitCommandInserter(std::move(implicitCommandInserter))
{
    registerPatterns();
}

void LPDDR5Interface::registerPatterns() {
    using namespace pattern_descriptor;
    using commandPattern_t = std::vector<pattern_descriptor::t>;
    // ACT
    // LPDDR5 needs 2 commands for activation (ACT-1 and ACT-2)
    // ACT-1 must be followed by ACT-2 in almost every case (CAS, WRITE,
    // MASK WRITE and READ commands can be issued inbetween ACT-1 and ACT-2)
    // Here we consider ACT = ACT-1 + ACT-2, not considering interleaving
    commandPattern_t act_pattern = {
        // ACT-1
        // R1
        H, H, H, R14, R15, R16, R17,
        // F1
        BA0, BA1, BA2, BA3, R11, R12, R13,
        // ACT-2
        // R2
        H, H, L, R7, R8, R9, R10,
        // F2
        R0, R1, R2, R3, R4, R5, R6
    };
    if (m_memSpec->bank_arch == MemSpecLPDDR5::MBG) {
        act_pattern[9] = BG0;
        act_pattern[10] = BG1;
    } else if (m_memSpec->bank_arch == MemSpecLPDDR5::M8B) {
        act_pattern[10] = V;
    }
    m_patternHandler.registerPattern<CmdType::ACT>(act_pattern);
    // PRE
    commandPattern_t pre_pattern = {
        // R1
        L, L, L, H, H, H, H,
        // F1
        BA0, BA1, BA2, BA3, V, V, L
    };
    if (m_memSpec->bank_arch == MemSpecLPDDR5::MBG) {
        pre_pattern[9] = BG0;
        pre_pattern[10] = BG1;
    } else if (m_memSpec->bank_arch == MemSpecLPDDR5::M8B) {
        pre_pattern[10] = V;
    }
    m_patternHandler.registerPattern<CmdType::PRE>(pre_pattern);
    // PREA
    commandPattern_t prea_pattern = {
        // R1
        L, L, L, H, H, H, H,
        // F1
        V, V, V, V, V, V, H
    };
    if (m_memSpec->bank_arch == MemSpecLPDDR5::MBG) {
        prea_pattern[9] = BG0;
        prea_pattern[10] = BG1;
    } else if (m_memSpec->bank_arch == MemSpecLPDDR5::M8B) {
        prea_pattern[10] = V;
    }
    m_patternHandler.registerPattern<CmdType::PREA>(prea_pattern);
    // REFB
    // For refresh commands LPDDR5 has RFM (Refresh Management)
    // Considering RFM is disabled, CA3 is V
    commandPattern_t refb_pattern = {
        // R1
        L, L, L, H, H, H, L,
        // F1
        BA0, BA1, BA2, V, V, V, L
    };
    if (m_memSpec->bank_arch == MemSpecLPDDR5::MBG) {
        refb_pattern[9] = BG0;
    }
    m_patternHandler.registerPattern<CmdType::REFB>(refb_pattern);
    // RD
    commandPattern_t rd_pattern = {
        // R1
        H, L, L, C0, C3, C4, C5,
        // F1
        BA0, BA1, BA2, BA3, C1, C2, L
    };
    if (m_memSpec->bank_arch == MemSpecLPDDR5::MBG) {
        rd_pattern[9] = BG0;
        rd_pattern[10] = BG1;
    } else if (m_memSpec->bank_arch == MemSpecLPDDR5::M8B) {
        rd_pattern[10] = L; // B4
    }
    m_patternHandler.registerPattern<CmdType::RD>(rd_pattern);
    // RDA
    commandPattern_t rda_pattern = {
        // R1
        H, L, L, C0, C3, C4, C5,
        // F1
        BA0, BA1, BA2, BA3, C1, C2, H
    };
    if (m_memSpec->bank_arch == MemSpecLPDDR5::MBG) {
        rda_pattern[9] = BG0;
        rda_pattern[10] = BG1;
    } else if (m_memSpec->bank_arch == MemSpecLPDDR5::M8B) {
        rda_pattern[10] = L;
    }
    m_patternHandler.registerPattern<CmdType::RDA>(rda_pattern);
    // WR
    commandPattern_t wr_pattern = {
        // R1
        L, H, H, C0, C3, C4, C5,
        // F1
        BA0, BA1, BA2, BA3, C1, C2, L
    };
    if (m_memSpec->bank_arch == MemSpecLPDDR5::MBG) {
        wr_pattern[9] = BG0;
        wr_pattern[10] = BG1;
    } else if (m_memSpec->bank_arch == MemSpecLPDDR5::M8B) {
        wr_pattern[10] = V;
    }
    m_patternHandler.registerPattern<CmdType::WR>(wr_pattern);
    // WRA
    commandPattern_t wra_pattern = {
        // R1
        L, H, H, C0, C3, C4, C5,
        // F1
        BA0, BA1, BA2, BA3, C1, C2, H
    };
    if (m_memSpec->bank_arch == MemSpecLPDDR5::MBG) {
        wra_pattern[9] = BG0;
        wra_pattern[10] = BG1;
    } else if (m_memSpec->bank_arch == MemSpecLPDDR5::M8B) {
        wra_pattern[10] = V;
    }
    m_patternHandler.registerPattern<CmdType::WRA>(wra_pattern);
    // REFP2B
    if (m_memSpec->bank_arch == MemSpecLPDDR5::MBG || m_memSpec->bank_arch == MemSpecLPDDR5::M16B) {
        m_patternHandler.registerPattern<CmdType::REFP2B>(refb_pattern);
    }
    // REFA
    commandPattern_t refa_pattern = {
        // R1
        L, L, L, H, H, H, L,
        // F1
        V, V, V, V, V, V, H
    };
    if (m_memSpec->bank_arch == MemSpecLPDDR5::MBG) {
        refa_pattern[9] = BG0;
    }
    m_patternHandler.registerPattern<CmdType::REFA>(refa_pattern);
    // SREFEN
    m_patternHandler.registerPattern<CmdType::SREFEN>({
        // R1
        L, L, L, H, L, H, H,
        // F1
        V, V, V, V, V, L, L
    });
    // SREFEX
    m_patternHandler.registerPattern<CmdType::SREFEX>({
        // R1
        L, L, L, H, L, H, L,
        // F1
        V, V, V, V, V, V, V
    });
    // PDEA
    m_patternHandler.registerPattern<CmdType::PDEA>({
        // R1
        L, L, L, H, L, H, H,
        // F1
        V, V, V, V, V, L, H
    });
    // PDEP
    m_patternHandler.registerPattern<CmdType::PDEP>({
        // R1
        L, L, L, H, L, H, H,
        // F1
        V, V, V, V, V, L, H
    });
    // PDXA
    m_patternHandler.registerPattern<CmdType::PDXA>({
        // R1
        L, L, L, H, L, H, L,
        // F1
        V, V, V, V, V, V, V
    });
    // PDXP
    m_patternHandler.registerPattern<CmdType::PDXP>({
        // R1
        L, L, L, H, L, H, L,
        // F1
        V, V, V, V, V, V, V
    });
    // DSMEN
    m_patternHandler.registerPattern<CmdType::DSMEN>({
        // R1
        L, L, L, H, L, H, H,
        // F1
        V, V, V, V, V, H, L
    });
    // DSMEX
    m_patternHandler.registerPattern<CmdType::DSMEX>({
        // R1
        L, L, L, H, L, H, L,
        // F1
        V, V, V, V, V, V, V
    });
}

void LPDDR5Interface::enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp) {
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

void LPDDR5Interface::enableBus(timestamp_t timestamp, timestamp_t enable_timestamp) {
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

timestamp_t LPDDR5Interface::updateTogglingRate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleratedefinition) {
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

void LPDDR5Interface::handleDBIPinChange(const timestamp_t load_timestamp, timestamp_t chunk_timestamp, std::size_t pin, bool state, bool read) {
    assert(pin < m_dbiread.size() || pin < m_dbiwrite.size());
    auto updatePinCallback = [this, chunk_timestamp, pin, state, read](){
        if (read) {
            this->m_dbiread[pin].set(chunk_timestamp, state ? util::PinState::H : util::PinState::L, 1);
        } else {
            this->m_dbiwrite[pin].set(chunk_timestamp, state ? util::PinState::H : util::PinState::L, 1);
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

std::optional<const uint8_t *> LPDDR5Interface::handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read) {
    if (0 == n_bits || !data || !m_dbi.isEnabled()) {
        // No DBI or no data to process
        return std::nullopt;
    }
    timestamp_t virtual_time = timestamp * m_memSpec->dataRate;
    // updateDBI calls the given callback to handle pin changes
    return m_dbi.updateDBI(virtual_time, n_bits, data, read);
}

void LPDDR5Interface::handleOverrides(size_t length, bool /*read*/) {
    // Set command bus pattern overrides
    switch(length) {
        case 32:
            m_patternHandler.getEncoder().settings.updateSettings({
                {pattern_descriptor::C0, PatternEncoderBitSpec::L},
            });
            break;
        default:
            // Pull down
            // No interface power needed for PatternEncoderBitSpec::L
            // Defaults to burst length 16
        case 16:
            m_patternHandler.getEncoder().settings.removeSetting(pattern_descriptor::C0);
            break;
    }
}

void LPDDR5Interface::handleCommandBus(const Command &cmd) {
    auto pattern = m_patternHandler.getCommandPattern(cmd);
    auto ca_length = m_patternHandler.getPattern(cmd.type).size() / m_commandBus.get_width();
    m_commandBus.load(cmd.timestamp, pattern, ca_length);
}

void LPDDR5Interface::handleData(const Command &cmd, bool read) {
    auto loadfunc = read ? &databus_t::loadRead : &databus_t::loadWrite;
    size_t length = 0;
    if (0 == cmd.sz_bits) {
        // No data provided by command
        if (m_dataBus.isTogglingRate()) {
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
    // DQS
    if (read) {
        // Read
        m_readDQS.start(cmd.timestamp);
        m_readDQS.stop(cmd.timestamp + length / m_memSpec->dataRate);
        if (!m_memSpec->wckAlwaysOnMode) {
            m_wck.start(cmd.timestamp);
            m_wck.stop(cmd.timestamp + length / m_memSpec->dataRate);
        }
    } else {
        // Write
        if (!m_memSpec->wckAlwaysOnMode) {
            m_wck.start(cmd.timestamp);
            m_wck.stop(cmd.timestamp + length / m_memSpec->dataRate);
        }
    }
    handleOverrides(length, read);
    handleCommandBus(cmd);
}

void LPDDR5Interface::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
    // Reset the DBI interface pins to idle state
    m_dbi.dispatchResetCallback(timestamp * m_memSpec->dataRate);

    stats.commandBus = m_commandBus.get_stats(timestamp);

    m_dataBus.get_stats(timestamp,
        stats.readBus,
        stats.writeBus,
        stats.togglingStats.read,
        stats.togglingStats.write
    );

    stats.clockStats = 2.0 * m_clock.get_stats_at(timestamp);
    stats.wClockStats = 2.0 * m_wck.get_stats_at(timestamp);
    stats.readDQSStats = 2.0 * m_readDQS.get_stats_at(timestamp);
    for (const auto &dbi_pin : m_dbiread) {
        stats.readDBI += dbi_pin.get_stats_at(timestamp, 2);
    }
    for (const auto &dbi_pin : m_dbiwrite) {
        stats.writeDBI += dbi_pin.get_stats_at(timestamp, 2);
    }
}

} // namespace DRAMPower