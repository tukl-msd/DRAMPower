#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4_H

#include <DRAMPower/util/pin.h>
#include <DRAMPower/util/bus.h>
#include <DRAMPower/util/databus.h>
#include <DRAMPower/util/databus_presets.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/dram/Interface.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/memspec/MemSpecDDR4.h>
#include <DRAMPower/util/dbi.h>

#include <DRAMUtils/config/toggling_rate.h>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/util/cycle_stats.h>
#include <DRAMPower/util/clock.h>

#include <stdint.h>
#include <vector>
#include <optional>

namespace DRAMPower {

template<typename Interface>
struct InterfaceRegisterHelper {
// Public constructors and assignment operators
public:
    InterfaceRegisterHelper(Interface *interface)
        : m_interface(interface)
    {}
    InterfaceRegisterHelper(const InterfaceRegisterHelper&) = delete; // No copy constructor
    InterfaceRegisterHelper& operator=(const InterfaceRegisterHelper&) = delete; // No copy assignment operator
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

template<typename Core>
struct CoreRegisterHelper {
// Public constructors and assignment operators
public:
    CoreRegisterHelper(Core *core, std::vector<Rank> &ranks)
        : m_core(core)
        , m_ranks(ranks)
    {}
    CoreRegisterHelper(const CoreRegisterHelper&) = delete; // No copy constructor
    CoreRegisterHelper& operator=(const CoreRegisterHelper&) = delete; // No copy assignment operator
    CoreRegisterHelper(CoreRegisterHelper&&) = default; // Move constructor
    CoreRegisterHelper& operator=(CoreRegisterHelper&&) = default; // Move assignment operator
    ~CoreRegisterHelper() = default; // Destructor

// Public member functions
public:
    template<typename Func>
    decltype(auto) registerBankHandler(Func &&member_func) {
        Core* this_ptr = m_core;
        return [this_ptr, ranks_ref = std::ref(m_ranks), member_func](const Command & command) {
            auto &this_ranks = ranks_ref.get();
            assert(this_ranks.size()>command.targetCoordinate.rank);
            auto & rank = this_ranks.at(command.targetCoordinate.rank);

            assert(rank.banks.size()>command.targetCoordinate.bank);
            auto & bank = rank.banks.at(command.targetCoordinate.bank);

            rank.commandCounter.inc(command.type);
            (this_ptr->*member_func)(rank, bank, command.timestamp);
        };
    }

    template<typename Func>
    decltype(auto) registerRankHandler(Func &&member_func) {
        Core* this_ptr = m_core;
        return [this_ptr, ranks_ref = std::ref(m_ranks), member_func](const Command & command) {
            auto &this_ranks = ranks_ref.get();
            assert(this_ranks.size()>command.targetCoordinate.rank);
            auto & rank = this_ranks.at(command.targetCoordinate.rank);

            rank.commandCounter.inc(command.type);
            (this_ptr->*member_func)(rank, command.timestamp);
        };
    }

    template<typename Func>
    decltype(auto) registerHandler(Func && member_func) {
        Core* this_ptr = m_core;
        return [this_ptr, member_func](const Command & command) {
            (this_ptr->*member_func)(command.timestamp);
        };
    }

// Private member variables
private:
    Core *m_core;
    std::vector<Rank> &m_ranks;
};

class DDR4Interface {
// Public type definitions
public:
    using commandbus_t = util::Bus<14>;
    using databus_t = util::databus_presets::databus_preset_t;
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using patternHandler_t = PatternHandler<CmdType>;
    using interfaceRegisterHelper_t = InterfaceRegisterHelper<DDR4Interface>;

// Public constructor
public:
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
    std::optional<const uint8_t *> handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read);
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
    const static std::size_t cmdBusWidth = 27;
    const static uint64_t cmdBusInitPattern = (1<<cmdBusWidth)-1;
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
    const MemSpecDDR4& m_memSpec;
    patternHandler_t &m_patternHandler;
    implicitCommandInserter_t m_implicitCommandInserter;
};



class DDR4Core {
// Public type definitions
public:
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using coreRegisterHelper_t = CoreRegisterHelper<DDR4Core>;

// Public constructors
public:
    DDR4Core(const MemSpecDDR4 &memSpec, implicitCommandInserter_t&& implicitCommandInserter)
        : m_ranks(memSpec.numberOfRanks, {static_cast<std::size_t>(memSpec.numberOfBanks)}) 
        , m_memSpec(memSpec)
        , m_implicitCommandInserter(std::move(implicitCommandInserter))
    {}

// Public member functions
public:
    coreRegisterHelper_t getRegisterHelper() {
        return coreRegisterHelper_t{this, m_ranks};
    }

// Private member functions
private:
    template<typename Func>
    void addImplicitCommand(timestamp_t timestamp, Func && func) {
        m_implicitCommandInserter.addImplicitCommand(timestamp, std::forward<Func>(func));
    }

// Public member functions
public:
    void handleAct(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handlePre(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handlePreAll(Rank & rank, timestamp_t timestamp); 
    void handleRefAll(Rank & rank, timestamp_t timestamp);
    void handleSelfRefreshEntry(Rank & rank, timestamp_t timestamp);
    void handleSelfRefreshExit(Rank & rank, timestamp_t timestamp);
    void handleRead(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleWrite(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleReadAuto(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleWriteAuto(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handlePowerDownActEntry(Rank & rank, timestamp_t timestamp);
    void handlePowerDownActExit(Rank & rank, timestamp_t timestamp);
    void handlePowerDownPreEntry(Rank & rank, timestamp_t timestamp);
    void handlePowerDownPreExit(Rank & rank, timestamp_t timestamp);
    timestamp_t earliestPossiblePowerDownEntryTime(Rank & rank) const;
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;

// Public members
public:
    std::vector<Rank> m_ranks;

// Private members
private:
    const MemSpecDDR4& m_memSpec;
    implicitCommandInserter_t m_implicitCommandInserter;
};

class DDR4 : public dram_base<CmdType> {
// Public constructors and assignment operators
public:
    DDR4() = delete; // No default constructor
    DDR4(const DDR4&) = delete; // No copy constructor
    DDR4& operator=(const DDR4&) = delete; // No copy assignment operator
    DDR4(DDR4&&) = default; // Move constructor
    DDR4& operator=(DDR4&&) = delete; // Move assignment operator
    DDR4(const MemSpecDDR4 &memSpec);
    ~DDR4() override = default;

// Overrides
private:
    timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) override;
public:
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;
    SimulationStats getWindowStats(timestamp_t timestamp) override;
    uint64_t getBankCount() override;
    uint64_t getRankCount() override;
    uint64_t getDeviceCount() override;

// Private member functions
private:
    void registerCommands();
    void registerExtensions();
    void endOfSimulation(timestamp_t timestamp);

// Private member variables
private:
    MemSpecDDR4 memSpec;
    DDR4Interface interface;
    DDR4Core core;
};

};

#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4_H */
