#include "DDR5Interface.h"
#include "DRAMPower/Types.h"
#include "DRAMPower/simconfig/simconfig.h"

namespace DRAMPower {

static constexpr DRAMUtils::Config::ToggleRateDefinition busConfig {
    0,
    0,
    0,
    0,
    DRAMUtils::Config::TogglingRateIdlePattern::H,
    DRAMUtils::Config::TogglingRateIdlePattern::H
};

DDR5Interface::DDR5Interface(const MemSpecDDR5& memSpec, const config::SimConfig& simConfig)
    : m_memSpec(memSpec)
    , m_commandBus{cmdBusWidth, 1,
        util::BusIdlePatternSpec::H}
    , m_dataBus{
        util::databus_presets::getDataBusPreset(
            util::DataBusConfig {
                memSpec.bitWidth * memSpec.numberOfDevices,
                memSpec.dataRate,
                simConfig.toggleRateDefinition.value_or(busConfig)
            },
            simConfig.toggleRateDefinition.has_value()
                ? util::DataBusMode::TogglingRate
                : util::DataBusMode::Bus
        )
    }
    , m_readDQS(memSpec.dataRateSpec.dqsBusRate, true)
    , m_writeDQS(memSpec.dataRateSpec.dqsBusRate, true)
    , m_patternHandler(PatternEncoderOverrides{
            {pattern_descriptor::V, PatternEncoderBitSpec::H},
            {pattern_descriptor::X, PatternEncoderBitSpec::H},
            {pattern_descriptor::C0, PatternEncoderBitSpec::H},
            {pattern_descriptor::C1, PatternEncoderBitSpec::H},
            // Default value for CID0-3 is H in Pattern.h
            // {pattern_descriptor::CID0, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID1, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID2, PatternEncoderBitSpec::H},
            // {pattern_descriptor::CID3, PatternEncoderBitSpec::H},
          }, DDR5Interface::cmdBusInitPattern)
    {
        registerPatterns();
    }

void DDR5Interface::setSimulationTime(timestamp_t timestamp) {
    m_offset = timestamp;
}

void DDR5Interface::reset() {
    m_commandBus.reset();
    m_dataBus.reset();
    m_readDQS.reset();
    m_writeDQS.reset();
    m_clock.reset();
    m_patternHandler.reset();
    m_last_command_time = 0;
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

timestamp_t DDR5Interface::getLastCommandTime() const {
    return m_last_command_time + m_offset;
}

void DDR5Interface::doCommand(const Command& cmd) {
        assert(cmd.timestamp >= m_offset);
        timestamp_t timestamp = cmd.timestamp - m_offset;
    switch(cmd.type) {
        case CmdType::NOP:
        case CmdType::ACT:
        case CmdType::PRE:
        case CmdType::PRESB:
        case CmdType::REFSB:
        case CmdType::REFA:
        case CmdType::PREA:
        case CmdType::SREFEN:
        case CmdType::SREFEX:
        case CmdType::PDEA:
        case CmdType::PDEP:
        case CmdType::PDXA:
        case CmdType::PDXP:
                handleCommandBus(timestamp, cmd.type, cmd.targetCoordinate);
            break;
        case CmdType::RD:
        case CmdType::RDA:
                handleData(timestamp, cmd.type, cmd.data, cmd.sz_bits, cmd.targetCoordinate, true);
            break;
        case CmdType::WR:
        case CmdType::WRA:
                handleData(timestamp, cmd.type, cmd.data, cmd.sz_bits, cmd.targetCoordinate, false);
            break;
        case CmdType::END_OF_SIMULATION:
            break;
        default:
            assert(false && "Invalid command");
            break;
    }
    m_last_command_time = timestamp;
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

void DDR5Interface::handleCommandBus(timestamp_t timestamp, CmdType type, const TargetCoordinate& target) {
    auto pattern = m_patternHandler.getCommandPattern(type, target);
    auto ca_length = m_patternHandler.getPattern(type).size() / m_commandBus.get_width();
    m_commandBus.load(timestamp, pattern, ca_length);
}

void DDR5Interface::handleDQs(timestamp_t timestamp, util::Clock &dqs, size_t length) {
    dqs.start(timestamp);
    dqs.stop(timestamp + length / m_memSpec.dataRate);
}

void DDR5Interface::handleData(timestamp_t timestamp, CmdType type, const uint8_t* data, std::size_t sz_bits, const TargetCoordinate& target,  bool read) {
    auto loadfunc = read ? &databus_t::loadRead : &databus_t::loadWrite;
    util::Clock &dqs = read ? m_readDQS : m_writeDQS;
    size_t length = 0;
    if (0 == sz_bits) {
        // No data provided by command
        // Use default burst length
        if (m_dataBus.isTogglingRate()) {
            length = m_memSpec.burstLength;
            (m_dataBus.*loadfunc)(timestamp, length * m_dataBus.getWidth(), nullptr);
        }
    } else {
        // Data provided by command
        length = sz_bits / (m_dataBus.getWidth());
        (m_dataBus.*loadfunc)(timestamp, sz_bits, data);
    }
    handleDQs(timestamp, dqs, length);
    handleOverrides(length, read);
    handleCommandBus(timestamp, type, target);
}

void DDR5Interface::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
    assert(timestamp >= m_offset);
    timestamp = timestamp - m_offset;
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
    if(m_memSpec.bitWidth == 16)
    {
        stats.readDQSStats *= 2;
        stats.writeDQSStats *= 2;
    }
}

void DDR5Interface::serialize(std::ostream& stream) const {
    stream.write(reinterpret_cast<const char*>(&m_last_command_time), sizeof(m_last_command_time));
    stream.write(reinterpret_cast<const char*>(&m_offset), sizeof(m_offset));
    m_patternHandler.serialize(stream);
    m_commandBus.serialize(stream);
    m_dataBus.serialize(stream);
    m_readDQS.serialize(stream);
    m_writeDQS.serialize(stream);
    m_clock.serialize(stream);
}

void DDR5Interface::deserialize(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&m_last_command_time), sizeof(m_last_command_time));
    stream.read(reinterpret_cast<char*>(&m_offset), sizeof(m_offset));
    m_patternHandler.deserialize(stream);
    m_commandBus.deserialize(stream);
    m_dataBus.deserialize(stream);
    m_readDQS.deserialize(stream);
    m_writeDQS.deserialize(stream);
    m_clock.deserialize(stream);
}

} // namespace DRAMPower
