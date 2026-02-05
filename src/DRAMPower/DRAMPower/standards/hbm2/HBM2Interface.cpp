#include "HBM2Interface.h"
#include "DRAMPower/command/CmdType.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/command/Pattern.h"
#include "DRAMPower/memspec/MemSpecHBM2.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/pin.h"
#include "DRAMPower/util/pin_types.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iostream>

namespace DRAMPower {

    bool isIncreasedSize(const MemSpecHBM2 & memSpec) {
        if ((memSpec.numberOfStacks > (1 << 1))
            || (memSpec.numberOfRows > (1 << 14))
        ) {
            return true;
        }
        return false;
    }

    std::size_t HBM2Interface::getRowWidth(const MemSpecHBM2& memSpec) {
        // 7 if rows[14] or SID[1]
        // 6 otherwise
        return (isIncreasedSize(memSpec) ? maxRowCmdBusWidth : minRowCmdBusWidth);
    }

    std::size_t HBM2Interface::getColumnWidth(const MemSpecHBM2& memSpec) {
        // 9 if rows[14] or SID[1]
        // 8 otherwise
        return (isIncreasedSize(memSpec) ? maxColumnCmdBusWidth : minColumnCmdBusWidth);
    }

    HBM2Interface::HBM2Interface(const MemSpecHBM2& memSpec, implicitCommandInserter_t&& implicitCommandInserter)
    : m_columnCommandBus{getColumnWidth(memSpec), memSpec.dataRate,
        util::BusIdlePatternSpec::H, util::BusInitPatternSpec::H}
    , m_rowCommandBus{getRowWidth(memSpec), memSpec.dataRate,
        util::BusIdlePatternSpec::H, util::BusInitPatternSpec::H}
    , m_cke(util::PinState::H)
    , m_dataBus{memSpec.numberOfPseudoChannels, {
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
        ),
        util::Clock {
            memSpec.dataRate, true
        },
        util::Clock {
            memSpec.dataRate, true
        }
    }}
    , m_clock(2, false)
    , m_memSpec(memSpec)
    , m_dbi(memSpec.numberOfDevices * memSpec.bitWidth, m_memSpec.burstLength,
        [this](timestamp_t load_timestamp, timestamp_t chunk_timestamp, std::size_t pin, bool inversion_state, bool read) {
            std::cout << "Toggle DBI PIN load_timestamp: " << load_timestamp << ",chunk_timestamp: " << chunk_timestamp << ",Pin: " << pin << ",State: " << inversion_state << ",Read: " << read << "\n";
        this->handleDBIPinChange(load_timestamp, chunk_timestamp, pin, inversion_state, read);
    }, false)
    , m_dbiread(m_dbi.getChunksPerWidth().value(), util::Pin{m_dbi.getIdlePattern()})
    , m_dbiwrite(m_dbi.getChunksPerWidth().value(), util::Pin{m_dbi.getIdlePattern()})
    , m_patternHandler(PatternEncoderOverrides {
        {pattern_descriptor::V, PatternEncoderBitSpec::H},
        {pattern_descriptor::C0, PatternEncoderBitSpec::H},
        {pattern_descriptor::X, PatternEncoderBitSpec::H},
        {pattern_descriptor::PAR, PatternEncoderBitSpec::H},
    })
    , m_implicitCommandInserter(std::move(implicitCommandInserter))
    {
        registerPatterns();
    }

    void HBM2Interface::registerPatterns() {
        using namespace pattern_descriptor;
        // R_SID0
        pattern_descriptor::t R_SID0 = V;
        if (m_memSpec.numberOfStacks > 1) {
            R_SID0 = SID0;
        }
        // R_SID1
        pattern_descriptor::t R_SID1 = V;
        if (m_memSpec.numberOfStacks > 2) {
            R_SID1 = SID1;
        }
        // R_BA4
        t R_BA4 = V;
        if (m_memSpec.numberOfPseudoChannels > 1) {
            R_BA4 = PC0;
        }

        // C_C0
        t C_C0 = C0;
        if (m_memSpec.numberOfStacks > 1) { // TODO verify
            C_C0 = SID0;
        }
        // C8
        t C_C8 = R_SID1;
        // C_BA4
        t C_BA4 = R_BA4;
        
        // middleware for removing the 6th pin of the bus
        bool remove_r7 = !isIncreasedSize(m_memSpec);
        bool remove_c8 = !isIncreasedSize(m_memSpec);
        auto remove_pin_middleware = [](bool remove, std::size_t pin_idx, std::initializer_list<pattern_descriptor::t> list) {
            std::vector<pattern_descriptor::t> result;
            result.reserve(list.size());

            std::size_t count = 0;
            for (auto const &entry : list) {
                // remove implies pin_check
                if ((!remove) || (count % pin_idx != 0)) {
                    result.push_back(entry);
                }
                ++count;
            }
            return result;
        };
        auto remove_r7_middleware = [remove_r7, &remove_pin_middleware](std::initializer_list<pattern_descriptor::t> list){
            return remove_pin_middleware(remove_r7, maxRowCmdBusWidth - 1, list);
        };
        auto remove_c8_middleware = [remove_c8, &remove_pin_middleware](std::initializer_list<pattern_descriptor::t> list){
            return remove_pin_middleware(remove_c8, maxColumnCmdBusWidth - 1, list);
        };

        // Row CommandBus
        // ACT
        m_patternHandler.registerPattern<CmdType::ACT>(remove_r7_middleware({
            L,   H,   R_SID0, BA0,  BA1,  BA2,  R14,
            R11, R12, PAR,  R_BA4, R13, BA3, V,
            R5, R6, R7,  R8,  R9,  R10, V,
            R0, R1, PAR, R2,  R3,  R4,  V
        }));
        // PRE
        m_patternHandler.registerPattern<CmdType::PRE>(remove_r7_middleware({
            H, H, L, BA0, BA1, BA2, R_SID1,
            V, R_SID0, PAR, R_BA4, L, BA3, V
        }));
        // PREA
        m_patternHandler.registerPattern<CmdType::PREA>(remove_r7_middleware({
            H, H, L, V, V, V, V,
            V, V, PAR, R_BA4, H, V, V
        }));
        // REFB
        m_patternHandler.registerPattern<CmdType::REFB>(remove_r7_middleware({
            L, L, H, BA0, BA1, BA2, R_SID1,
            V, R_SID0, PAR, R_BA4, L, BA3, V
        }));
        // REF
        m_patternHandler.registerPattern<CmdType::REFA>(remove_r7_middleware({
            L, L, H, V, V, V, V,
            V, V, PAR, R_BA4, H, V, V
        }));
        // PDE
        auto PDE = remove_r7_middleware({
            H, H, H, V, V, V, V,
            V, V, PAR, V, V, V, V
        });
        m_patternHandler.registerPattern<CmdType::PDEA>(PDE);
        m_patternHandler.registerPattern<CmdType::PDEP>(PDE);
        // SRE
        m_patternHandler.registerPattern<CmdType::SREFEN>(remove_r7_middleware({
            L, L, H, V, V, V, V,
            V, V, PAR, V, V, V, V
        }));
        // PDX / SRX
        auto PDX = remove_r7_middleware({
            H, H, H, V, V, V, V,
            V, V, V, V, V, V, V
        });
        m_patternHandler.registerPattern<CmdType::PDXA>(PDX);
        m_patternHandler.registerPattern<CmdType::PDXP>(PDX);
        m_patternHandler.registerPattern<CmdType::SREFEX>(PDX);

        // Column Commandbus
        // RD
        m_patternHandler.registerPattern<CmdType::RD>(remove_c8_middleware({
            H, L, H, L, BA0, BA1, BA2, BA3, C_C8,
            C_C0, C1, PAR, C2, C3, C4, C5, C_BA4, V
        }));
        // RDA
        m_patternHandler.registerPattern<CmdType::RDA>(remove_c8_middleware({
            H, L, H, H, BA0, BA1, BA2, BA3, C_C8,
            C_C0, C1, PAR, C2, C3, C4, C5, C_BA4, V
        }));
        // WR
        m_patternHandler.registerPattern<CmdType::WR>(remove_c8_middleware({
            H, L, L, L, BA0, BA1, BA2, BA3, C_C8,
            C_C0, C1, PAR, C2, C3, C4, C5, C_BA4, V
        }));
        // WRA
        m_patternHandler.registerPattern<CmdType::WRA>(remove_c8_middleware({
            H, L, L, L, BA0, BA1, BA2, BA3, C_C8,
            C_C0, C1, PAR, C2, C3, C4, C5, C_BA4, V
        }));
        // NOP
        m_patternHandler.registerPattern<CmdType::NOP>(remove_c8_middleware({
            H, H, H, V, V, V, V, V, V,
            V, V, PAR, V, V, V, V, V, V
        }));
    }

    // Update toggling rate
    void HBM2Interface::enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp) {
        // Change from bus to toggling rate
        assert(enable_timestamp >= timestamp);
        auto callback = [this, enable_timestamp]() {
            std::for_each(m_dataBus.begin(), m_dataBus.end(), [enable_timestamp](auto& entry){
                entry.m_dataBus.enableTogglingRate(enable_timestamp);
            });
        };
        if ( enable_timestamp > timestamp ) {
            // Schedule toggling rate enable
            m_implicitCommandInserter.addImplicitCommand(enable_timestamp, callback);
        } else {
            callback();
        }
    }

    void HBM2Interface::enableBus(timestamp_t timestamp, timestamp_t enable_timestamp) {
        // Change from toggling rate to bus
        assert(enable_timestamp >= timestamp);
        auto callback = [this, enable_timestamp]() {
            std::for_each(m_dataBus.begin(), m_dataBus.end(), [enable_timestamp](auto& entry){
                entry.m_dataBus.enableBus(enable_timestamp);
            });
        };
        if ( enable_timestamp > timestamp ) {
            // Schedule toggling rate disable
            m_implicitCommandInserter.addImplicitCommand(enable_timestamp, callback);
        } else {
            callback();
        }
    }

    timestamp_t HBM2Interface::updateTogglingRate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleratedefinition) {
        timestamp_t g_enable_timestamp = timestamp;
        auto updateToggleRateBus = [this, timestamp, &toggleratedefinition, &g_enable_timestamp](databusContainer_t &dataBusContainer) {
            auto& dataBus = dataBusContainer.m_dataBus;
            timestamp_t enable_timestamp = std::max(timestamp, dataBus.lastBurst());
            if (toggleratedefinition) {
                dataBus.setTogglingRateDefinition(*toggleratedefinition);
                if (dataBus.isTogglingRate()) {
                    // toggling rate already enabled
                    return;
                }
                // Enable toggling rate
                enableTogglingHandle(timestamp, enable_timestamp);
            } else {
                if (dataBus.isBus()) {
                    // Bus already enabled
                    return;
                }
                // Enable bus
                enableBus(timestamp, enable_timestamp);
            }
            g_enable_timestamp = std::max(g_enable_timestamp, enable_timestamp);
        };
        std::for_each(m_dataBus.begin(), m_dataBus.end(), updateToggleRateBus);
        return g_enable_timestamp;
    }

    void HBM2Interface::handleDBIPinChange(const timestamp_t load_timestamp, timestamp_t chunk_timestamp, std::size_t pin, bool state, bool read) {
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
            m_implicitCommandInserter.addImplicitCommand(chunk_timestamp / m_memSpec.dataRate, updatePinCallback);
        } else {
            // chunk_timestamp <= load_timestamp
            updatePinCallback();
        }
    }

    std::optional<const uint8_t *> HBM2Interface::handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read) {
        if (0 == n_bits || !data || !m_dbi.isEnabled()) {
            // No DBI or no data to process
            return std::nullopt;
        }
        timestamp_t virtual_time = timestamp * m_memSpec.dataRate;
        // updateDBI calls the given callback to handle pin changes
        return m_dbi.updateDBI(virtual_time, n_bits, data, read);
    }

    void HBM2Interface::handleColumnCommandBus(const Command &cmd) {
        // NOTE: NOP pattern is not registered in the patternHandler
        using namespace pattern_descriptor;
        const static std::initializer_list<t> nop_pattern = {
            H, H, H, V, V, V, V, V, V,
            V, V, PAR, V, V, V, V, V, V
        };
        uint64_t pattern = 0;
        if (CmdType::NOP == cmd.type) {
            pattern = m_patternHandler.getEncoder().encode(cmd, nop_pattern);
        } else {
            pattern = m_patternHandler.getCommandPattern(cmd);
        }
        auto ca_length = m_patternHandler.getPattern(cmd.type).size() / m_columnCommandBus.get_width();
        auto corrected_timestamp = cmd.timestamp + ca_length - 1; // Command executed on last beat
        assert(corrected_timestamp >= 0 && "Invalid timestamp: Respect the command length constraints");
        assert(ca_length > 0 && "Invalid command registered in DRAMPower");
        
        this->m_columnCommandBus.load(corrected_timestamp, pattern, ca_length);
        // For all column commands cke is H
        assert(util::PinState::H == m_cke.get(corrected_timestamp)
            && "Invalid cke pin state for column command"
        );
    }

    void HBM2Interface::handleRowCommandBus(const Command &cmd) {
        // NOTE: NOP pattern is not registered in the patternHandler
        using namespace pattern_descriptor;
        const static std::initializer_list<t> nop_pattern = {
            H, H, H, V, V, V, V,
            V, V, PAR, V, V, V, V,
        };
        uint64_t pattern = 0;
        if (CmdType::NOP == cmd.type) {
            pattern = m_patternHandler.getEncoder().encode(cmd, nop_pattern);
        } else {
            pattern = m_patternHandler.getCommandPattern(cmd);
        }
        auto ca_length = m_patternHandler.getPattern(cmd.type).size() / m_rowCommandBus.get_width();
        auto corrected_timestamp = cmd.timestamp + ca_length - 1; // Command executed on last beat
        assert(corrected_timestamp >= 0 && "Invalid timestamp for cmd. The command length must be respected");
        assert(ca_length > 0 && "Invalid command registered in DRAMPower");

        this->m_rowCommandBus.load(corrected_timestamp, pattern, ca_length);

        // handle CKE
        // Low for PDE and SRE
        switch(cmd.type) {
            case CmdType::PDEA:
            case CmdType::PDEP:
            case CmdType::SREFEN:
                m_cke.set(corrected_timestamp, util::PinState::L);
                break;
            default:
                m_cke.set(corrected_timestamp, util::PinState::H);
                break;
        }
    }

    void HBM2Interface::handleDQs(const Command &cmd, util::Clock &dqs, const size_t length) {
        dqs.start(cmd.timestamp);
        dqs.stop(cmd.timestamp + length / m_memSpec.dataRate);
    }

    void HBM2Interface::handleData(const Command &cmd, bool read) {
        auto busDataCallback = [this, &cmd, read](databusContainer_t &dataBusContainer){
            auto &dataBus = dataBusContainer.m_dataBus;
            auto &writeDQS = dataBusContainer.m_writeDQS;
            auto &readDQS = dataBusContainer.m_readDQS;
            
            auto loadfunc = read ? &databus_t::loadRead : &databus_t::loadWrite;
            util::Clock &dqs = read ? readDQS : writeDQS;
            size_t length = 0;
            if (0 == cmd.sz_bits) {
                // No data provided by command
                // Use default burst length
                if (dataBus.isTogglingRate()) {
                    // If bus is enabled skip loading data
                    length = m_memSpec.burstLength;
                    (dataBus.*loadfunc)(cmd.timestamp, length * dataBus.getWidth(), nullptr);
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
            handleDQs(cmd, dqs, length);
        };
        assert(m_dataBus.size() > cmd.targetCoordinate.pseudoChannel
            && "Invalid pseudoChannel in targetCoordinate"
        );
        busDataCallback(m_dataBus.at(cmd.targetCoordinate.pseudoChannel));
        handleColumnCommandBus(cmd);
    }

    void HBM2Interface::endOfSimulation(timestamp_t timestamp) {
        m_dbi.dispatchResetCallback(timestamp);
    }

    void HBM2Interface::getWindowStats(timestamp_t timestamp, SimulationStats &stats) const {
        stats.commandBus += 
            m_columnCommandBus.get_stats(timestamp)
            + m_rowCommandBus.get_stats(timestamp);

        assert(0 == m_memSpec.bitWidth % DWORD
            && "dataBus bitwidth is not a multiple of DWORD"
        );
        std::size_t NumDQsPairs = (m_memSpec.bitWidth * m_memSpec.numberOfDevices) / DWORD;
        auto getStatsCallback = [timestamp, &stats, NumDQsPairs](const databusContainer_t& container) {
            container.m_dataBus.get_stats(timestamp,
                stats.readBus,
                stats.writeBus,
                stats.togglingStats.read,
                stats.togglingStats.write
            );
            stats.readDQSStats += NumDQsPairs * 2u * container.m_readDQS.get_stats_at(timestamp);
            stats.writeDQSStats += NumDQsPairs * 2u * container.m_writeDQS.get_stats_at(timestamp);
        };
        std::for_each(m_dataBus.begin(), m_dataBus.end(), getStatsCallback);

        // single line stored in stats
        // differential power calculated in interface calculation
        stats.clockStats += 2u * m_clock.get_stats_at(timestamp);

        auto pinTempChangeCreator = [this] (bool read, timestamp_t timestamp, util::PinState idlePinState) -> std::optional<util::PinTempChange> {
            auto burstend = m_dbi.getLastBurstEnd(read);
            if (burstend && *burstend < timestamp * m_memSpec.dataRate) {
                return util::PinTempChange {
                    *burstend,
                    idlePinState
                };
            }
            return std::nullopt;
        };
        auto burstEndRead = pinTempChangeCreator(true, timestamp, m_dbi.getIdlePattern());
        auto burstEndWrite = pinTempChangeCreator(false, timestamp, m_dbi.getIdlePattern());
        for (const auto &dbi_pin : m_dbiread) {
            stats.readDBI += dbi_pin.get_stats_at(timestamp, 2, burstEndRead);
        }
        for (const auto &dbi_pin : m_dbiwrite) {
            stats.writeDBI += dbi_pin.get_stats_at(timestamp, 2, burstEndWrite);
        }

        stats.cke += m_cke.get_stats_at(timestamp);
    }

    void HBM2Interface::serialize(std::ostream& stream) const {
        m_patternHandler.serialize(stream);
        m_rowCommandBus.serialize(stream);
        m_columnCommandBus.serialize(stream);
        for (const auto& bus : m_dataBus) {
            bus.m_dataBus.serialize(stream);
            bus.m_readDQS.serialize(stream);
            bus.m_writeDQS.serialize(stream);
        }
        m_clock.serialize(stream);
        m_dbi.serialize(stream);
        for (const auto& pin : m_dbiread) {
            pin.serialize(stream);
        }
        for (const auto& pin : m_dbiwrite) {
            pin.serialize(stream);
        }
        m_cke.serialize(stream);
    }

    void HBM2Interface::deserialize(std::istream& stream) {
        m_patternHandler.deserialize(stream);
        m_rowCommandBus.deserialize(stream);
        m_columnCommandBus.deserialize(stream);
        for (auto& bus : m_dataBus) {
            bus.m_dataBus.deserialize(stream);
            bus.m_readDQS.deserialize(stream);
            bus.m_writeDQS.deserialize(stream);
        }
        m_clock.deserialize(stream);
        m_dbi.deserialize(stream);
        for (auto &pin : m_dbiread) {
            pin.deserialize(stream);
        }
        for (auto &pin : m_dbiwrite) {
            pin.deserialize(stream);
        }
        m_cke.deserialize(stream);
    }

} // namespace DRAMPower
