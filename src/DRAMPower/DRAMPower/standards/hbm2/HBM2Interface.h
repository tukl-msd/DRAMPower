#ifndef DRAMPOWER_STANDARDS_HBM2_HBM2INTERFACE_H
#define DRAMPOWER_STANDARDS_HBM2_HBM2INTERFACE_H

#include "DRAMPower/standards/hbm2/HBM2Command.h"
#include "DRAMPower/standards/hbm2/HBM2Pattern.h"
#include "DRAMPower/util/pin.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/databus_presets.h"
#include "DRAMPower/util/clock.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/data/stats.h"

#include "DRAMPower/util/PatternHandler.h"
#include "DRAMPower/util/dbialgos.h"
#include "DRAMPower/util/dbi.h"
#include "DRAMPower/util/pin_types.h"

#include "DRAMPower/memspec/MemSpecHBM2.h"

#include "DRAMPower/simconfig/simconfig.h"

#include <cstdint>
#include <cstddef>
#include <vector>

namespace DRAMPower {

struct HBM2InterfaceMemSpec {
    HBM2InterfaceMemSpec(const MemSpecHBM2& memSpec)
        : dataRate(memSpec.dataRate)
        , burstLength(memSpec.burstLength)
        , bitWidth(memSpec.bitWidth)
        , numberOfStacks(memSpec.numberOfStacks)
        , numberOfPseudoChannels(memSpec.numberOfPseudoChannels)
        , numberOfRows(memSpec.numberOfRows)
        , numberOfDevices(memSpec.numberOfDevices)
    {}

    uint64_t dataRate;
    uint64_t burstLength;
    uint64_t bitWidth;
    uint64_t numberOfStacks;
    uint64_t numberOfPseudoChannels;
    uint64_t numberOfRows;
    uint64_t numberOfDevices;
};

class HBM2Interface {
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
    using databus_t = util::databus_presets::databus_preset_t;
    using pin_dbi_t = util::Pin<64>;
    using pin_cke_t = util::Pin<1>;
    using patternHandler_t = PatternHandler<CmdType, pattern_descriptor_HBM2::t, HBM2TargetCoordinate, HBM2Encoder, HBM2PatternExtraData>;

    struct databusContainer_t {
        databus_t m_dataBus;
        util::Clock m_readDQS; // one per DWORD -> scaled in get_stats
        util::Clock m_writeDQS; // one per DWORD -> scaled in get_stats
    };

// Public constructors and assignment operators
public:
    HBM2Interface(const MemSpecHBM2& memSpec, const config::SimConfig &simConfig = {});

// Public member functions
public:
// Member functions
    timestamp_t getLastCommandTime() const;
    void doCommand(const HBM2Command& cmd);
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;
// Extensions
    void enableDBI(bool enable) {
        m_dbi.enable(enable);
    }

// Private member functions
private:
    void registerPatterns();
    std::optional<const uint8_t *> handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read);
    void handleDBIPinChange(const timestamp_t load_timestamp, std::size_t pin, bool state, bool read);
    void handleDQs(const HBM2Command& cmd, util::Clock &dqs, size_t length);
    void handleColumnCommandBus(const HBM2Command& cmd);
    void handleRowCommandBus(const HBM2Command& cmd);
    void handleData(const HBM2Command &cmd, bool read);
    void endOfSimulation(timestamp_t timestamp);
    
    static std::size_t getRowWidth(const HBM2InterfaceMemSpec& memSpec);
    static std::size_t getColumnWidth(const HBM2InterfaceMemSpec& memSpec);

// Private member variables
private:
    HBM2InterfaceMemSpec m_memSpec;
    columnCommandbus_t m_columnCommandBus;
    rowCommandbus_t m_rowCommandBus;
    std::vector<databusContainer_t> m_dataBus;
    util::Clock m_clock;
    pin_cke_t m_cke;
    util::DBI<uint8_t, 1, util::PinState::H, util::DynamicDBI<4>> m_dbi;
    std::vector<pin_dbi_t> m_dbiread;
    std::vector<pin_dbi_t> m_dbiwrite;
    patternHandler_t m_patternHandler;
    timestamp_t m_last_command_time = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_HBM2_HBM2INTERFACE_H */
