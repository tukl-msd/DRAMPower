#ifndef DRAMPOWER_STANDARDS_LPDDR4_LPDDR4INTERFACE_H
#define DRAMPOWER_STANDARDS_LPDDR4_LPDDR4INTERFACE_H

#include "DRAMPower/util/pin.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/databus_presets.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/RegisterHelper.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/data/stats.h"

#include "DRAMPower/util/PatternHandler.h"
#include "DRAMPower/util/ImplicitCommandHandler.h"
#include "DRAMPower/util/dbi.h"

#include "DRAMPower/memspec/MemSpecLPDDR4.h"

#include "DRAMUtils/config/toggling_rate.h"

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
    LPDDR4Interface& operator=(const LPDDR4Interface&) = delete; // copy assignment operator
    LPDDR4Interface(LPDDR4Interface&&) = default; // move constructor
    LPDDR4Interface& operator=(LPDDR4Interface&&) = delete; // move assignment operator

    LPDDR4Interface(const MemSpecLPDDR4& memSpec, implicitCommandInserter_t&& implicitCommandInserter);

// Private Member functions
private:
    void registerPatterns();
    std::optional<const uint8_t *> handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read);
    void handleDBIPinChange(const timestamp_t load_timestamp, timestamp_t chunk_timestamp, std::size_t pin, bool state, bool read);

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
    util::DBI m_dbi;
    std::vector<util::Pin> m_dbiread;
    std::vector<util::Pin> m_dbiwrite;

// Private member variables
private:
    const MemSpecLPDDR4& m_memSpec;
    patternHandler_t m_patternHandler;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR4_LPDDR4INTERFACE_H */
