#include "LPDDR6Interface.h"
#include "DRAMPower/Types.h"
#include "DRAMPower/command/Pattern.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Command.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Pattern.h"
#include "DRAMPower/util/databus_types.h"
#include <algorithm>
#include <functional>
#include <tuple>
#include <cstddef>

namespace DRAMPower {

static constexpr DRAMUtils::Config::ToggleRateDefinition busConfig {
    0,
    0,
    0,
    0,
    DRAMUtils::Config::TogglingRateIdlePattern::L,
    DRAMUtils::Config::TogglingRateIdlePattern::L
};

LPDDR6Interface::LPDDR6Interface(const MemSpecLPDDR6& memSpec, const config::SimConfig &simConfig, bool enabled)
    : m_memSpec(memSpec)
    , m_commandBus{cmdBusWidth, 2, // modelled with datarate 2
        util::BusIdlePatternSpec::L}
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
    , m_readDQS(memSpec.dataRate, true)
    , m_wck(memSpec.dataRate / memSpec.memTimingSpec.WCKtoCK, !enabled || !memSpec.wckAlwaysOnMode)
    , m_clock(2, !enabled)
    , m_dbi(std::nullopt, m_memSpec.burstLength, nullptr, false)
    , m_patternHandler(BasePatternEncoderOverrides<pattern_descriptor_LPDDR6::t>{}) // No overrides
    , m_enabled(enabled)
{
    registerPatterns();
    m_patternHandler.getEncoder().getExtraData().numberOfBankGroups = memSpec.numberOfBankGroups;
}

void LPDDR6Interface::setSimulationTime(timestamp_t timestamp) {
    m_offset = timestamp;
}

void LPDDR6Interface::reset() {
    m_commandBus.reset();
    m_dataBus.reset();
    m_readDQS.reset();
    m_wck.reset();
    m_clock.reset();
    m_dbi.reset();
    m_formatter.reset();
    m_patternHandler.reset();
    m_last_command_time = 0;
}

void LPDDR6Interface::registerPatterns() {
    using namespace pattern_descriptor_LPDDR6;
    using commandPattern_t = std::vector<pattern_descriptor_LPDDR6::t>;
    
    // LPDDR6 needs 2 commands for activation (ACT-1 and ACT-2)
    // ACT-1 must be followed by ACT-2 in almost every case (CAS, WRITE,
    // MASK WRITE and READ commands can be issued inbetween ACT-1 and ACT-2)
    // Here we consider ACT = ACT-1 + ACT-2, not considering interleaving
    // NOP
    commandPattern_t nop_pattern = {
        // R1
        L, L, L, PAR,
        // F1
        L, L, L, V,
        // R2
        X, X, X, X,
        // F2
        X, X, X, X
    };
    m_patternHandler.registerPattern<CmdType::NOP>(nop_pattern);

    // PDE
    commandPattern_t pde_pattern = {
        // R1
        L, L, L, PAR,
        // F1
        L, H, L, V,
        // R2
        X, X, X, X,
        // F2
        X, X, X, X
    };
    m_patternHandler.registerPattern<CmdType::PDEA>(pde_pattern);
    m_patternHandler.registerPattern<CmdType::PDEP>(pde_pattern);
    
    // PDX
    commandPattern_t pdx_pattern = {
        // R1
        V, X, X, X,
        // F1
        V, X, X, X,
        // R2
        V, X, X, X,
        // F2
        V, X, X, X
    };
    m_patternHandler.registerPattern<CmdType::PDXA>(pdx_pattern);
    m_patternHandler.registerPattern<CmdType::PDXP>(pdx_pattern);

    // SRE
    auto sre_gen = [](t PD) -> commandPattern_t {
        return {
            // R1
            L, L, L, PAR,
            // F1
            L, L, H, PD,
            // R2
            X, X, X, X,
            // F2
            X, X, X, X
        };
    };
    // SRE with PD
    m_patternHandler.registerPattern<CmdType::SREFEN>(sre_gen(H));

    // SRX
    commandPattern_t srx_pattern = {
        // R1
        L, L, L, PAR,
        // F1
        L, H, H, V,
        // R2
        X, X, X, X,
        // F2
        X, X, X, X
    };
    m_patternHandler.registerPattern<CmdType::SREFEX>(srx_pattern);

    // PRE / PREA
    auto pre_gen = [](bool AB) -> commandPattern_t {
        return {
            // R1
            L, L, L, PAR,
            // F1
            H, H, V, SC,
            // R2
            V, V, V, AB?H:L,
            // F2
            AB?X:BA0, AB?X:BA1, AB?X:BG0, AB?X:BG1
        };
    };
    m_patternHandler.registerPattern<CmdType::PRE>(pre_gen(false));
    m_patternHandler.registerPattern<CmdType::PREA>(pre_gen(true));

    // REF / REFDB
    auto ref_gen = [](t RFM, bool AB) -> commandPattern_t {
        return {
            // R1
            L, L, L, PAR,
            // F1
            H, L, RFM, SC,
            // R2
            AB?X:DBG0, AB?X:DBG1, V, AB?H:L,
            // F2
            AB?X:BA0, AB?X:BA1, AB?X:BG0, AB?X:BG1
        };
    };
    m_patternHandler.registerPattern<CmdType::REFDB>(ref_gen(L, false));
    m_patternHandler.registerPattern<CmdType::REFA>(ref_gen(L, true));

    // ACT
    commandPattern_t act_pattern = {
        // ACT-1
        // R1
        H, H, H, PAR,
        // F1
        H, R15, R16, SC,
        // R2
        R11, R12, R13, R14,
        // F2
        BA0, BA1, BG0, BG1,
        // ACT-2
        // R1
        H, H, H, PAR,
        // F1
        L, R8, R9, R10,
        // R2
        R4, R5, R6, R7,
        // F2
        R0, R1, R2, R3
    };
    m_patternHandler.registerPattern<CmdType::ACT>(act_pattern);

    // WR / WRA
    auto wr_gen = [](t AP) -> commandPattern_t {
        return {
            // R1
            BL, L, H, PAR,
            // F1
            C0, C1, AP, SC,
            // R2
            C2, C3, C4, C5,
            // F2
            BA0, BA1, BG0, BG1
        };
    };
    m_patternHandler.registerPattern<CmdType::WR>(wr_gen(L));
    m_patternHandler.registerPattern<CmdType::WRA>(wr_gen(H));

    // RD / RDA
    auto rd_gen = [](t AP) -> commandPattern_t {
        return {
            // R1
            BL, H, L, PAR,
            // F1
            C0, C1, AP, SC,
            // R2
            C2, C3, C4, C5,
            // F2
            BA0, BA1, BG0, BG1
        };
    };
    m_patternHandler.registerPattern<CmdType::RD>(rd_gen(L));
    m_patternHandler.registerPattern<CmdType::RDA>(rd_gen(H));
}

timestamp_t LPDDR6Interface::getLastCommandTime() const {
    return m_last_command_time + m_offset;
}

void LPDDR6Interface::doCommand(const LPDDR6Command& cmd) {
    assert(cmd.timestamp >= m_offset);
    timestamp_t timestamp = cmd.timestamp - m_offset;
    switch(cmd.type) {
        case CmdType::NOP:
        case CmdType::PDEA:
        case CmdType::PDEP:
        case CmdType::PDXA:
        case CmdType::PDXP:
        case CmdType::SREFEN:
        case CmdType::SREFEX:
        case CmdType::PRE:
        case CmdType::PREA:
        case CmdType::REFDB:
        case CmdType::REFA:
        case CmdType::ACT:
            handleCommandBus(timestamp, cmd.type, cmd.targetCoordinate);
            break;
        case CmdType::WR:
        case CmdType::WRA:
            handleData(timestamp, cmd.type, cmd.data, cmd.sz_bits, cmd.targetCoordinate, false);
            break;
        case CmdType::RD:
        case CmdType::RDA:
            handleData(timestamp, cmd.type, cmd.data, cmd.sz_bits, cmd.targetCoordinate, true);
            break;
        case CmdType::END_OF_SIMULATION:
            endOfSimulation(timestamp);
            break;
        default:
            assert(false && "Invalid command");
            break;
    }
    m_last_command_time = timestamp;
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

void LPDDR6Interface::handleCommandBus(timestamp_t timestamp, CmdType type, const LPDDR6TargetCoordinate& target) {
    auto pattern = m_patternHandler.getCommandPattern(type, target);
    auto ca_length = m_patternHandler.getPattern(type).size() / m_commandBus.get_width();
    m_commandBus.load(timestamp, pattern, ca_length);
}

void LPDDR6Interface::enable(timestamp_t timestamp) {
    if (m_enabled) return;
    if (m_memSpec.wckAlwaysOnMode) {
        m_wck.start(timestamp);
    }
    m_clock.start(timestamp);
    m_last_command_time = timestamp;
    m_enabled = true;
}

void LPDDR6Interface::disable(timestamp_t timestamp) {
    if (!m_enabled) return;
    if (m_memSpec.wckAlwaysOnMode) {
        m_wck.stop(timestamp);
    }
    m_clock.stop(timestamp);
    m_last_command_time = timestamp;
    m_enabled = false;
}

bool LPDDR6Interface::isEnabled() const {
    return m_enabled;
}

void LPDDR6Interface::setMetaData(uint16_t metaDataB1, uint16_t metaDataB2) {
    m_formatter.m_metaData[0] = metaDataB1;
    m_formatter.m_metaData[1] = metaDataB2;
}

std::tuple<uint16_t, uint16_t> LPDDR6Interface::getMetaData() const {
    return std::make_tuple(m_formatter.m_metaData[0], m_formatter.m_metaData[1]);
}

void LPDDR6Interface::setParityCheckMode(bool state) {
    // TODO WCK Always ON mode must be set active when CA parity is enabled
    assert(((m_memSpec.wckAlwaysOnMode && state) || !state) && "Invalid wckAlwaysOnMode");
    m_patternHandler.getEncoder().getExtraData().parity_check_mode = state;
}
bool LPDDR6Interface::getParityCheckMode() const {
    return m_patternHandler.getEncoder().getExtraData().parity_check_mode;
}

void LPDDR6Interface::handleData(timestamp_t timestamp, CmdType type, const uint8_t* data, std::size_t sz_bits, const LPDDR6TargetCoordinate& target, bool read) {
    auto loadfunc = read ? &databus_t::loadRead : &databus_t::loadWrite;
    size_t length = 0;
    if (0 == sz_bits) {
        // No data provided by command
        if (m_dataBus.isTogglingRate()) {
            length = m_memSpec.burstLength;
            (m_dataBus.*loadfunc)(timestamp, length * m_dataBus.getWidth(), nullptr);
        }
    } else {
        std::optional<const uint8_t *> dbi_data = std::nullopt;
        // Data provided by command
        if (m_dataBus.isBus() && m_dbi.isEnabled()) {
            dbi_data = handleDBIInterface(timestamp, sz_bits, data, read);
        }
        auto[data_ptr, data_ptr_sz_bits] = m_formatter.formatData(
            dbi_data.value_or(data), sz_bits,
            m_dbi.isEnabled() ?
                read ? std::make_optional(m_dbi.getInversionStateRead()) : std::make_optional(m_dbi.getInversionStateWrite())
                : std::nullopt);
        (m_dataBus.*loadfunc)(timestamp, data_ptr_sz_bits, data_ptr);
        length = data_ptr_sz_bits / m_dataBus.getWidth();
    }
    // DQS
    if (read) {
        // Read
        m_readDQS.start(timestamp);
        m_readDQS.stop(timestamp + length / m_memSpec.dataRate);
        if (!m_memSpec.wckAlwaysOnMode) {
            m_wck.start(timestamp);
            m_wck.stop(timestamp + length / m_memSpec.dataRate);
        }
    } else {
        // Write
        if (!m_memSpec.wckAlwaysOnMode) {
            m_wck.start(timestamp);
            m_wck.stop(timestamp + length / m_memSpec.dataRate);
        }
    }
    m_patternHandler.getEncoder().getExtraData().currentBurstLength = length;
    handleCommandBus(timestamp, type, target);
}

void LPDDR6Interface::endOfSimulation(timestamp_t timestamp) {
    m_dbi.dispatchResetCallback(timestamp);
}

void LPDDR6Interface::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
    assert(timestamp >= m_offset);
    timestamp = timestamp - m_offset;
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
    stream.write(reinterpret_cast<const char*>(&m_offset), sizeof(m_offset));
    m_patternHandler.serialize(stream);
    m_commandBus.serialize(stream);
    m_dataBus.serialize(stream);
    m_readDQS.serialize(stream);
    m_wck.serialize(stream);
    m_clock.serialize(stream);
}

void LPDDR6Interface::deserialize(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&m_last_command_time), sizeof(m_last_command_time));
    stream.read(reinterpret_cast<char*>(&m_offset), sizeof(m_offset));
    m_patternHandler.deserialize(stream);
    m_commandBus.deserialize(stream);
    m_dataBus.deserialize(stream);
    m_readDQS.deserialize(stream);
    m_wck.deserialize(stream);
    m_clock.deserialize(stream);
}

std::tuple<const uint8_t*, std::size_t> LPDDR6Interface::DataFormatter::formatData(
    const uint8_t* inputData, std::size_t n_bits,
    std::optional<std::reference_wrapper<const std::vector<bool>>> InversionState
) {
    assert(((dataBitsPerBurst - additionalDataPerBurst) == n_bits
            || (2 * (dataBitsPerBurst - additionalDataPerBurst)) == n_bits)
            && "Invalid burst");
    std::size_t nbursts = (2 * (dataBitsPerBurst - additionalDataPerBurst) == n_bits) ? 2 : 1;
    
    // Toggling rate
    if (nullptr == inputData) {
        return std::make_tuple(nullptr, n_bits + nbursts * additionalDataPerBurst);
    }

    // Reserve
    std::size_t requiredBytes = nbursts * dataBytesPerBurst;
    if (m_formatResult.size() < requiredBytes) {
        m_formatResult.resize(requiredBytes);
    }
    std::fill_n(m_formatResult.begin(), requiredBytes, 0);

    uint32_t dbival = 0;
    if (InversionState.has_value()) {
        std::size_t n = std::min(InversionState->get().size(), static_cast<std::size_t>(32));
        for (std::size_t i = 0; i < n; ++i) {
            dbival |= (static_cast<uint16_t>(InversionState->get()[i]) << i);
        }
    }

    std::size_t dataBitIdx;
    std::size_t burst;
    std::size_t dq;

    for (std::size_t burst24 = 0; burst24 < nbursts; ++burst24) {
        dataBitIdx = 0;
        burst = 0;
        dq = 0;
        for (int i = 0; i < 288; ++i) {
            bool isSpecialRow = (dq == 4 || dq == 5 || dq == 10 || dq == 11);
            bool isMetaColumn = (burst >= 8  && burst < 12);
            bool isMeta       = isMetaColumn && isSpecialRow;
            bool isDbiColumn  = (burst >= 20 && burst < 24);
            bool isDbi        = isDbiColumn && isSpecialRow;
            bool isData       = !(isMeta || isDbi);


            int group = (((dq >> 2) & 2) | (dq & 1)) * isSpecialRow; // 4: 0, 5: 1, 10: 2, 11: 3
            int bitOffsetSub = (isMeta * 8) + (isDbi * 20);
            int bitOffset   = (group * 4) + (burst - bitOffsetSub);

            int mapping = dataBitsPerBurstNoMetaNoDBI * burst24 + dataBitMapping[dataBitIdx]; 
            bool dataBit = (inputData[mapping >> 3] >> (mapping & 7)) & 1;
            bool metaBit = (m_metaData[burst24] >> bitOffset) & 1;
            bool dbiBit = (dbival >> ((16 * burst24) + bitOffset)) & 1;

            bool bitValue = (dbiBit && isDbi) || (metaBit && isMeta) || (dataBit && isData);

            size_t outputindex = nbursts * dataBitsPerBurst - 1 - (burst24 * dataBitsPerBurst + i);
            m_formatResult[outputindex >> 3] |= (static_cast<uint8_t>(bitValue) << (outputindex & 7));

            dataBitIdx += isData;

            if (12 == ++dq) {
                dq = 0;
                ++burst;
            }
        }
    }
    return std::make_tuple(m_formatResult.data(), nbursts * 288);
}

void LPDDR6Interface::DataFormatter::reset() {
    m_formatResult.clear();
    m_metaData = {0, 0};
}

} // namespace DRAMPower
