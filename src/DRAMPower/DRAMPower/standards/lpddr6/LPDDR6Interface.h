#ifndef DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H
#define DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H

#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/databus_presets.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/Serialize.h"
#include "DRAMPower/util/Deserialize.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"
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
    using patternHandler_t = PatternHandler<CmdType>;

// Public constructor
public:
    LPDDR6Interface(const MemSpecLPDDR6 &memSpec, const config::SimConfig &simConfig);

// Public member functions
public:
// Member functions
    timestamp_t getLastCommandTime() const;
    void doCommand(const Command& cmd);
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;
// Overrides
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;
// Extensions
    void enableDBI(bool enable) {
        m_dbi.enable(enable);
    }
    void setMetaData(uint16_t metaData);
    uint16_t getMetaData();

// Private member functions
private:
    void registerPatterns();
    std::optional<const uint8_t *> handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read);
    void handleOverrides(size_t length, bool read);
    void handleDQs(const Command& cmd, util::Clock &dqs, size_t length);
    void handleCommandBus(const Command& cmd);
    void handleData(const Command &cmd, bool read);
    const uint8_t* formatData(const uint8_t* data, std::size_t n_bits, bool read);
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
    uint16_t m_metaData = 0;
    patternHandler_t m_patternHandler;
    timestamp_t m_last_command_time;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H */
