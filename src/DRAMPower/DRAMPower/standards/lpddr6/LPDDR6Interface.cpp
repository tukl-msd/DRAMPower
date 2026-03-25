#include "LPDDR6Interface.h"
#include "DRAMPower/util/databus_types.h"

namespace DRAMPower {

static constexpr DRAMUtils::Config::ToggleRateDefinition busConfig {
    0,
    0,
    0,
    0,
    DRAMUtils::Config::TogglingRateIdlePattern::L,
    DRAMUtils::Config::TogglingRateIdlePattern::L
};

LPDDR6Interface::LPDDR6Interface(const MemSpecLPDDR6& memSpec, const config::SimConfig &simConfig)
    : m_memSpec(memSpec)
    , m_commandBus{cmdBusWidth, 2, // modelled with datarate 2
        util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L}
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
    , m_wck(memSpec.dataRate / memSpec.memTimingSpec.WCKtoCK, !memSpec.wckAlwaysOnMode)
    , m_dbi(std::nullopt, m_memSpec.burstLength, nullptr, false)
    , m_patternHandler(PatternEncoderOverrides{}) // No overrides
{
    registerPatterns();
    m_formatResult.resize(12 * 24 / 8); // 288 bits bursts // 36 bytes
}

void LPDDR6Interface::registerPatterns() {
    using namespace pattern_descriptor;
    using commandPattern_t = std::vector<pattern_descriptor::t>;
    // ACT
    // LPDDR6 needs 2 commands for activation (ACT-1 and ACT-2)
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
    if (m_memSpec.bank_arch == MemSpecLPDDR6::MBG) {
        act_pattern[9] = BG0;
        act_pattern[10] = BG1;
    } else if (m_memSpec.bank_arch == MemSpecLPDDR6::M8B) {
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
    if (m_memSpec.bank_arch == MemSpecLPDDR6::MBG) {
        pre_pattern[9] = BG0;
        pre_pattern[10] = BG1;
    } else if (m_memSpec.bank_arch == MemSpecLPDDR6::M8B) {
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
    if (m_memSpec.bank_arch == MemSpecLPDDR6::MBG) {
        prea_pattern[9] = BG0;
        prea_pattern[10] = BG1;
    } else if (m_memSpec.bank_arch == MemSpecLPDDR6::M8B) {
        prea_pattern[10] = V;
    }
    m_patternHandler.registerPattern<CmdType::PREA>(prea_pattern);
    // REFB
    // For refresh commands LPDDR6 has RFM (Refresh Management)
    // Considering RFM is disabled, CA3 is V
    commandPattern_t refb_pattern = {
        // R1
        L, L, L, H, H, H, L,
        // F1
        BA0, BA1, BA2, V, V, V, L
    };
    if (m_memSpec.bank_arch == MemSpecLPDDR6::MBG) {
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
    if (m_memSpec.bank_arch == MemSpecLPDDR6::MBG) {
        rd_pattern[9] = BG0;
        rd_pattern[10] = BG1;
    } else if (m_memSpec.bank_arch == MemSpecLPDDR6::M8B) {
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
    if (m_memSpec.bank_arch == MemSpecLPDDR6::MBG) {
        rda_pattern[9] = BG0;
        rda_pattern[10] = BG1;
    } else if (m_memSpec.bank_arch == MemSpecLPDDR6::M8B) {
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
    if (m_memSpec.bank_arch == MemSpecLPDDR6::MBG) {
        wr_pattern[9] = BG0;
        wr_pattern[10] = BG1;
    } else if (m_memSpec.bank_arch == MemSpecLPDDR6::M8B) {
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
    if (m_memSpec.bank_arch == MemSpecLPDDR6::MBG) {
        wra_pattern[9] = BG0;
        wra_pattern[10] = BG1;
    } else if (m_memSpec.bank_arch == MemSpecLPDDR6::M8B) {
        wra_pattern[10] = V;
    }
    m_patternHandler.registerPattern<CmdType::WRA>(wra_pattern);
    // REFP2B
    if (m_memSpec.bank_arch == MemSpecLPDDR6::MBG || m_memSpec.bank_arch == MemSpecLPDDR6::M16B) {
        m_patternHandler.registerPattern<CmdType::REFP2B>(refb_pattern);
    }
    // REFA
    commandPattern_t refa_pattern = {
        // R1
        L, L, L, H, H, H, L,
        // F1
        V, V, V, V, V, V, H
    };
    if (m_memSpec.bank_arch == MemSpecLPDDR6::MBG) {
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
    // EOS
}

timestamp_t LPDDR6Interface::getLastCommandTime() const {
    return m_last_command_time;
}

void LPDDR6Interface::doCommand(const Command& cmd) {
    switch(cmd.type) {
        case CmdType::ACT:
        case CmdType::PRE:
        case CmdType::PREA:
        case CmdType::REFB:
        case CmdType::REFA:
        case CmdType::SREFEN:
        case CmdType::SREFEX:
        case CmdType::PDEA:
        case CmdType::PDEP:
        case CmdType::PDXA:
        case CmdType::PDXP:
        case CmdType::DSMEN:
        case CmdType::DSMEX:
            handleCommandBus(cmd);
            break;
        case CmdType::REFP2B:
            if (m_memSpec.bank_arch != MemSpecLPDDR6::MBG && m_memSpec.bank_arch != MemSpecLPDDR6::M16B) {
                throw Exception(std::string("REFP2B command is not supported for this bank architecture: ") + CmdTypeUtil::to_string(CmdType::REFP2B));
            }
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
        default:
            assert(false && "Invalid command");
            break;
    }
    m_last_command_time = cmd.timestamp;
}

std::optional<const uint8_t *> LPDDR6Interface::handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read) {
    if (0 == n_bits || !data || !m_dbi.isEnabled()) {
        // No DBI or no data to process
        return std::nullopt;
    }
    timestamp_t virtual_time = timestamp * m_memSpec.dataRate;
    // updateDBI calls the given callback to handle pin changes
    return m_dbi.updateDBI(virtual_time, n_bits, data, read);
}

void LPDDR6Interface::handleOverrides(size_t length, bool /*read*/) {
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

void LPDDR6Interface::handleCommandBus(const Command &cmd) {
    auto pattern = m_patternHandler.getCommandPattern(cmd);
    auto ca_length = m_patternHandler.getPattern(cmd.type).size() / m_commandBus.get_width();
    m_commandBus.load(cmd.timestamp, pattern, ca_length);
}

void LPDDR6Interface::setMetaData(uint16_t metaData) {
    m_metaData = metaData;
}

uint16_t LPDDR6Interface::getMetaData() {
    return m_metaData;
}

const uint8_t* LPDDR6Interface::formatData(const uint8_t* data, std::size_t n_bits, bool read) {
    assert(256 == n_bits && "Invalid LPDDR6 burst");

    uint16_t dbival = 0;
    if (m_dbi.isEnabled()) {
        auto& dbivec = read ? m_dbi.getInversionStateRead() : m_dbi.getInversionStateWrite();
        std::size_t n = std::min(dbivec.size(), static_cast<std::size_t>(16));
        for (std::size_t i = 0; i < n; ++i) {
            dbival |= (static_cast<uint16_t>(dbivec[i]) << i);
        }
    }

    std::fill(m_formatResult.begin(), m_formatResult.end(), 0);

    std::size_t dataBitIdx = 0;
    std::size_t burst = 0;
    std::size_t dq = 0;

    for (int i = 0; i < 288; ++i) {
        bool isSpecial = (dq == 4 || dq == 5 || dq == 10 || dq == 11);
        bool isMeta    = (burst >= 8  && burst < 12) && isSpecial;
        bool isDbi     = (burst >= 20 && burst < 24) && isSpecial;
        bool isData    = !(isMeta || isDbi);


        int group = (((dq >> 2) & 2) | (dq & 1)) * isSpecial; // 4: 0, 5: 1, 10: 2, 11: 3
        int bitOffsetSub = (isMeta * 8) + (isDbi * 20);
        int bitOffset   = (group * 4) + (burst - bitOffsetSub);


        bool metaBit = (m_metaData >> bitOffset) & 1;
        bool dbiBit = (dbival >> bitOffset) & 1;
        bool dataBit = (data[dataBitIdx >> 3] >> (dataBitIdx & 7)) & 1;

        bool bitValue = (isMeta * metaBit) | (isDbi * dbiBit) | (isData * dataBit);

        m_formatResult[i >> 3] |= (static_cast<uint8_t>(bitValue) << (i & 7));
        
        dataBitIdx += isData;

        if (12 == ++dq) {
            dq = 0;
            ++burst;
        }
    }
    return m_formatResult.data();
}

void LPDDR6Interface::handleData(const Command &cmd, bool read) {
    auto loadfunc = read ? &databus_t::loadRead : &databus_t::loadWrite;
    size_t length = 0;
    if (0 == cmd.sz_bits) {
        // No data provided by command
        if (m_dataBus.isTogglingRate()) {
            length = m_memSpec.burstLength;
            (m_dataBus.*loadfunc)(cmd.timestamp, length * m_dataBus.getWidth(), nullptr);
        }
    } else {
        std::optional<const uint8_t *> dbi_data = std::nullopt;
        // Data provided by command
        if (m_dataBus.isBus() && m_dbi.isEnabled()) {
            dbi_data = handleDBIInterface(cmd.timestamp, cmd.sz_bits, cmd.data, read);
        }
        length = cmd.sz_bits / m_dataBus.getWidth();
        (m_dataBus.*loadfunc)(cmd.timestamp, cmd.sz_bits, formatData(dbi_data.value_or(cmd.data), cmd.sz_bits, read));
    }
    // DQS
    if (read) {
        // Read
        m_readDQS.start(cmd.timestamp);
        m_readDQS.stop(cmd.timestamp + length / m_memSpec.dataRate);
        if (!m_memSpec.wckAlwaysOnMode) {
            m_wck.start(cmd.timestamp);
            m_wck.stop(cmd.timestamp + length / m_memSpec.dataRate);
        }
    } else {
        // Write
        if (!m_memSpec.wckAlwaysOnMode) {
            m_wck.start(cmd.timestamp);
            m_wck.stop(cmd.timestamp + length / m_memSpec.dataRate);
        }
    }
    handleOverrides(length, read);
    handleCommandBus(cmd);
}

void LPDDR6Interface::endOfSimulation(timestamp_t timestamp) {
    m_dbi.dispatchResetCallback(timestamp);
}

void LPDDR6Interface::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
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

}

void LPDDR6Interface::serialize(std::ostream& stream) const {
    stream.write(reinterpret_cast<const char*>(&m_last_command_time), sizeof(m_last_command_time));
    m_patternHandler.serialize(stream);
    m_commandBus.serialize(stream);
    m_dataBus.serialize(stream);
    m_readDQS.serialize(stream);
    m_wck.serialize(stream);
    m_clock.serialize(stream);
}

void LPDDR6Interface::deserialize(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&m_last_command_time), sizeof(m_last_command_time));
    m_patternHandler.deserialize(stream);
    m_commandBus.deserialize(stream);
    m_dataBus.deserialize(stream);
    m_readDQS.deserialize(stream);
    m_wck.deserialize(stream);
    m_clock.deserialize(stream);
}

} // namespace DRAMPower
