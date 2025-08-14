#ifndef DRAMPOWER_DRAM_DRAM_BASE_H
#define DRAMPOWER_DRAM_DRAM_BASE_H

#include <DRAMPower/command/Command.h>
#include <DRAMPower/command/Pattern.h>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>
#include <DRAMPower/util/extension_manager.h>
#include <DRAMPower/util/extensions.h>

#include <DRAMPower/util/Router.h>
#include <DRAMPower/util/PatternHandler.h>
#include <DRAMPower/util/ImplicitCommandHandler.h>
#include <DRAMPower/util/cli_architecture_config.h>
#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

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

template <typename CommandEnum>
class dram_base : public util::Serialize, public util::Deserialize
{
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
    dram_base(const dram_base&) = default; // copy constructor
    dram_base& operator=(const dram_base&) = default; // copy assignment operator
    dram_base(dram_base&&) = default; // Move constructor
    dram_base& operator=(dram_base&&) = default; // Move assignment operator
    virtual ~dram_base() = 0;
// Protected constructors
protected:
    dram_base()
        : m_commandCoreCount(static_cast<std::size_t>(commandEnum_t::COUNT), 0)
        , m_commandInterfaceCount(static_cast<std::size_t>(commandEnum_t::COUNT), 0)
        , m_commandCoreRouter(std::nullopt)
        , m_commandInterfaceRouter(std::nullopt)
    {}

// Interface
private:
    virtual timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<ToggleRateDefinition> &toggleRateDefinition) = 0;
public:
    virtual energy_t calcCoreEnergy(timestamp_t timestamp) = 0;
    virtual interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) = 0;
    virtual SimulationStats getWindowStats(timestamp_t timestamp) = 0;
    virtual util::CLIArchitectureConfig getCLIArchitectureConfig() = 0;
public:
    virtual void serialize_impl(std::ostream& stream) const = 0;
    void serialize(std::ostream& stream) const override
    {
        // Serialize the command counts
        for (const auto& count : m_commandCoreCount) {
            stream.write(reinterpret_cast<const char*>(&count), sizeof(count));
        }
        for (const auto& count : m_commandInterfaceCount) {
            stream.write(reinterpret_cast<const char*>(&count), sizeof(count));
        }
        // Serialize the last command time
        stream.write(reinterpret_cast<const char*>(&m_last_command_time), sizeof(m_last_command_time));
        // Serialize the extension manager
        m_extensionManager.serialize(stream);
        serialize_impl(stream);
    }
    virtual void deserialize_impl(std::istream& stream) = 0;
    void deserialize(std::istream& stream) override
    {
        // Deserialize the command counts
        for (auto &count : m_commandCoreCount) {
            stream.read(reinterpret_cast<char*>(&count), sizeof(count));
        }
        for (auto &count : m_commandInterfaceCount) {
            stream.read(reinterpret_cast<char*>(&count), sizeof(count));
        }
        // Deserialize the last command time
        stream.read(reinterpret_cast<char*>(&m_last_command_time), sizeof(m_last_command_time));
        // Deserialize the extension manager
        m_extensionManager.deserialize(stream);
        deserialize_impl(stream);
    }
    bool isSerializable() {
        return 0 == m_ImplicitCommandHandler.implicitCommandCount();
    }

// Getters for member variables
public:
    // ExtensionManager
    extension_manager_t& getExtensionManager() { return m_extensionManager; }
    const extension_manager_t& getExtensionManager() const { return m_extensionManager; }
protected:
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
    std::size_t implicitCommandCount() const { return m_ImplicitCommandHandler.implicitCommandCount(); }
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
        m_ImplicitCommandHandler.processImplicitCommandQueue(command.timestamp, m_last_command_time);

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
