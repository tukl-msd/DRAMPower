#ifndef DRAMPOWER_STANDARDS_DDR5_DDR5INTERFACE_H
#define DRAMPOWER_STANDARDS_DDR5_DDR5INTERFACE_H

#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/databus_presets.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/Serialize.h"
#include "DRAMPower/util/Deserialize.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/data/stats.h"

#include "DRAMPower/util/PatternHandler.h"

#include "DRAMPower/memspec/MemSpecDDR5.h"

#include "DRAMUtils/config/toggling_rate.h"

#include <stdint.h>
#include <cstddef>

namespace DRAMPower {

class DDR5Interface : public util::Serialize, public util::Deserialize {
// Public constants
public:
    const static std::size_t cmdBusWidth = 14;
    const static uint64_t cmdBusInitPattern = (1<<cmdBusWidth)-1;

// Public type definitions
public:
    using commandbus_t = util::Bus<cmdBusWidth>;
    using databus_t = util::databus_presets::databus_preset_t;
    using patternHandler_t = PatternHandler<CmdType>;

// Public constructors and assignment operators
public:
    DDR5Interface() = delete; // no default constructor
    DDR5Interface(const DDR5Interface&) = default; // copy constructor
    DDR5Interface& operator=(const DDR5Interface&) = delete; // copy assignment operator
    DDR5Interface(DDR5Interface&&) = default; // move constructor
    DDR5Interface& operator=(DDR5Interface&&) = delete; // move assignment operator

    DDR5Interface(const MemSpecDDR5& memSpec, const DRAMUtils::Config::ToggleRateDefinition& toggleRate);

// Public member functions
public:
// Member functions
    timestamp_t getLastCommandTime() const;
    void doCommand(const Command& cmd);
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;
// Overrides
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;

// Private member functions
private:
    void registerPatterns();
    void handleOverrides(size_t length, bool read);
    void handleDQs(const Command& cmd, util::Clock &dqs, size_t length);
    void handleCommandBus(const Command& cmd);
    void handleData(const Command &cmd, bool read);

// Private member variables
private:
    const MemSpecDDR5& m_memSpec;
    commandbus_t m_commandBus;
    databus_t m_dataBus;
    util::Clock m_readDQS;
    util::Clock m_writeDQS;
    util::Clock m_clock;
    patternHandler_t m_patternHandler;
    timestamp_t m_last_command_time = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR5_DDR5INTERFACE_H */
