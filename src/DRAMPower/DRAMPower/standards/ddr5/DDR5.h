#ifndef DRAMPOWER_STANDARDS_DDR5_DDR5_H
#define DRAMPOWER_STANDARDS_DDR5_DDR5_H

#include <memory>
#include <optional>

#include <DRAMUtils/config/toggling_rate.h>

#include "DRAMPower/standards/ddr5/DDR5Core.h"
#include "DRAMPower/standards/ddr5/DDR5Interface.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/data/energy.h"
#include <DRAMPower/dram/Interface.h>
#include "DRAMPower/dram/dram_base.h"
#include "DRAMPower/util/cli_architecture_config.h"

namespace DRAMPower {

namespace internal {
    template<typename Standard, typename Core, typename Interface>
    class TestAccessor;
}

class DDR5 : public dram_base<CmdType> {
// Friend classes
friend class internal::TestAccessor<DDR5, DDR5Core, DDR5Interface>;

// public constructors and assignment operators
public:
    DDR5() = delete; // No default constructor
    DDR5(const DDR5&) = default; // copy constructor
    DDR5& operator=(const DDR5&) = delete; // copy assignment operator
    DDR5(DDR5&&) = default; // move constructor
    DDR5& operator=(DDR5&&) = delete; // move assignment operator
    ~DDR5() override = default;

    DDR5(const MemSpecDDR5& memSpec);

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
    DDR5Core& getCore() {
        return m_core;
    }
    const DDR5Core& getCore() const {
        return m_core;
    }
    DDR5Interface& getInterface() {
        return m_interface;
    }
    const DDR5Interface& getInterface() const {
        return m_interface;
    }
    void registerCommands();
    void registerExtensions();
    void endOfSimulation(timestamp_t timestamp);

// Private member variables
private:
    MemSpecDDR5 m_memSpec;
    DDR5Interface m_interface;
    DDR5Core m_core;
};

}  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR5_DDR5_H */
