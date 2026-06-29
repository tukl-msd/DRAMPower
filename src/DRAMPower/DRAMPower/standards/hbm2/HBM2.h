#ifndef DRAMPOWER_STANDARDS_HBM2_HBM2_H
#define DRAMPOWER_STANDARDS_HBM2_HBM2_H

#include "DRAMPower/util/cli_architecture_config.h"
#include <DRAMPower/standards/hbm2/HBM2Interface.h>
#include <DRAMPower/standards/hbm2/HBM2Core.h>

#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/dram/Interface.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/memspec/MemSpecHBM2.h>
#include <DRAMPower/util/dbi.h>

#include "DRAMPower/simconfig/simconfig.h"

#include <DRAMUtils/config/toggling_rate.h>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/util/cycle_stats.h>
#include <DRAMPower/util/clock.h>

#include <stdint.h>

namespace DRAMPower {

class HBM2 : public dram_base<CmdType> {
// Public constructors and assignment operators
public:
    HBM2() = delete; // No default constructor
    HBM2(const HBM2&) = default; // copy constructor
    HBM2(HBM2&&) noexcept = default; // move constructor
    HBM2& operator=(const HBM2&) = default; // copy assignment operator
    HBM2& operator=(HBM2&&) = default; // move assignment operator
    ~HBM2() override = default;
    
    HBM2(const MemSpecHBM2 &memSpec, const config::SimConfig &simConfig = {});

// Public member functions
public:
// Member functions
    HBM2Core& getCore() {
        return m_core;
    }
    const HBM2Core& getCore() const {
        return m_core;
    }
    HBM2Interface& getInterface() {
        return m_interface;
    }
    const HBM2Interface& getInterface() const {
        return m_interface;
    }

// Overrides
public:
    energy_t calcCoreEnergyStats(const SimulationStats& stats) const override;
    interface_energy_info_t calcInterfaceEnergyStats(const SimulationStats& stats) const override;
    SimulationStats getWindowStats(timestamp_t timestamp) override;
    util::CLIArchitectureConfig getCLIArchitectureConfig() override;
    bool isSerializable() const override {
        return m_core.isSerializable();
    }
    void setSimulationTime(timestamp_t timestamp) override;
    void reset() override;

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
    MemSpecHBM2 m_memSpec;
    HBM2Interface m_interface;
    HBM2Core m_core;
};

};

#endif /* DRAMPOWER_STANDARDS_HBM2_HBM2_H */
