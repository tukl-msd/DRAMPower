#ifndef DRAMPOWER_STANDARDS_LPDDR5_LPDDR5_H
#define DRAMPOWER_STANDARDS_LPDDR5_LPDDR5_H

#include "DRAMPower/util/cli_architecture_config.h"

#include <DRAMPower/Types.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>

#include <DRAMPower/standards/lpddr5/LPDDR5Interface.h>
#include <DRAMPower/standards/lpddr5/LPDDR5Core.h>
#include <DRAMPower/memspec/MemSpecLPDDR5.h>

#include "DRAMPower/simconfig/simconfig.h"

#include <algorithm>

namespace DRAMPower {

class LPDDR5 : public dram_base<CmdType> {
// Public constructors and assignment operators
public:
    LPDDR5() = delete; // No default constructor
    LPDDR5(const LPDDR5&) = default; // copy constructor
    LPDDR5& operator=(const LPDDR5&) = default; // copy assignment operator
    LPDDR5(LPDDR5&&) = default; // move constructor
    LPDDR5& operator=(LPDDR5&&) = default; // move assignment operator
    ~LPDDR5() override = default;
    LPDDR5(const MemSpecLPDDR5& memSpec, const config::SimConfig& toggleRate = {});

// Public member functions
public:
// Member functions
    LPDDR5Core& getCore() {
        return m_core;
    }
    const LPDDR5Core& getCore() const {
        return m_core;
    }
    LPDDR5Interface& getInterface() {
        return m_interface;
    }
    const LPDDR5Interface& getInterface() const {
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
    MemSpecLPDDR5 m_memSpec;
    LPDDR5Interface m_interface;
    LPDDR5Core m_core;
};

}  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR5_LPDDR5_H */
