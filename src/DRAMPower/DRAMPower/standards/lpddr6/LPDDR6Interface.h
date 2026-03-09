#ifndef DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H
#define DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H

#include "DRAMPower/util/pin.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/databus_presets.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/RegisterHelper.h"
#include "DRAMPower/util/Serialize.h"
#include "DRAMPower/util/Deserialize.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/data/stats.h"

#include "DRAMPower/util/PatternHandler.h"
#include "DRAMPower/util/ImplicitCommandHandler.h"
#include "DRAMPower/util/dbi.h"

#include "DRAMPower/memspec/MemSpecLPDDR6.h"

#include "DRAMUtils/config/toggling_rate.h"

#include <stdint.h>
#include <cstddef>

namespace DRAMPower {

class LPDDR6Interface : public util::Serialize, public util::Deserialize {
// Public constants
public:
    const static std::size_t cmdBusWidth = 4;
    const static uint64_t cmdBusInitPattern = 0;

// Public type definitions
public:
    using commandbus_t = util::Bus<cmdBusWidth>;
    using pin_dbi_t = util::Pin<8>; // max_burst_length = 8
    using databus_t = util::databus_presets::databus_preset_t;
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using patternHandler_t = PatternHandler<CmdType>;
    using interfaceRegisterHelper_t = util::InterfaceRegisterHelper<LPDDR6Interface>;

// Public constructor
public:
    LPDDR6Interface() = delete; // no default constructor
    LPDDR6Interface(const LPDDR6Interface&) = default; // copy constructor
    LPDDR6Interface& operator=(const LPDDR6Interface&) = delete; // copy assignment operator
    LPDDR6Interface(LPDDR6Interface&&) = default; // move constructor
    LPDDR6Interface& operator=(LPDDR6Interface&&) = delete; // move assignment operator

    LPDDR6Interface(const MemSpecLPDDR6 &memSpec, implicitCommandInserter_t&& implicitCommandInserter);

// Private member functions
private:
    void registerPatterns();
    std::optional<const uint8_t *> handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read);
    const uint8_t* formatData(const uint8_t* data, std::size_t n_bits, bool read);

// Public member functions
public:
    interfaceRegisterHelper_t getRegisterHelper() {
        return interfaceRegisterHelper_t{this};
    }
    void handleOverrides(size_t length, bool read);
    void handleDQs(const Command& cmd, util::Clock &dqs, size_t length);
    void handleCommandBus(const Command& cmd);
    void handleData(const Command &cmd, bool read);
    void endOfSimulation(timestamp_t timestamp);
    
    void enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp);
    void enableBus(timestamp_t timestamp, timestamp_t enable_timestamp);
    timestamp_t updateTogglingRate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition);
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;

    void setMetaData(uint16_t metaData);
    uint16_t getMetaData();

// Overrides
public:
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;

// Public members
public:
    commandbus_t m_commandBus;
    databus_t m_dataBus;
    util::Clock m_readDQS;
    util::Clock m_wck;
    util::Clock m_clock;
private:
    const MemSpecLPDDR6& m_memSpec;
public:
    util::DBI<uint8_t, 1, util::PinState::L, util::StaticDBI> m_dbi;

// Private member variables
private:
    std::vector<uint8_t> m_formatResult;
    uint16_t m_metaData = 0;
    patternHandler_t m_patternHandler;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H */
