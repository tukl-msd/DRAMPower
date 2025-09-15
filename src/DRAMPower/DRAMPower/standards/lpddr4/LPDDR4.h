#ifndef DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H
#define DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H

#include "DRAMPower/data/stats.h"
#include "DRAMPower/standards/lpddr4/LPDDR4Interface.h"
#include "DRAMPower/standards/lpddr4/LPDDR4Core.h"
#include "DRAMPower/util/cli_architecture_config.h"
#include <DRAMPower/util/bus.h>
#include <DRAMPower/util/databus.h>
#include <DRAMPower/util/databus_presets.h>
#include <DRAMPower/util/clock.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/dram/Interface.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/memspec/MemSpecLPDDR4.h>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/util/cycle_stats.h>
#include <DRAMPower/util/databus.h>

#include <DRAMUtils/config/toggling_rate.h>

#include <memory>
#include <stdint.h>
#include <optional>

namespace DRAMPower {

namespace internal {
    template<typename Standard, typename Core, typename Interface>
    class TestAccessor;
}

class LPDDR4 : public dram_base<CmdType>{
// Friend classes
friend class internal::TestAccessor<LPDDR4, LPDDR4Core, LPDDR4Interface>;

// public constructors and assignment operators
public:
    LPDDR4() = delete; // No default constructor
    LPDDR4(const LPDDR4&) = default; // copy constructor
    LPDDR4& operator=(const LPDDR4&) = delete; // copy assignment operator
    LPDDR4(LPDDR4&&) = default; // move constructor
    LPDDR4& operator=(LPDDR4&&) = delete; // move assignment operator
    ~LPDDR4() override = default;
    
    LPDDR4(const MemSpecLPDDR4& memSpec);

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
    void registerCommands();
    void registerExtensions();
    void endOfSimulation(timestamp_t timestamp);

// Private member variables
private:
    MemSpecLPDDR4 m_memSpec;
    LPDDR4Interface m_interface;
    LPDDR4Core m_core;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR4_LPDDR4_H */
