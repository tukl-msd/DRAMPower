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

#include <DRAMUtils/config/toggling_rate.h>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/util/cycle_stats.h>
#include <DRAMPower/util/clock.h>

#include <stdint.h>
#include <optional>

namespace DRAMPower {

namespace internal {
    template<typename Standard, typename Core, typename Interface>
    class TestAccessor;
}

class HBM2 : public dram_base<CmdType> {
// Friend classes
friend class internal::TestAccessor<HBM2, HBM2Core, HBM2Interface>;

// Public constructors and assignment operators
public:
    HBM2() = delete; // No default constructor
    HBM2(const HBM2&) = default; // copy constructor
    HBM2& operator=(const HBM2&) = delete; // copy assignment operator
    HBM2(HBM2&&) = default; // move constructor
    HBM2& operator=(HBM2&&) = delete; // move assignment operator
    ~HBM2() override = default;
    
    HBM2(const MemSpecHBM2 &memSpec);

// Overrides
private:
    timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) override;
public:
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;
    SimulationStats getWindowStats(timestamp_t timestamp) override;
    util::CLIArchitectureConfig getCLIArchitectureConfig() override;
    void serialize_impl(std::ostream& stream) const override;
    void deserialize_impl(std::istream& stream) override;

    
// Private member functions
private:
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
    void registerCommands();
    void registerExtensions();
    void endOfSimulation(timestamp_t timestamp);

// Private member variables
private:
    MemSpecHBM2 m_memSpec;
    HBM2Interface m_interface;
    HBM2Core m_core;
};

};

#endif /* DRAMPOWER_STANDARDS_HBM2_HBM2_H */
