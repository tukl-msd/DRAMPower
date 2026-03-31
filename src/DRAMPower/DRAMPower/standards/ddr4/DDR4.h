#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4_H

#include "DRAMPower/util/cli_architecture_config.h"

#include <DRAMPower/Types.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>

#include <DRAMPower/standards/ddr4/DDR4Interface.h>
#include <DRAMPower/standards/ddr4/DDR4Core.h>
#include <DRAMPower/memspec/MemSpecDDR4.h>
#include <DRAMPower/standards/ddr4/core_calculation_DDR4.h>
#include <DRAMPower/standards/ddr4/interface_calculation_DDR4.h>

#include "DRAMPower/simconfig/simconfig.h"

#include <algorithm>

namespace DRAMPower {

class DDR4 : public dram_base<CmdType> {
// Public constructors and assignment operators
public:
    DDR4() = delete; // No default constructor
    DDR4& operator=(const DDR4&) = delete; // copy assignment operator
    DDR4& operator=(DDR4&&) = delete; // move assignment operator
    ~DDR4() override = default;
    
    DDR4(const MemSpecDDR4 &memSpec, const config::SimConfig &simConfig = {});
    DDR4(const DDR4& other)
        : m_memSpec(other.m_memSpec)
        , m_interface(other.m_interface, m_memSpec)
        , m_core(other.m_core, m_memSpec)
        , m_calc_interface(m_memSpec)
        , m_calc_core(m_memSpec)
    {}
    DDR4(DDR4&& other) noexcept // TODO
        : m_memSpec(std::move(other.m_memSpec))
        , m_interface(std::move(other.m_interface), m_memSpec)
        , m_core(std::move(other.m_core), m_memSpec)
        , m_calc_interface(m_memSpec)
        , m_calc_core(m_memSpec)
    {}

// Public member functions
public:
// Member functions
    DDR4Core& getCore() {
        return m_core;
    }
    const DDR4Core& getCore() const {
        return m_core;
    }
    DDR4Interface& getInterface() {
        return m_interface;
    }
    const DDR4Interface& getInterface() const {
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
    MemSpecDDR4 m_memSpec;
    DDR4Interface m_interface;
    DDR4Core m_core;
    InterfaceCalculation_DDR4 m_calc_interface;
    Calculation_DDR4 m_calc_core;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4_H */
