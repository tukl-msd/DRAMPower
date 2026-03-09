#ifndef DRAMPOWER_STANDARDS_LPDDR6_LPDDR6_H
#define DRAMPOWER_STANDARDS_LPDDR6_LPDDR6_H

#include <stdint.h>

#include <DRAMUtils/config/toggling_rate.h>

#include "DRAMPower/Types.h"
#include "DRAMPower/data/energy.h"
#include <DRAMPower/dram/Interface.h>
#include "DRAMPower/dram/dram_base.h"
#include "DRAMPower/memspec/MemSpecLPDDR6.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Core.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Interface.h"
#include "DRAMPower/util/cli_architecture_config.h"

namespace DRAMPower {

namespace internal {
    template<typename Standard, typename Core, typename Interface>
    class TestAccessor;
}

class LPDDR6 : public dram_base<CmdType> {
// Friend classes
friend class internal::TestAccessor<LPDDR6, LPDDR6Core, LPDDR6Interface>;

// Public constructors and assignment operators
public:
    LPDDR6() = delete; // No default constructor
    LPDDR6(const LPDDR6&) = default; // copy constructor
    LPDDR6& operator=(const LPDDR6&) = delete; // copy assignment operator
    LPDDR6(LPDDR6&&) = default; // move constructor
    LPDDR6& operator=(LPDDR6&&) = delete; // move assignment operator
    ~LPDDR6() override = default;
    LPDDR6(const MemSpecLPDDR6& memSpec);

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
    LPDDR6Core& getCore() {
        return m_core;
    }
    const LPDDR6Core& getCore() const {
        return m_core;
    }
    LPDDR6Interface& getInterface() {
        return m_interface;
    }
    const LPDDR6Interface& getInterface() const {
        return m_interface;
    }
    void registerCommands();
    void registerExtensions();
    void endOfSimulation(timestamp_t timestamp);

// Private member variables
private:
    MemSpecLPDDR6 m_memSpec;
    LPDDR6Interface m_interface;
    LPDDR6Core m_core;
};

}  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR6_LPDDR6_H */
