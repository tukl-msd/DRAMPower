#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4_H

#include "DRAMPower/standards/ddr4/CoreWrapper.h"
#include "DRAMPower/util/cli_architecture_config.h"
#include <DRAMPower/standards/ddr4/DDR4Interface.h>
#include <DRAMPower/standards/ddr4/DDR4Core.h>

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

#include <algorithm>
#include <stdint.h>

namespace DRAMPower {

namespace internal {
    template<typename Standard, typename Core, typename Interface>
    class TestAccessor;
}

class DDR4 : public dram_base<CmdType> {
// Friend classes
friend class internal::TestAccessor<DDR4, DDR4Core, DDR4Interface>;

// Public constructors and assignment operators
public:
    DDR4() = delete; // No default constructor
    DDR4(const DDR4&) = default; // copy constructor
    DDR4& operator=(const DDR4&) = delete; // copy assignment operator
    DDR4(DDR4&&) = default; // move constructor
    DDR4& operator=(DDR4&&) = delete; // move assignment operator
    ~DDR4() override = default;
    
    DDR4(const MemSpecDDR4 &memSpec);

// Overrides
public:
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;
    SimulationStats getWindowStats(timestamp_t timestamp) override;
    util::CLIArchitectureConfig getCLIArchitectureConfig() override;
    bool isSerializable() const override {
        return m_core.isSerializable();
    }
    timestamp_t getLastCommandTime() const {
        return std::max(m_core.getLastCommandTime(), m_interface.getLastCommandTime());
    }
private:
    void doCoreCommandImpl(const Command& command) override {
        m_core.doCommand(command);
    }
    void doInterfaceCommandImpl(const Command& command) override {
        m_interface.doCommand(command);
    }
    timestamp_t getLastCommandTime_impl() const override {
        return getLastCommandTime();
    }
    void serialize_impl(std::ostream& stream) const override;
    void deserialize_impl(std::istream& stream) override;

    
// Private member functions
private:
    DDR4Core& getCore() {
        return m_core.getCore();
    }
    const DDR4Core& getCore() const {
        return m_core.getCore();
    }
    DDR4Interface& getInterface() {
        return m_interface;
    }
    const DDR4Interface& getInterface() const {
        return m_interface;
    }
    void registerExtensions();
    void endOfSimulation(timestamp_t timestamp);

// Private member variables
private:
    MemSpecDDR4 m_memSpec;
    DDR4Interface m_interface;
    CoreWrapper<DDR4Core> m_core;
    timestamp_t m_last_command_time;
};

};

#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4_H */
