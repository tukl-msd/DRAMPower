#ifndef DRAMPOWER_STANDARDS_LPDDR4_LPDDR4INTERFACE_H
#define DRAMPOWER_STANDARDS_LPDDR4_LPDDR4INTERFACE_H

#include "DRAMPower/util/pin.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/databus_presets.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/RegisterHelper.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"

#include "DRAMPower/util/PatternHandler.h"
#include "DRAMPower/util/ImplicitCommandHandler.h"
#include "DRAMPower/util/dbi.h"

#include "DRAMPower/memspec/MemSpecLPDDR4.h"

#include "DRAMUtils/config/toggling_rate.h"

#include <functional>
#include <stdint.h>
#include <cstddef>
#include <vector>

namespace DRAMPower {

class LPDDR4Interface {
// Public constants
public:
    const static std::size_t cmdBusWidth = 6;
    const static uint64_t cmdBusInitPattern = (1<<cmdBusWidth)-1;

// Public type definitions
public:
    using commandbus_t = util::Bus<cmdBusWidth>;
    using databus_t = util::databus_presets::databus_preset_t;
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using patternHandler_t = PatternHandler<CmdType>;
    using interfaceRegisterHelper_t = util::InterfaceRegisterHelper<LPDDR4Interface>;

// Public constructors and assignment operators
public:
    LPDDR4Interface() = delete; // no default constructor
    LPDDR4Interface(const LPDDR4Interface&) = default; // copy constructor
    LPDDR4Interface& operator=(const LPDDR4Interface&) = default; // copy assignment operator
    LPDDR4Interface(LPDDR4Interface&&) = default; // move constructor
    LPDDR4Interface& operator=(LPDDR4Interface&&) = default; // move assignment operator

    LPDDR4Interface(const std::shared_ptr<const MemSpecLPDDR4>& memSpec, implicitCommandInserter_t&& implicitCommandInserter);

// Private Member functions
private:
    void registerPatterns();

// Public member functions
public:
    interfaceRegisterHelper_t getRegisterHelper() {
        return interfaceRegisterHelper_t{this};
    }
    void handleOverrides(size_t length, bool read);
    void handleDQs(const Command& cmd, util::Clock &dqs, size_t length);
    void handleCommandBus(const Command& cmd);
    void handleData(const Command &cmd, bool read);

    void enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp);
    void enableBus(timestamp_t timestamp, timestamp_t enable_timestamp);
    timestamp_t updateTogglingRate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition);
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;

// Public member variables
public:
    commandbus_t m_commandBus;
    databus_t m_dataBus;
    util::Clock m_readDQS;
    util::Clock m_writeDQS;
    util::Clock m_clock;

// Private member variables
private:
    std::shared_ptr<const MemSpecLPDDR4> m_memSpec;
    patternHandler_t m_patternHandler;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR4_LPDDR4INTERFACE_H */
