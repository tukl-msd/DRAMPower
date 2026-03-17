#ifndef DRAMPOWER_STANDARDS_DDR5_DDR5_H
#define DRAMPOWER_STANDARDS_DDR5_DDR5_H


#include "DRAMPower/util/CoreWrapper.h"
#include "DRAMPower/util/cli_architecture_config.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/dram/dram_base.h"
#include <DRAMPower/command/Command.h>
#include "DRAMPower/data/energy.h"
#include <DRAMPower/data/stats.h>

#include "DRAMPower/standards/ddr5/DDR5Core.h"
#include "DRAMPower/standards/ddr5/DDR5Interface.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"

#include "DRAMUtils/config/toggling_rate.h"

#include <algorithm>

namespace DRAMPower {

class DDR5 : public dram_base<CmdType> {
// public constructors and assignment operators
public:
    DDR5() = delete; // No default constructor
    DDR5(const DDR5&) = default; // copy constructor
    DDR5& operator=(const DDR5&) = delete; // copy assignment operator
    DDR5(DDR5&&) = default; // move constructor
    DDR5& operator=(DDR5&&) = delete; // move assignment operator
    ~DDR5() override = default;

    DDR5(const MemSpecDDR5& memSpec, const DRAMUtils::Config::ToggleRateDefinition& toggleRate);

// Public member functions
public:
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;
    SimulationStats getWindowStats(timestamp_t timestamp) override;
    util::CLIArchitectureConfig getCLIArchitectureConfig() override;
    bool isSerializable() const override {
        return m_core.isSerializable();
    }
// member functions
    DDR5Core& getCore() {
        return m_core.getCore();
    }
    const DDR5Core& getCore() const {
        return m_core.getCore();
    }
    DDR5Interface& getInterface() {
        return m_interface;
    }
    const DDR5Interface& getInterface() const {
        return m_interface;
    }

// Private member functions
private:
// Overrides
    void doCoreCommandImpl(const Command& command) override {
        m_core.doCommand(command);
    }
    void doInterfaceCommandImpl(const Command& command) override {
        m_interface.doCommand(command);
    }
    timestamp_t getLastCommandTime_impl() const override {
        return std::max(m_core.getLastCommandTime(), m_interface.getLastCommandTime());
    }
    void serialize_impl(std::ostream& stream) const override;
    void deserialize_impl(std::istream& stream) override;

// Private member variables
private:
    MemSpecDDR5 m_memSpec;
    DDR5Interface m_interface;
    CoreWrapper<DDR5Core> m_core;
};

}  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR5_DDR5_H */
