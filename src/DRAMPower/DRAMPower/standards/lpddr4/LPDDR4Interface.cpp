#include "LPDDR4Interface.h"
#include "DRAMPower/command/CmdType.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/util/pin.h"

namespace DRAMPower {

static constexpr DRAMUtils::Config::ToggleRateDefinition busConfig {
    0,
    0,
    0,
    0,
    DRAMUtils::Config::TogglingRateIdlePattern::H,
    DRAMUtils::Config::TogglingRateIdlePattern::H
};

LPDDR4Interface::LPDDR4Interface(const MemSpecLPDDR4& memSpec, const config::SimConfig& simConfig)
    : m_memSpec(memSpec)
    , m_commandBus{6, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L}
    , m_dataBus{
        util::databus_presets::getDataBusPreset(
            util::DataBusConfig {
                memSpec.bitWidth * memSpec.numberOfDevices,
                memSpec.dataRate,
                simConfig.toggleRateDefinition.value_or(busConfig)
            },
            simConfig.toggleRateDefinition.has_value()
                ? util::DataBusMode::TogglingRate
                : util::DataBusMode::Bus,
            false
        )
    }
    , m_readDQS(memSpec.dataRate, true)
    , m_writeDQS(memSpec.dataRate, true)
    , m_dbi(memSpec.numberOfDevices * memSpec.bitWidth, m_memSpec.burstLength,
        [this](timestamp_t load_timestamp, timestamp_t, std::size_t pin, bool inversion_state, bool read) {
        this->handleDBIPinChange(load_timestamp, pin, inversion_state, read);
    }, false)
    , m_dbiread(m_dbi.getChunksPerWidth().value(), pin_dbi_t{m_dbi.getIdlePattern(), m_dbi.getIdlePattern()})
    , m_dbiwrite(m_dbi.getChunksPerWidth().value(), pin_dbi_t{m_dbi.getIdlePattern(), m_dbi.getIdlePattern()})
    , m_patternHandler(PatternEncoderOverrides{
        {pattern_descriptor::C0, PatternEncoderBitSpec::L},
        {pattern_descriptor::C1, PatternEncoderBitSpec::L},
    })
{
    registerPatterns();
}

void LPDDR4Interface::registerPatterns() {
    using namespace pattern_descriptor;
    // ACT
    m_patternHandler.registerPattern<CmdType::ACT>({
        H, L, R12, R13, R14, R15,
        BA0, BA1, BA2, R16, R10, R11,
        R17, R18, R6, R7, R8, R9,
        R0, R1, R2, R3, R4, R5,
    });
    // PRE
    m_patternHandler.registerPattern<CmdType::PRE>({
        L, L, L, L, H, L,
        BA0, BA1, BA2, V, V, V,
    });
    // PREA
    m_patternHandler.registerPattern<CmdType::PREA>({
        L, L, L, L, H, H,
        V, V, V, V, V, V,
    });
    // REFB
    m_patternHandler.registerPattern<CmdType::REFB>({
        L, L, L, H, L, L,
        BA0, BA1, BA2, V, V, V,
    });
    // RD
    m_patternHandler.registerPattern<CmdType::RD>({
        L, H, L, L, L, BL,
        BA0, BA1, BA2, V, C9, L,
        L, H, L, L, H, C8,
        C2, C3, C4, C5, C6, C7
    });
    // RDA
    m_patternHandler.registerPattern<CmdType::RDA>({
        L, H, L, L, L, BL,
        BA0, BA1, BA2, V, C9, H,
        L, H, L, L, H, C8,
        C2, C3, C4, C5, C6, C7
    });
    // WR
    m_patternHandler.registerPattern<CmdType::WR>({
        L, L, H, L, L, BL,
        BA0, BA1, BA2, V, C9, L,
        L, H, L, L, H, C8,
        C2, C3, C4, C5, C6, C7,
    });
    // WRA
    m_patternHandler.registerPattern<CmdType::WRA>({
        L, L, H, L, L, BL,
        BA0, BA1, BA2, V, C9, H,
        L, H, L, L, H, C8,
        C2, C3, C4, C5, C6, C7,
    });
    // REFA
    m_patternHandler.registerPattern<CmdType::REFA>({
        L, L, L, H, L, H,
        V, V, V, V, V, V,
    });
    // SREFEN
    m_patternHandler.registerPattern<CmdType::SREFEN>({
        L, L, L, H, H, V,
        V, V, V, V, V, V,
    });
    // SREFEX
    m_patternHandler.registerPattern<CmdType::SREFEX>({
        L, L, H, L, H, V,
        V, V, V, V, V, V,
    });
}

    timestamp_t LPDDR4Interface::getLastCommandTime() const {
        return m_last_command_time;
    }

    void LPDDR4Interface::doCommand(const Command& cmd) {
        switch(cmd.type) {
            case CmdType::ACT:
            case CmdType::PRE:
            case CmdType::PREA:
            case CmdType::REFB:
            case CmdType::REFA:
            case CmdType::SREFEN:
            case CmdType::SREFEX:
                handleCommandBus(cmd);
                break;
            case CmdType::RD:
            case CmdType::RDA:
                handleData(cmd, true);
                break;
            case CmdType::WR:
            case CmdType::WRA:
                handleData(cmd, false);
                break;
            case CmdType::END_OF_SIMULATION:
                endOfSimulation(cmd.timestamp);
                break;
            case CmdType::PDEA:
            case CmdType::PDXA:
            case CmdType::PDEP:
            case CmdType::PDXP:
                break;
            default:
                assert(false && "Invalid command");
                break;
        }
        m_last_command_time = cmd.timestamp;
    }

void LPDDR4Interface::handleDBIPinChange(const timestamp_t load_timestamp, std::size_t pin, bool state, bool read) {
    assert(pin < m_dbiread.size() || pin < m_dbiwrite.size());
    if (read) {
        this->m_dbiread[pin].set(load_timestamp, state ? util::PinState::H : util::PinState::L, 1);
    } else {
        this->m_dbiwrite[pin].set(load_timestamp, state ? util::PinState::H : util::PinState::L, 1);
    }
}

std::optional<const uint8_t *> LPDDR4Interface::handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read) {
    if (0 == n_bits || !data || !m_dbi.isEnabled()) {
        // No DBI or no data to process
        return std::nullopt;
    }
    timestamp_t virtual_time = timestamp * m_memSpec.dataRate;
    // updateDBI calls the given callback to handle pin changes
    return m_dbi.updateDBI(virtual_time, n_bits, data, read);
}

void LPDDR4Interface::handleOverrides(size_t length, bool /*read*/)
{
    // Set command bus pattern overrides
    switch(length) {
        case 32:
            m_patternHandler.getEncoder().settings.updateSettings({
                {pattern_descriptor::C4, PatternEncoderBitSpec::L},
                {pattern_descriptor::C3, PatternEncoderBitSpec::L},
                {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                {pattern_descriptor::BL, PatternEncoderBitSpec::H},
            });
            break;
        default:
            // Pull down
            // No interface power needed for PatternEncoderBitSpec::L
            // Defaults to burst length 16
        case 16:
            m_patternHandler.getEncoder().settings.removeSetting(pattern_descriptor::C4);
            m_patternHandler.getEncoder().settings.updateSettings({
                {pattern_descriptor::C3, PatternEncoderBitSpec::L},
                {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                {pattern_descriptor::BL, PatternEncoderBitSpec::L},
            });
            break;
    }
}

void LPDDR4Interface::handleCommandBus(const Command &cmd) {
    auto pattern = m_patternHandler.getCommandPattern(cmd);
    auto ca_length = m_patternHandler.getPattern(cmd.type).size() / m_commandBus.get_width();
    m_commandBus.load(cmd.timestamp, pattern, ca_length);
}

void LPDDR4Interface::handleDQs(const Command& cmd, util::Clock &dqs, size_t length) {
    dqs.start(cmd.timestamp);
    dqs.stop(cmd.timestamp + length / m_memSpec.dataRate);
}

void LPDDR4Interface::handleData(const Command &cmd, bool read) {
    auto loadfunc = read ? &databus_t::loadRead : &databus_t::loadWrite;
    util::Clock &dqs = read ? m_readDQS : m_writeDQS;
    size_t length = 0;
    if (0 == cmd.sz_bits) {
        // No data provided by command
        // Use default burst length
        if (m_dataBus.isTogglingRate()) {
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
    handleDQs(cmd, dqs, length);
    handleOverrides(length, read);
    handleCommandBus(cmd);
}

void LPDDR4Interface::endOfSimulation(timestamp_t timestamp) {
    m_dbi.dispatchResetCallback(timestamp);
}

void LPDDR4Interface::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
    stats.commandBus = m_commandBus.get_stats(timestamp);
    
    m_dataBus.get_stats(timestamp,
        stats.readBus,
        stats.writeBus,
        stats.togglingStats.read,
        stats.togglingStats.write
    );

    stats.clockStats = 2 * m_clock.get_stats_at(timestamp);
    stats.readDQSStats = 2 * m_readDQS.get_stats_at(timestamp);
    stats.writeDQSStats = 2 * m_writeDQS.get_stats_at(timestamp);

    for (const auto &dbi_pin : m_dbiread) {
        stats.readDBI += dbi_pin.get_stats_at(timestamp, 2);
    }
    for (const auto &dbi_pin : m_dbiwrite) {
        stats.writeDBI += dbi_pin.get_stats_at(timestamp, 2);
    }

    if (m_memSpec.bitWidth == 16) {
        stats.readDQSStats *= 2;
        stats.writeDQSStats *= 2;
    }
}

void LPDDR4Interface::serialize(std::ostream& stream) const {
    stream.write(reinterpret_cast<const char*>(&m_last_command_time), sizeof(m_last_command_time));
    m_patternHandler.serialize(stream);
    m_commandBus.serialize(stream);
    m_dataBus.serialize(stream);
    m_readDQS.serialize(stream);
    m_writeDQS.serialize(stream);
    m_clock.serialize(stream);
}
void LPDDR4Interface::deserialize(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&m_last_command_time), sizeof(m_last_command_time));
    m_patternHandler.deserialize(stream);
    m_commandBus.deserialize(stream);
    m_dataBus.deserialize(stream);
    m_readDQS.deserialize(stream);
    m_writeDQS.deserialize(stream);
    m_clock.deserialize(stream);
}


} // namespace DRAMPower