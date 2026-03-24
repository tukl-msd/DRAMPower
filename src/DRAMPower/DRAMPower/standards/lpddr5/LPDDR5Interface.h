#ifndef DRAMPOWER_STANDARDS_LPDDR5_LPDDR5INTERFACE_H
#define DRAMPOWER_STANDARDS_LPDDR5_LPDDR5INTERFACE_H

#include "DRAMPower/util/pin.h"
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

#include "DRAMPower/memspec/MemSpecLPDDR5.h"

#include "DRAMPower/simconfig/simconfig.h"

#include <stdint.h>
#include <cstddef>
#include <vector>

namespace DRAMPower {

class LPDDR5Interface : public util::Serialize, public util::Deserialize {
// Public constants
public:
    const static std::size_t cmdBusWidth = 7;
    const static uint64_t cmdBusInitPattern = (1<<cmdBusWidth)-1;

// Public type definitions
public:
    using commandbus_t = util::Bus<cmdBusWidth>;
    using pin_dbi_t = util::Pin<32>; // max_burst_length = 32
    using databus_t = util::databus_presets::databus_preset_t;
    using patternHandler_t = PatternHandler<CmdType>;

// Public constructors and assignment operators
public:
    LPDDR5Interface() = delete; // no default constructor
    LPDDR5Interface(const LPDDR5Interface&) = default; // copy constructor
    LPDDR5Interface& operator=(const LPDDR5Interface&) = delete; // copy assignment operator
    LPDDR5Interface(LPDDR5Interface&&) = default; // move constructor
    LPDDR5Interface& operator=(LPDDR5Interface&&) = delete; // move assignment operator

    LPDDR5Interface(const MemSpecLPDDR5& memSpec, const config::SimConfig& simConfig);

// Public member functions
public:
// Member functions
    timestamp_t getLastCommandTime() const;
    void doCommand(const Command& cmd);
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;
// Override
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;
// Extensions
    void enableDBI(bool enable) {
        m_dbi.enable(enable);
    }

// Public member functions
private:
    void registerPatterns();
    std::optional<const uint8_t *> handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read);
    void handleDBIPinChange(const timestamp_t load_timestamp, std::size_t pin, bool state, bool read);
    void handleOverrides(size_t length, bool read);
    void handleDQs(const Command& cmd, util::Clock &dqs, size_t length, uint64_t datarate);
    void handleCommandBus(const Command& cmd);
    void handleData(const Command &cmd, bool read);
    void endOfSimulation(timestamp_t timestamp);

// Private member variables
private:
    const MemSpecLPDDR5& m_memSpec;
    commandbus_t m_commandBus;
    databus_t m_dataBus;
    util::Clock m_readDQS;
    util::Clock m_wck;
    util::Clock m_clock;
    util::DBI<uint8_t, 1, util::PinState::L, util::StaticDBI> m_dbi;
    std::vector<pin_dbi_t> m_dbiread;
    std::vector<pin_dbi_t> m_dbiwrite;
    patternHandler_t m_patternHandler;
    timestamp_t m_last_command_time = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR5_LPDDR5INTERFACE_H */
