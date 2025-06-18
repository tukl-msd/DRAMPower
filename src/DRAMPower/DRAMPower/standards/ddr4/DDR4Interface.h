#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4INTERFACE_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4INTERFACE_H

#include <DRAMPower/util/pin.h>
#include <DRAMPower/util/bus.h>
#include <DRAMPower/util/databus.h>
#include <DRAMPower/util/databus_presets.h>
#include <DRAMPower/util/clock.h>

#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>

#include <DRAMPower/util/PatternHandler.h>
#include <DRAMPower/util/ImplicitCommandHandler.h>
#include <DRAMPower/util/dbi.h>

#include <DRAMPower/memspec/MemSpecDDR4.h>

#include <DRAMUtils/config/toggling_rate.h>

#include <functional>
#include <stdint.h>
#include <cstddef>
#include <vector>

namespace DRAMPower {

template<typename Interface>
struct InterfaceRegisterHelper {
// Public constructors and assignment operators
public:
    InterfaceRegisterHelper(Interface *interface)
        : m_interface(interface)
    {}
    InterfaceRegisterHelper(const InterfaceRegisterHelper&) = default; // copy constructor
    InterfaceRegisterHelper& operator=(const InterfaceRegisterHelper&) = default; // copy assignment operator
    InterfaceRegisterHelper(InterfaceRegisterHelper&&) = default; // Move constructor
    InterfaceRegisterHelper& operator=(InterfaceRegisterHelper&&) = default; // Move assignment operator
    ~InterfaceRegisterHelper() = default; // Destructor

// Public member functions
    template<typename Func>
    decltype(auto) registerHandler(Func &&member_func) {
        Interface *this_ptr = m_interface;
        return [this_ptr, member_func](const Command & command) {
            (this_ptr->*member_func)(command);
        };
    }

// Private member variables
private:
    Interface *m_interface;
};

class DDR4Interface {
public:
    const static std::size_t cmdBusWidth = 27;
    const static uint64_t cmdBusInitPattern = (1<<cmdBusWidth)-1;
// Public type definitions
public:
    using commandbus_t = util::Bus<cmdBusWidth>;
    using databus_t = util::databus_presets::databus_preset_t;
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using patternHandler_t = PatternHandler<CmdType>;
    using interfaceRegisterHelper_t = InterfaceRegisterHelper<DDR4Interface>;

// Public constructor
public:
    DDR4Interface() = delete; // no default constructor
    DDR4Interface(const DDR4Interface&) = default; // copy constructor
    DDR4Interface& operator=(const DDR4Interface&) = default; // copy assignment operator
    DDR4Interface(DDR4Interface&&) = default; // move constructor
    DDR4Interface& operator=(DDR4Interface&&) = default; // move assignment operator

    DDR4Interface(const MemSpecDDR4 &memSpec, implicitCommandInserter_t&& implicitCommandInserter, 
                  patternHandler_t &patternHandler)
    : m_commandBus{cmdBusWidth, 1,
        util::BusIdlePatternSpec::H, util::BusInitPatternSpec::H}
    , m_dataBus{
        util::databus_presets::getDataBusPreset(
            memSpec.bitWidth * memSpec.numberOfDevices,
            util::DataBusConfig {
                memSpec.bitWidth * memSpec.numberOfDevices,
                memSpec.dataRate,
                util::BusIdlePatternSpec::H,
                util::BusInitPatternSpec::H,
                DRAMUtils::Config::TogglingRateIdlePattern::H,
                0.0,
                0.0,
                util::DataBusMode::Bus
            }
        )
    }
    , m_readDQS(memSpec.dataRate, true)
    , m_writeDQS(memSpec.dataRate, true)
    , m_clock(2, false)
    , m_dbi(memSpec.numberOfDevices * memSpec.bitWidth, util::DBI::IdlePattern_t::H, 8,
        [this](timestamp_t load_timestamp, timestamp_t chunk_timestamp, std::size_t pin, bool inversion_state, bool read) {
        this->handleDBIPinChange(load_timestamp, chunk_timestamp, pin, inversion_state, read);
    }, false)
    , m_dbiread(m_dbi.getChunksPerWidth(), util::Pin{m_dbi.getIdlePattern()})
    , m_dbiwrite(m_dbi.getChunksPerWidth(), util::Pin{m_dbi.getIdlePattern()})
    , prepostambleReadMinTccd(memSpec.prePostamble.readMinTccd)
    , prepostambleWriteMinTccd(memSpec.prePostamble.writeMinTccd)
    , m_ranks(memSpec.numberOfRanks)
    , m_memSpec(memSpec)
    , m_patternHandler(patternHandler)
    , m_implicitCommandInserter(std::move(implicitCommandInserter))
    {
        registerPatterns();
    }

// Private member functions
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
    void handlePrePostamble(
        const timestamp_t   timestamp,
        const uint64_t      length,
        RankInterface &     rank,
        bool                read
    );
    
    void enableTogglingHandle(timestamp_t timestamp, timestamp_t enable_timestamp);
    void enableBus(timestamp_t timestamp, timestamp_t enable_timestamp);
    timestamp_t updateTogglingRate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition);
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp);
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;

private:
    template<typename Func>
    void addImplicitCommand(timestamp_t timestamp, Func && func) {
        m_implicitCommandInserter.addImplicitCommand(timestamp, std::forward<Func>(func));
    }

// Public members
public:
    commandbus_t m_commandBus;
    databus_t m_dataBus;
    util::Clock m_readDQS;
    util::Clock m_writeDQS;
    util::Clock m_clock;
    util::DBI m_dbi;
    std::vector<util::Pin> m_dbiread;
    std::vector<util::Pin> m_dbiwrite;
    uint64_t prepostambleReadMinTccd;
    uint64_t prepostambleWriteMinTccd;
    std::vector<RankInterface> m_ranks;

// Private members
private:
    std::reference_wrapper<const MemSpecDDR4> m_memSpec;
    std::reference_wrapper<patternHandler_t> m_patternHandler;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4INTERFACE_H */
