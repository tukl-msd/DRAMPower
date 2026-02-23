#ifndef DRAMPOWER_STANDARDS_HBM2_HBM2INTERFACE_H
#define DRAMPOWER_STANDARDS_HBM2_HBM2INTERFACE_H

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
#include "DRAMPower/util/dbialgos.h"
#include "DRAMPower/util/dbi.h"
#include "DRAMPower/util/pin_types.h"

#include "DRAMPower/memspec/MemSpecHBM2.h"

#include "DRAMUtils/config/toggling_rate.h"

#include <cstdint>
#include <stdint.h>
#include <cstddef>
#include <vector>

namespace DRAMPower {

class HBM2Interface : public util::Serialize, public util::Deserialize {
// Public constants
public:
    const static std::size_t maxColumnCmdBusWidth = 9;
    const static std::size_t minColumnCmdBusWidth = 8;
    const static std::size_t maxRowCmdBusWidth = 7;
    const static std::size_t minRowCmdBusWidth = 6;
    const static std::size_t DWORD = 32;

// Public type definitions
public:
    using columnCommandbus_t = util::Bus<maxColumnCmdBusWidth>;
    using rowCommandbus_t = util::Bus<maxRowCmdBusWidth>;

    using pin_dbi_t = util::Pin<64>;

    using pin_cke_t = util::Pin<1>;

    using databus_t = util::databus_presets::databus_preset_t;
    struct databusContainer_t {
        databus_t m_dataBus;
        util::Clock m_readDQS; // one per DWORD -> scaled in get_stats
        util::Clock m_writeDQS; // one per DWORD -> scaled in get_stats
    };

    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using patternHandler_t = PatternHandler<CmdType>;
    using interfaceRegisterHelper_t = util::InterfaceRegisterHelper<HBM2Interface>;

// Public constructors and assignment operators
public:
    HBM2Interface() = delete; // no default constructor
    HBM2Interface(const HBM2Interface&) = default; // copy constructor
    HBM2Interface& operator=(const HBM2Interface&) = delete; // copy assignment operator
    HBM2Interface(HBM2Interface&&) = default; // move constructor
    HBM2Interface& operator=(HBM2Interface&&) = delete; // move assignment operator

    HBM2Interface(const MemSpecHBM2& memSpec, implicitCommandInserter_t&& implicitCommandInserter);

// Private member functions
private:
    void registerPatterns();
    std::optional<const uint8_t *> handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read);
    void handleDBIPinChange(const timestamp_t load_timestamp, std::size_t pin, bool state, bool read);
    static std::size_t getRowWidth(const MemSpecHBM2& memSpec);
    static std::size_t getColumnWidth(const MemSpecHBM2& memSpec);

// Public member functions
public:
    interfaceRegisterHelper_t getRegisterHelper() {
        return interfaceRegisterHelper_t{this};
    }
    void handleDQs(const Command& cmd, util::Clock &dqs, size_t length);
    void handleColumnCommandBus(const Command& cmd);
    void handleRowCommandBus(const Command& cmd);
    void handleData(const Command &cmd, bool read);

    void endOfSimulation(timestamp_t timestamp);

    void enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp);
    void enableBus(timestamp_t timestamp, timestamp_t enable_timestamp);
    timestamp_t updateTogglingRate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition);
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;

// Overrides
public:
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;

// Public member variables
public:
    columnCommandbus_t m_columnCommandBus;
    rowCommandbus_t m_rowCommandBus;
    pin_cke_t m_cke;
    std::vector<databusContainer_t> m_dataBus;
    util::Clock m_clock;
private:
    const MemSpecHBM2& m_memSpec;
public:
    util::DBI<uint8_t, 1, util::PinState::H, util::DynamicDBI<4>> m_dbi;
    std::vector<pin_dbi_t> m_dbiread;
    std::vector<pin_dbi_t> m_dbiwrite;

// Private member variables
private:
    patternHandler_t m_patternHandler;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_HBM2_HBM2INTERFACE_H */
