#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4INTERFACE_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4INTERFACE_H

#include "DRAMPower/util/pin.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/databus_presets.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/Serialize.h"
#include "DRAMPower/util/Deserialize.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/data/stats.h"

#include "DRAMPower/util/PatternHandler.h"
#include "DRAMPower/util/dbi.h"

#include "DRAMPower/memspec/MemSpecDDR4.h"

#include "DRAMPower/simconfig/simconfig.h"

#include <stdint.h>
#include <cstddef>
#include <vector>
#include <optional>

namespace DRAMPower {

struct DDR4InterfaceMemSpec {
    DDR4InterfaceMemSpec(const MemSpecDDR4& memSpec)
        : dataRate(memSpec.dataRate)
        , burstLength(memSpec.burstLength)
        , bitWidth(memSpec.bitWidth)
        , numberOfRanks(memSpec.numberOfRanks)
    {}

    uint64_t dataRate;
    uint64_t burstLength;
    uint64_t bitWidth;
    uint64_t numberOfRanks;
};

class DDR4Interface : public util::Serialize, public util::Deserialize {
// Public constants
public:
    const static std::size_t cmdBusWidth = 27;
    const static uint64_t cmdBusInitPattern = (1<<cmdBusWidth)-1;

// Public type definitions
public:
    using commandbus_t = util::Bus<cmdBusWidth>;
    using pin_dbi_t = util::Pin<8>; // max_burst_length = 8
    using databus_t = util::databus_presets::databus_preset_t;
    using patternHandler_t = PatternHandler<CmdType>;

// Public constructors and assignment operators
public:
    DDR4Interface(const MemSpecDDR4& memSpec, const config::SimConfig &simConfig = {});

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

// Private member functions
private:
    void registerPatterns();
    std::optional<const uint8_t *> handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read);
    void handleDBIPinChange(const timestamp_t load_timestamp, std::size_t pin, bool state, bool read);
    void handleOverrides(size_t length, bool read);
    void handleDQs(const Command& cmd, util::Clock &dqs, size_t length);
    void handleCommandBus(const Command& cmd);
    void handleData(const Command &cmd, bool read);
    void handlePrePostamble(
        const timestamp_t   timestamp,
        const uint64_t      length,
        RankInterface &     rank,
        bool                read
    );
    void endOfSimulation(timestamp_t timestamp);

// Private member variables
private:
    DDR4InterfaceMemSpec m_memSpec;
    commandbus_t m_commandBus;
    databus_t m_dataBus;
    util::Clock m_readDQS;
    util::Clock m_writeDQS;
    util::Clock m_clock;
    util::DBI<uint8_t, 1, util::PinState::H, util::StaticDBI> m_dbi;
    std::vector<pin_dbi_t> m_dbiread;
    std::vector<pin_dbi_t> m_dbiwrite;
    uint64_t prepostambleReadMinTccd;
    uint64_t prepostambleWriteMinTccd;
    std::vector<RankInterface> m_ranks;
    patternHandler_t m_patternHandler;
    timestamp_t m_last_command_time = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4INTERFACE_H */
