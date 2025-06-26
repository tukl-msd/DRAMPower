#include "LPDDR4Interface.h"
#include "DRAMPower/data/stats.h"

namespace DRAMPower {

LPDDR4Interface::LPDDR4Interface(const std::shared_ptr<const MemSpecLPDDR4>& memSpec, implicitCommandInserter_t&& implicitCommandInserter)
    : m_commandBus{6, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L}
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
    , m_writeDQS(memSpec->dataRate, true)
    , m_memSpec(memSpec)
    , m_patternHandler(PatternEncoderOverrides{
        {pattern_descriptor::C0, PatternEncoderBitSpec::L},
        {pattern_descriptor::C1, PatternEncoderBitSpec::L},
    })
    , m_implicitCommandInserter(std::move(implicitCommandInserter))
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

void LPDDR4Interface::enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp) {
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

void LPDDR4Interface::enableBus(timestamp_t timestamp, timestamp_t enable_timestamp) {
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

timestamp_t LPDDR4Interface::updateTogglingRate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) {
    if (toggleRateDefinition) {
        m_dataBus.setTogglingRateDefinition(*toggleRateDefinition);
        if (m_dataBus.isTogglingRate()) {
            // toggling rate already enabled
            return timestamp;
        }
        // Enable toggling rate
        auto enable_timestamp = std::max(timestamp, m_dataBus.lastBurst());
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
    dqs.stop(cmd.timestamp + length / m_memSpec->dataRate);
}

void LPDDR4Interface::handleData(const Command &cmd, bool read) {
    auto loadfunc = read ? &databus_t::loadRead : &databus_t::loadWrite;
    util::Clock &dqs = read ? m_readDQS : m_writeDQS;
    size_t length = 0;
    if (0 == cmd.sz_bits) {
        // No data provided by command
        // Use default burst length
        if (m_dataBus.isTogglingRate()) {
            length = m_memSpec->burstLength;
            (m_dataBus.*loadfunc)(cmd.timestamp, length * m_dataBus.getWidth(), nullptr);
        }
    } else {
        // Data provided by command
        length = cmd.sz_bits / m_dataBus.getWidth();
        (m_dataBus.*loadfunc)(cmd.timestamp, cmd.sz_bits, cmd.data);
    }
    handleDQs(cmd, dqs, length);
    handleOverrides(length, read);
    handleCommandBus(cmd);
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
    if (m_memSpec->bitWidth == 16) {
        stats.readDQSStats *= 2;
        stats.writeDQSStats *= 2;
    }
}

} // namespace DRAMPower