#ifndef DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H
#define DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H

#include "DRAMPower/standards/lpddr6/LPDDR6Pattern.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/databus_presets.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/Serialize.h"
#include "DRAMPower/util/Deserialize.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Command.h"
#include "DRAMPower/data/stats.h"

#include "DRAMPower/util/PatternHandler.h"
#include "DRAMPower/util/dbi.h"

#include "DRAMPower/memspec/MemSpecLPDDR6.h"

#include "DRAMPower/simconfig/simconfig.h"

#include <stdint.h>
#include <cstddef>

namespace DRAMPower {

struct LPDDR6InterfaceMemSpec {
    LPDDR6InterfaceMemSpec(const MemSpecLPDDR6& memSpec)
        : dataRate(memSpec.dataRate)
        , burstLength(memSpec.burstLength)
        , bitWidth(memSpec.bitWidth)
        , bank_arch(memSpec.bank_arch)
        , wckAlwaysOnMode(memSpec.wckAlwaysOnMode)
    {}

    uint64_t dataRate;
    uint64_t burstLength;
    uint64_t bitWidth;
    MemSpecLPDDR6::BankArchitectureMode bank_arch;
    bool wckAlwaysOnMode;
};

class LPDDR6Interface : public util::Serialize, public util::Deserialize {
// Public constants
public:
    const static std::size_t cmdBusWidth = 4;
    const static uint64_t cmdBusInitPattern = 0;

// Public type definitions
public:
    using commandbus_t = util::Bus<cmdBusWidth>;
    using databus_t = util::databus_presets::databus_preset_t;
    using patternHandler_t = PatternHandler<CmdType, pattern_descriptor_LPDDR6::t, LPDDR6TargetCoordinate, LPDDR6Encoder, LPDDR6PatternExtraData>;

// Public constructor
public:
    LPDDR6Interface(const MemSpecLPDDR6 &memSpec, const config::SimConfig &simConfig, bool enabled = true);

// Public member functions
public:
// Member functions
    timestamp_t getLastCommandTime() const;
    void doCommand(const LPDDR6Command& cmd);
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;
// Overrides
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;
// Extensions
    void enable(timestamp_t timestamp);
    void disable(timestamp_t timestamp);
    bool isEnabled() const;
    void enableDBI(bool enable) {
        m_dbi.enable(enable);
    }
    void setMetaDataB1(uint16_t metaData);
    void setMetaDataB2(uint16_t metaData);
    uint16_t getMetaDataB1() const;
    uint16_t getMetaDataB2() const;
    void setParityCheckMode(bool state);
    bool getParityCheckMode() const;

// Private member functions
private:
    void registerPatterns();
    std::optional<const uint8_t *> handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read);
    void handleDQs(const LPDDR6Command& cmd, util::Clock &dqs, size_t length);
    void handleCommandBus(const LPDDR6Command& cmd);
    void handleData(const LPDDR6Command& cmd, bool read);
    std::tuple<const uint8_t*, std::size_t> formatData(const uint8_t* data, std::size_t n_bits, bool read);
    void endOfSimulation(timestamp_t timestamp);

// Private members
    LPDDR6InterfaceMemSpec m_memSpec;
    commandbus_t m_commandBus;
    databus_t m_dataBus;
    util::Clock m_readDQS;
    util::Clock m_wck;
    util::Clock m_clock;
    util::DBI<uint8_t, 2, util::PinState::L, util::StaticDBI> m_dbi;
    std::vector<uint8_t> m_formatResult;
    uint16_t m_metaData[2] = {0, 0};
    patternHandler_t m_patternHandler;
    timestamp_t m_last_command_time;
    bool m_enabled;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H */
