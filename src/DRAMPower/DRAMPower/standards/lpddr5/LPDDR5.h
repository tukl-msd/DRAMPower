#ifndef DRAMPOWER_STANDARDS_LPDDR5_LPDDR5_H
#define DRAMPOWER_STANDARDS_LPDDR5_LPDDR5_H

#include <memory>
#include <stdint.h>

#include <DRAMUtils/config/toggling_rate.h>

#include "DRAMPower/Types.h"
#include "DRAMPower/data/energy.h"
#include <DRAMPower/dram/Interface.h>
#include "DRAMPower/dram/dram_base.h"
#include "DRAMPower/memspec/MemSpecLPDDR5.h"
#include "DRAMPower/standards/lpddr5/LPDDR5Core.h"
#include "DRAMPower/standards/lpddr5/LPDDR5Interface.h"
#include "DRAMPower/util/cli_architecture_config.h"

namespace DRAMPower {

namespace internal {
    template<typename Standard, typename Core, typename Interface>
    class TestAccessor;
}

class LPDDR5 : public dram_base<CmdType> {
// Friend classes
friend class internal::TestAccessor<LPDDR5, LPDDR5Core, LPDDR5Interface>;

// Public constructors and assignment operators
public:
    LPDDR5() = delete; // No default constructor
    LPDDR5(const LPDDR5&) = default; // copy constructor
    LPDDR5& operator=(const LPDDR5&) = delete; // copy assignment operator
    LPDDR5(LPDDR5&&) = default; // move constructor
    LPDDR5& operator=(LPDDR5&&) = delete; // move assignment operator
    ~LPDDR5() override = default;
    LPDDR5(const MemSpecLPDDR5& memSpec);

// Overrides
private:
    timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) override;
public:
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;
    SimulationStats getWindowStats(timestamp_t timestamp) override;
    util::CLIArchitectureConfig getCLIArchitectureConfig() override;

// Private member functions
private:
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
    void registerCommands();
    void registerExtensions();
    void endOfSimulation(timestamp_t timestamp);

// Private member variables
private:
    MemSpecLPDDR5 m_memSpec;
    LPDDR5Interface m_interface;
    LPDDR5Core m_core;

};
};  // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR5_LPDDR5_H */
