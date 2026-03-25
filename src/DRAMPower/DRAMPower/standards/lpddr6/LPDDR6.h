#ifndef DRAMPOWER_STANDARDS_LPDDR6_LPDDR6_H
#define DRAMPOWER_STANDARDS_LPDDR6_LPDDR6_H

#include "DRAMPower/simconfig/simconfig.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/dram/dram_base.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/memspec/MemSpecLPDDR6.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Core.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Interface.h"
#include "DRAMPower/util/CoreWrapper.h"
#include "DRAMPower/util/cli_architecture_config.h"

#include <algorithm>

namespace DRAMPower {

class LPDDR6 : public dram_base<CmdType> {
// Public constructors and assignment operators
public:
    LPDDR6() = delete; // No default constructor
    LPDDR6(const LPDDR6&) = default; // copy constructor
    LPDDR6& operator=(const LPDDR6&) = delete; // copy assignment operator
    LPDDR6(LPDDR6&&) = default; // move constructor
    LPDDR6& operator=(LPDDR6&&) = delete; // move assignment operator
    ~LPDDR6() override = default;
    LPDDR6(const MemSpecLPDDR6& memSpec, const config::SimConfig &simConfig = {});

// Public member functions
public:
// Member functions
    LPDDR6Core& getCore() {
        return m_core.getCore();
    }
    const LPDDR6Core& getCore() const {
        return m_core.getCore();
    }
    LPDDR6Interface& getInterface() {
        return m_interface;
    }
    const LPDDR6Interface& getInterface() const {
        return m_interface;
    }
// Overrided
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;
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
    MemSpecLPDDR6 m_memSpec;
    LPDDR6Interface m_interface;
    CoreWrapper<LPDDR6Core> m_core;
};

}  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR6_LPDDR6_H */
