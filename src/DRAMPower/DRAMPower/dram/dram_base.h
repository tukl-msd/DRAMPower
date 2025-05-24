#ifndef DRAMPOWER_DRAM_DRAM_BASE_H
#define DRAMPOWER_DRAM_DRAM_BASE_H

#include <DRAMPower/command/Command.h>
#include <DRAMPower/command/Pattern.h>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>
#include <DRAMPower/util/extension_manager.h>
#include <DRAMPower/util/extensions.h>

#include <DRAMUtils/config/toggling_rate.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <functional>
#include <vector>
#include <limits>
#include <optional>

namespace DRAMPower {

using namespace DRAMUtils::Config;

class ImplicitCommandHandler {
// Public type definitions
public:
    using implicitCommand_t = std::function<void(void)>;
    using implicitCommandListEntry_t = std::pair<timestamp_t, implicitCommand_t>;
    using implicitCommandList_t = std::deque<implicitCommandListEntry_t>;

// Helper classes
public:
class Inserter {
// Friend classes
    friend class ImplicitCommandHandler;

// Private constructors and assignment operators
private:
    Inserter() = delete; // No default constructor
    Inserter(const Inserter&) = delete; // No copy constructor
    Inserter& operator=(const Inserter&) = delete; // No copy assignment operator

    // Constructor that takes a reference to the implicit command list
    explicit Inserter(implicitCommandList_t& implicitCommandList)
        : m_implicitCommandList(implicitCommandList)
    {}

// Public constructors and assignment operators
public:
    Inserter(Inserter&&) = default;
    Inserter& operator=(Inserter&&) = default;
    ~Inserter() = default;

// Public member functions
public:
    // Add an implicit command to the list
    template <typename Func>
    void addImplicitCommand(timestamp_t timestamp, Func&& func)
    {
        auto entry = std::make_pair(timestamp, std::forward<Func>(func));

        auto upper = std::upper_bound(m_implicitCommandList.begin(), m_implicitCommandList.end(),
            entry,
            [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

        m_implicitCommandList.emplace(upper, entry);
    }

    std::size_t implicitCommandCount() const { return this->m_implicitCommandList.size(); };

// Private member variables
private:
    implicitCommandList_t& m_implicitCommandList;
};

// Helper type definitions
public:
    using Inserter_t = Inserter;

// Constructors and assignment operators
public:
    ImplicitCommandHandler() = default;
    ImplicitCommandHandler(const ImplicitCommandHandler&) = delete; // No copy constructor
    ImplicitCommandHandler& operator=(const ImplicitCommandHandler&) = delete; // No copy assignment operator
    ImplicitCommandHandler(ImplicitCommandHandler&&) = default;
    ImplicitCommandHandler& operator=(ImplicitCommandHandler&&) = default;
    ~ImplicitCommandHandler() = default;

// Inserter forwards
public:
    void addImplicitCommand(timestamp_t timestamp, const implicitCommand_t& func)
    {
        m_inserter.addImplicitCommand(timestamp, func);
    }

// Public member functions
public:
    // Get inserter reference
    Inserter_t& getInserter() {
        return m_inserter;
    }
    // Get const inserter reference
    const Inserter_t& getInserter() const {
        return m_inserter;
    }
    // Create a inserter for adding implicit commands which holds a reference to the implicit command list
    Inserter_t createInserter() {
        return Inserter_t(m_implicitCommandList);
    }
    // Process implicit command queue
    void processImplicitCommandQueue(timestamp_t timestamp, timestamp_t &last_command_time)
    {
        // Process implicit command list
        while (!m_implicitCommandList.empty() && m_implicitCommandList.front().first <= timestamp) {
            // Execute implicit command functor
            auto& [i_timestamp, i_implicitCommand] = m_implicitCommandList.front();
            i_implicitCommand();
            last_command_time = i_timestamp;
            m_implicitCommandList.pop_front();
        };
    }

// Private member variables
private:
    implicitCommandList_t m_implicitCommandList;
    Inserter_t m_inserter{m_implicitCommandList};
};

template <typename CommandEnum>
class PatternHandler {
// Public type definitions+
public:
    using commandEnum_t = CommandEnum;
    using commandPattern_t = std::vector<pattern_descriptor::t>;
    using commandPatternMap_t = std::vector<commandPattern_t>;

// Constructors and assignment operators
public:
    PatternHandler() = delete; // No default constructor
    PatternHandler(const PatternHandler&) = default;
    PatternHandler& operator=(const PatternHandler&) = default;
    PatternHandler(PatternHandler&&) = default;
    PatternHandler& operator=(PatternHandler&&) = default;
    ~PatternHandler() = default;

    // Constructor with encoder overrides and initial patterns
    explicit PatternHandler(PatternEncoderOverrides encoderoverrides, uint64_t initPattern = 0)
        : m_encoder(encoderoverrides)
        , m_commandPatternMap(static_cast<std::size_t>(commandEnum_t::COUNT), commandPattern_t {})
        , m_lastPattern(initPattern)
    {}
    // Constructor with no encoder overrides and initial patterns
    explicit PatternHandler(uint64_t initPattern = 0)
        : m_lastPattern(initPattern)
        , m_commandPatternMap(static_cast<std::size_t>(commandEnum_t::COUNT), commandPattern_t {})
    {}

// Public member functions
public:
    PatternEncoder& getEncoder() { return m_encoder; }
    const PatternEncoder& getEncoder() const { return m_encoder; }

    template <commandEnum_t cmd_type>
    void registerPattern(const commandPattern_t &pattern)
    {
        assert(m_commandPatternMap.size() > static_cast<std::size_t>(cmd_type));
        m_commandPatternMap[static_cast<std::size_t>(cmd_type)] = pattern;
    }
    template <commandEnum_t cmd_type>
    void registerPattern(std::initializer_list<pattern_descriptor::t> pattern)
    {
        assert(m_commandPatternMap.size() > static_cast<std::size_t>(cmd_type));
        m_commandPatternMap[static_cast<std::size_t>(cmd_type)] = commandPattern_t(pattern);
    }

    const commandPattern_t& getPattern(CmdType cmd_type) const
    {
        assert(m_commandPatternMap.size() > static_cast<std::size_t>(cmd_type));
        return m_commandPatternMap[static_cast<std::size_t>(cmd_type)];
    }
    uint64_t getCommandPattern(const Command& cmd)
    {
        if (m_commandPatternMap[static_cast<std::size_t>(cmd.type)].empty()) {
            // No pattern registered for this command
            throw std::runtime_error("No pattern registered for this command");
        }
        const auto& pattern = m_commandPatternMap[static_cast<std::size_t>(cmd.type)];
        m_lastPattern = m_encoder.encode(cmd, pattern, m_lastPattern);
        return m_lastPattern;
    }

    uint64_t getCoordinatePattern(const TargetCoordinate& coordinate, const commandPattern_t& pattern)
    {
        if (pattern.empty()) {
            // No pattern provided
            throw std::runtime_error("No pattern provided for this coordinate");
        }
        m_lastPattern = m_encoder.encode(coordinate, pattern, m_lastPattern);
        return m_lastPattern;
    }

// Private member variables
private:
    PatternEncoder m_encoder;
    commandPatternMap_t m_commandPatternMap;
    uint64_t m_lastPattern;
};

template <typename CommandEnum>
class Router {
// Public type definitions
public:
    using commandEnum_t = CommandEnum;
    using commandHandler_t = std::function<void(const Command&)>;
    using commandRouter_t = std::vector<commandHandler_t>;

// Constructors and assignment operators
public:
    Router() = delete; // No default constructor
    Router(const Router&) = default;
    Router& operator=(const Router&) = default;
    Router(Router&&) = default;
    Router& operator=(Router&&) = default;
    ~Router() = default;

    // constructor with default handler
    Router(std::optional<commandHandler_t> defaultHandler = std::nullopt)
        : m_router(static_cast<std::size_t>(commandEnum_t::COUNT), defaultHandler.value_or([](const Command&) {}))
    {}

// Public member functions
public:
    // Add a command handler to the router for the given command
    template <commandEnum_t cmd, typename Func>
    void routeCommand(Func&& func)
    {
        assert(m_router.size() > static_cast<std::size_t>(cmd));
        m_router[static_cast<std::size_t>(cmd)] = std::forward<Func>(func);
    }

    // Execute the command handler for the given command
    void executeCommand(const Command& command) const
    {
        assert(m_router.size() > static_cast<std::size_t>(command.type));
        m_router[static_cast<std::size_t>(command.type)](command);
    }

// Private member variables
private:
    commandRouter_t m_router;
};

template <typename CommandEnum>
class dram_base {
// Public type definitions
public:
    using commandEnum_t = CommandEnum;
    using commandCount_t = std::vector<std::size_t>;
    using commandRouter_t = Router<commandEnum_t>;
    // PatternHandler
    using patternHandler_t = PatternHandler<commandEnum_t>;
    // ImplicitCommandHandler
    using ImplicitCommandHandler_t = ImplicitCommandHandler;
    using implicitCommandInserter_t = typename ImplicitCommandHandler::Inserter_t;
    // ExtensionManager
    using extension_manager_t = util::extension_manager::ExtensionManager<
        extensions::Base
    >;

// Public constructors and assignment operators
public:
    dram_base() = delete; // No default constructor
    dram_base(const dram_base&) = delete; // No copy constructor
    dram_base& operator=(const dram_base&) = delete; // No copy assignment operator
    dram_base(dram_base&&) = default; // Move constructor
    dram_base& operator=(dram_base&&) = default; // Move assignment operator
    virtual ~dram_base() = 0;
// Protected constructors
protected:
    dram_base(PatternEncoderOverrides encoderoverrides, uint64_t initPattern = 0)
        : m_commandCoreCount(static_cast<std::size_t>(commandEnum_t::COUNT), 0)
        , m_commandInterfaceCount(static_cast<std::size_t>(commandEnum_t::COUNT), 0)
        , m_commandCoreRouter(std::nullopt)
        , m_commandInterfaceRouter(std::nullopt)
        , m_patternHandler(encoderoverrides, initPattern)
    {}

// Interface
private:
    virtual timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<ToggleRateDefinition> &toggleRateDefinition) = 0;
public:
    virtual energy_t calcCoreEnergy(timestamp_t timestamp) = 0;
    virtual interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) = 0;
    virtual SimulationStats getWindowStats(timestamp_t timestamp) = 0;
    virtual uint64_t getBankCount() = 0;
    virtual uint64_t getRankCount() = 0;
    virtual uint64_t getDeviceCount() = 0;

// Getters for member variables
public:
    // ExtensionManager
    extension_manager_t& getExtensionManager() { return m_extensionManager; }
    const extension_manager_t& getExtensionManager() const { return m_extensionManager; }
protected:
    // PatternHandler
    patternHandler_t& getPatternHandler() { return m_patternHandler; }
    const patternHandler_t& getPatternHandler() const { return m_patternHandler; }
    // ImplicitCommandHandler
    ImplicitCommandHandler_t& getImplicitCommandHandler() { return m_ImplicitCommandHandler; }
    const ImplicitCommandHandler_t& getImplicitCommandHandler() const { return m_ImplicitCommandHandler; }
    // Router Core
    commandRouter_t& getCommandCoreRouter() { return m_commandCoreRouter; }
    const commandRouter_t& getCommandCoreRouter() const { return m_commandCoreRouter; }
    // Router Interface
    commandRouter_t& getCommandInterfaceRouter() { return m_commandInterfaceRouter; }
    const commandRouter_t& getCommandInterfaceRouter() const { return m_commandInterfaceRouter; }


// Router forwards
protected:
    template <commandEnum_t cmd, typename Func>
    void routeCoreCommand(Func&& func)
    {
        m_commandCoreRouter.template routeCommand<cmd>(std::forward<Func>(func));
    }

    template <commandEnum_t cmd, typename Func>
    void routeInterfaceCommand(Func&& func)
    {
        m_commandInterfaceRouter.template routeCommand<cmd>(std::forward<Func>(func));
    }

// ImplicitCommandHandler forwards
public:
    // Forward implicit command count
    std::size_t implicitCommandCount() const { return m_ImplicitCommandHandler.getInserter().implicitCommandCount(); }
protected:
    void processImplicitCommandQueue(timestamp_t timestamp)
    {
        m_ImplicitCommandHandler.processImplicitCommandQueue(timestamp, m_last_command_time);
    }


// Private member functions
private:
    void doCommand(const Command& command, commandCount_t& commandCount, commandRouter_t& commandRouter)
    {
        assert(commandCount.size() > static_cast<std::size_t>(command.type));
        assert(commandRouter.size() > static_cast<std::size_t>(command.type));

        // Process implicit command list
        m_ImplicitCommandHandler.processImplicitCommandQueue(command.timestamp, this->last_command_time);

        commandCount[static_cast<std::size_t>(command.type)]++;
        commandRouter.executeCommand(command);

        m_last_command_time = command.timestamp;
    }

// Public member functions
public:
    void doCoreCommand(const Command& command)
    {
        doCommand(command, m_commandCoreCount, m_commandCoreRouter);
    }

    void doInterfaceCommand(const Command& command)
    {
        doCommand(command, m_commandInterfaceCount, m_commandInterfaceRouter);
    }

    void doCoreInterfaceCommand(const Command& command)
    {
        doCoreCommand(command);
        doInterfaceCommand(command);
    }

    void setToggleRate(timestamp_t timestamp, const std::optional<ToggleRateDefinition> &toggleRateDefinition)
    {
        update_toggling_rate(timestamp, toggleRateDefinition);
    }

    auto getCommandCoreCount(commandEnum_t cmd) const
    {
        assert(m_commandCoreCount.size() > static_cast<std::size_t>(cmd));

        return this->m_commandCoreCount[static_cast<std::size_t>(cmd)];
    }

    auto getCommandInterfaceCount(commandEnum_t cmd) const
    {
        assert(m_commandInterfaceCount.size() > static_cast<std::size_t>(cmd));

        return this->m_commandInterfaceCount[static_cast<std::size_t>(cmd)];
    }

    timestamp_t getLastCommandTime() const
    {
        return m_last_command_time;
    }

    double getTotalEnergy(timestamp_t timestamp)
    {
        return calcCoreEnergy(timestamp).total() + calcInterfaceEnergy(timestamp).total();
    };

    SimulationStats getStats() {
        return getWindowStats(getLastCommandTime());
    }

// Private member variables
private:
    // Counter
    commandCount_t m_commandCoreCount;
    commandCount_t m_commandInterfaceCount;
    // Router
    commandRouter_t m_commandCoreRouter;
    commandRouter_t m_commandInterfaceRouter;
    // PatternHandler
    patternHandler_t m_patternHandler;
    // ExtensionManager
    extension_manager_t m_extensionManager;
    // ImplicitCommandHandler
    ImplicitCommandHandler_t m_ImplicitCommandHandler;
    // Last command time
    timestamp_t m_last_command_time;

};

template <typename CommandEnum>
dram_base<CommandEnum>::~dram_base() = default;

}

#endif /* DRAMPOWER_DDR_DRAM_BASE_H */
