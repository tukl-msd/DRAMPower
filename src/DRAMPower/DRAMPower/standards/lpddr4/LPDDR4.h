#ifndef DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H
#define DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H

#include "DRAMPower/util/cli_architecture_config.h"

#include <DRAMPower/Types.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>

#include <DRAMPower/standards/lpddr4/LPDDR4Interface.h>
#include <DRAMPower/standards/lpddr4/LPDDR4Core.h>
#include <DRAMPower/memspec/MemSpecLPDDR4.h>

#include "DRAMPower/simconfig/simconfig.h"

#include <algorithm>

namespace DRAMPower {

class LPDDR4 : public dram_base<CmdType>{
// public constructors and assignment operators
public:
    LPDDR4() = delete; // No default constructor
    LPDDR4(const LPDDR4&) = default; // copy constructor
    LPDDR4& operator=(const LPDDR4&) = default; // copy assignment operator
    LPDDR4(LPDDR4&&) = default; // move constructor
    LPDDR4& operator=(LPDDR4&&) = default; // move assignment operator
    ~LPDDR4() override = default;
    
    LPDDR4(const MemSpecLPDDR4& memSpec, const config::SimConfig &simConfig = {});

// Public member functions
public:
// Member functions
    LPDDR4Core& getCore() {
        return m_core;
    }
    const LPDDR4Core& getCore() const {
        return m_core;
    }
    LPDDR4Interface& getInterface() {
        return m_interface;
    }
    const LPDDR4Interface& getInterface() const {
        return m_interface;
    }
// Overrides
    energy_t calcCoreEnergyStats(const SimulationStats& stats) const override;
    interface_energy_info_t calcInterfaceEnergyStats(const SimulationStats& stats) const override;
    SimulationStats getWindowStats(timestamp_t timestamp) override;
    util::CLIArchitectureConfig getCLIArchitectureConfig() override;
    bool isSerializable() const override {
        return m_core.isSerializable();
    }


// Private member functions
private:
// Member functions
    void registerExtensions();
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
    MemSpecLPDDR4 m_memSpec;
    LPDDR4Interface m_interface;
    LPDDR4Core m_core;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H */
