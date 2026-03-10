#ifndef DRAMPOWER_DRAM_DRAM_BASE_H
#define DRAMPOWER_DRAM_DRAM_BASE_H

#include <DRAMPower/command/Command.h>

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

#include <cassert>
#include <vector>

namespace DRAMPower {

using namespace DRAMUtils::Config;

template <typename CommandEnum>
class dram_base : public util::Serialize, public util::Deserialize
{
// Public type definitions
public:
    using commandEnum_t = CommandEnum;
    using commandCount_t = std::vector<std::size_t>;
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
    dram_base() = default;

// Public member functions
public:
    // ExtensionManager
    extension_manager_t& getExtensionManager() { return m_extensionManager; }
    const extension_manager_t& getExtensionManager() const { return m_extensionManager; }

    void doCoreCommand(const Command& command) {
        doCoreCommandImpl(command);
    }
    
    void doInterfaceCommand(const Command& command) {
        doInterfaceCommandImpl(command);
    }

    void doCommand(const Command& command) {
        doCoreCommandImpl(command);
        doInterfaceCommandImpl(command);
    }

    timestamp_t getLastCommandTime() const {
        return getLastCommandTime_impl();
    }

    double getTotalEnergy(timestamp_t timestamp) {
        return calcCoreEnergy(timestamp).total() + calcInterfaceEnergy(timestamp).total();
    };

    SimulationStats getStats() {
        return getWindowStats(getLastCommandTime());
    }

    void serialize(std::ostream& stream) const override {
        // Serialize the extension manager
        m_extensionManager.serialize(stream);
        serialize_impl(stream);
    }

    void deserialize(std::istream& stream) override {
        // Deserialize the extension manager
        m_extensionManager.deserialize(stream);
        deserialize_impl(stream);
    }

// Public virtual methods
public:
    virtual energy_t calcCoreEnergy(timestamp_t timestamp) = 0;
    virtual interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) = 0;
    virtual SimulationStats getWindowStats(timestamp_t timestamp) = 0;
    virtual util::CLIArchitectureConfig getCLIArchitectureConfig() = 0;
    virtual bool isSerializable() const = 0;

// Private virtual methods
private:
    virtual void doCoreCommandImpl(const Command& command) = 0;
    virtual void doInterfaceCommandImpl(const Command& command) = 0;
    virtual timestamp_t getLastCommandTime_impl() const = 0;
    virtual void serialize_impl(std::ostream& stream) const = 0;
    virtual void deserialize_impl(std::istream& stream) = 0;

// Private member variables
private:
    extension_manager_t m_extensionManager;
};

template <typename CommandEnum>
dram_base<CommandEnum>::~dram_base() = default;

}

#endif /* DRAMPOWER_DDR_DRAM_BASE_H */
