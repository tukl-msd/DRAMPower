#include "DDR5Interface.h"
#include "DRAMPower/util/bus_types.h"
#include "DRAMUtils/config/toggling_rate.h"

namespace DRAMPower {

 DDR5Interface::DDR5Interface(const std::shared_ptr<const MemSpecDDR5>& memSpec, implicitCommandInserter_t&& implicitCommandInserter)
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
    , m_readDQS(memSpec->dataRateSpec.dqsBusRate, true)
    , m_writeDQS(memSpec->dataRateSpec.dqsBusRate, true)
    , m_memSpec(memSpec)
    , m_patternHandler(PatternEncoderOverrides{
            {pattern_descriptor::V, PatternEncoderBitSpec::H},
            {pattern_descriptor::X, PatternEncoderBitSpec::H}, // TODO high impedance ???
            {pattern_descriptor::C0, PatternEncoderBitSpec::H},
            {pattern_descriptor::C1, PatternEncoderBitSpec::H},
            // Default value for CID0-3 is H in Pattern.h
            // {pattern_descriptor::CID0, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID1, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID2, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID3, PatternEncoderBitSpec::H},
          }, DDR5Interface::cmdBusInitPattern)
    , m_implicitCommandInserter(std::move(implicitCommandInserter))
    {
        registerPatterns();
    }

void DDR5Interface::registerPatterns() {
    using namespace pattern_descriptor;
    // ACT
    m_patternHandler.registerPattern<CmdType::ACT>({
        // note: CID3 is mutually exclusive with R17 and depends on usage mode
        L, L, R0, R1, R2, R3, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
        R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16, CID3
    });
    // PRE
    m_patternHandler.registerPattern<CmdType::PRE>({
        H, H, L, H, H, CID3, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2
    });
    // RD
    m_patternHandler.registerPattern<CmdType::RD>({
        H, L, H, H, H, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
        C2, C3, C4, C5, C6, C7, C8, C9, C10, V, H, V, V, CID3
    });
    // RDA
    m_patternHandler.registerPattern<CmdType::RDA>({
        H, L, H, H, H, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
        C2, C3, C4, C5, C6, C7, C8, C9, C10, V, L, V, V, CID3
    });
    // WR
    m_patternHandler.registerPattern<CmdType::WR>({
        H, L, H, H, L, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
        V, C3, C4, C5, C6, C7, C8, C9, C10, V, H, H, V, CID3
    });
    // WRA
    m_patternHandler.registerPattern<CmdType::WRA>({
        H, L, H, H, L, H, BA0, BA1, BG0, BG1, BG2, CID0, CID1, CID2,
        V, C3, C4, C5, C6, C7, C8, C9, C10, V, L, H, V, CID3
    });
    // PRESB
    m_patternHandler.registerPattern<CmdType::PRESB>({
        H, H, L, H, L, CID3, BA0, BA1, V, V, H, CID0, CID1, CID2
    });
    // REFSB
    m_patternHandler.registerPattern<CmdType::REFSB>({
        H, H, L, L, H, CID3, BA0, BA1, V, V, H, CID0, CID1, CID2
    });
    // REFA
    m_patternHandler.registerPattern<CmdType::REFA>({
        H, H, L, L, H, CID3, V, V, V, V, L, CID0, CID1, CID2
    });
    // PREA
    m_patternHandler.registerPattern<CmdType::PREA>({
        H, H, L, H, L, CID3, V, V, V, V, L, CID0, CID1, CID2
    });
    // SREFEN
    m_patternHandler.registerPattern<CmdType::SREFEN>({
        H, H, H, L, H, V, V, V, V, H, L, V, V, V
    });
    // PDEA
    m_patternHandler.registerPattern<CmdType::PDEA>({
        H, H, H, L, H, V, V, V, V, V, H, L, V, V
    });
    // PDEP
    m_patternHandler.registerPattern<CmdType::PDEP>({
        H, H, H, L, H, V, V, V, V, V, H, L, V, V
    });
    // PDXA
    m_patternHandler.registerPattern<CmdType::PDXA>({
        H, H, H, H, H, V, V, V, V, V, V, V, V, V
    });
    // PDXP
    m_patternHandler.registerPattern<CmdType::PDXP>({
        H, H, H, H, H, V, V, V, V, V, V, V, V, V
    });
    // NOP
    m_patternHandler.registerPattern<CmdType::NOP>({
        H, H, H, H, H, V, V, V, V, V, V, V, V, V
    });
}

void DDR5Interface::enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp) {
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

void DDR5Interface::enableBus(timestamp_t timestamp, timestamp_t enable_timestamp) {
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

timestamp_t DDR5Interface::updateTogglingRate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleratedefinition)
{
    if (toggleratedefinition) {
        m_dataBus.setTogglingRateDefinition(*toggleratedefinition);
        if (m_dataBus.isTogglingRate()) {
            // toggling rate is already enabled
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

// Interface
void DDR5Interface::handleOverrides(size_t length, bool read)
{
    // Set command bus pattern overrides
    switch(length) {
        case 8:
            m_patternHandler.getEncoder().settings.removeSetting(pattern_descriptor::C10);
            if(read)
            {
                // Read
                m_patternHandler.getEncoder().settings.updateSettings({
                    {pattern_descriptor::C3, PatternEncoderBitSpec::L},
                    {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                });
            }
            else
            {
                // Write
                m_patternHandler.getEncoder().settings.updateSettings({
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
            m_patternHandler.getEncoder().settings.removeSetting(pattern_descriptor::C10);
            if(read)
            {
                // Read
                m_patternHandler.getEncoder().settings.updateSettings({
                    {pattern_descriptor::C3, PatternEncoderBitSpec::L},
                    {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                });
            }
            else
            {
                // Write
                m_patternHandler.getEncoder().settings.updateSettings({
                    {pattern_descriptor::C3, PatternEncoderBitSpec::H},
                    {pattern_descriptor::C2, PatternEncoderBitSpec::H},
                });
            }
            break;
        case 32:
            if(read)
            {
                // Read
                m_patternHandler.getEncoder().settings.updateSettings({
                    {pattern_descriptor::C10, PatternEncoderBitSpec::L},
                    {pattern_descriptor::C3, PatternEncoderBitSpec::L},
                    {pattern_descriptor::C2, PatternEncoderBitSpec::L},
                });
            }
            else
            {
                // Write
                m_patternHandler.getEncoder().settings.updateSettings({
                    {pattern_descriptor::C10, PatternEncoderBitSpec::L},
                    {pattern_descriptor::C3, PatternEncoderBitSpec::H},
                    {pattern_descriptor::C2, PatternEncoderBitSpec::H},
                });
            }
            break;
    }
}

void DDR5Interface::handleCommandBus(const Command& cmd) {
    auto pattern = m_patternHandler.getCommandPattern(cmd);
    auto ca_length = m_patternHandler.getPattern(cmd.type).size() / m_commandBus.get_width();
    m_commandBus.load(cmd.timestamp, pattern, ca_length);
}

void DDR5Interface::handleDQs(const Command& cmd, util::Clock &dqs, size_t length) {
    dqs.start(cmd.timestamp);
    dqs.stop(cmd.timestamp + length / m_memSpec->dataRate);
}

void DDR5Interface::handleData(const Command &cmd, bool read) {
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

void DDR5Interface::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
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

    // x16 devices have two dqs pairs
    if(m_memSpec->bitWidth == 16)
    {
        stats.readDQSStats *= 2;
        stats.writeDQSStats *= 2;
    }
}

} // namespace DRAMPower