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
    DDR4Interface() = delete; // no default constructor
    DDR4Interface(const MemSpecDDR4& memSpec, const config::SimConfig &simConfig = {});
    DDR4Interface(const DDR4Interface& other, MemSpecDDR4& memSpec)
        : m_memSpec(memSpec)
        , m_commandBus(other.m_commandBus)
        , m_dataBus(other.m_dataBus)
        , m_readDQS(other.m_readDQS)
        , m_writeDQS(other.m_writeDQS)
        , m_clock(other.m_clock)
        , m_dbi(other.m_dbi)
        , m_dbiread(other.m_dbiread)
        , m_dbiwrite(other.m_dbiwrite)
        , prepostambleReadMinTccd(other.prepostambleReadMinTccd)
        , prepostambleWriteMinTccd(other.prepostambleWriteMinTccd)
        , m_ranks(other.m_ranks)
        , m_patternHandler(other.m_patternHandler)
        , m_last_command_time(other.m_last_command_time)
    {}
    DDR4Interface(DDR4Interface&& other, MemSpecDDR4& memSpec) noexcept // TODO
        : m_memSpec(memSpec)
        , m_commandBus(std::move(other.m_commandBus))
        , m_dataBus(std::move(other.m_dataBus))
        , m_readDQS(std::move(other.m_readDQS))
        , m_writeDQS(std::move(other.m_writeDQS))
        , m_clock(std::move(other.m_clock))
        , m_dbi(std::move(other.m_dbi))
        , m_dbiread(std::move(other.m_dbiread))
        , m_dbiwrite(std::move(other.m_dbiwrite))
        , prepostambleReadMinTccd(std::move(other.prepostambleReadMinTccd))
        , prepostambleWriteMinTccd(std::move(other.prepostambleWriteMinTccd))
        , m_ranks(std::move(other.m_ranks))
        , m_patternHandler(std::move(other.m_patternHandler))
        , m_last_command_time(std::move(other.m_last_command_time))
    {}

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
    const MemSpecDDR4& m_memSpec;
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
    timestamp_t m_last_command_time;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4INTERFACE_H */
