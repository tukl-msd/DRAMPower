#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4_H

#include <DRAMPower/standards/ddr4/DDR4Interface.h>
#include <DRAMPower/standards/ddr4/DDR4Core.h>

#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/dram/Interface.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/memspec/MemSpecDDR4.h>
#include <DRAMPower/util/dbi.h>

#include <DRAMUtils/config/toggling_rate.h>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/util/cycle_stats.h>
#include <DRAMPower/util/clock.h>

#include <stdint.h>
#include <optional>

namespace DRAMPower {

class DDR4 : public dram_base<CmdType> {
// Public constructors and assignment operators
public:
    DDR4() = delete; // No default constructor
    DDR4(const DDR4&) = delete; // No copy constructor
    DDR4& operator=(const DDR4&) = delete; // No copy assignment operator
    DDR4(DDR4&&) = delete; // no move constructor
    DDR4& operator=(DDR4&&) = delete; // no move assignment operator
    DDR4(const MemSpecDDR4 &memSpec);
    ~DDR4() override = default;

// Overrides
private:
    timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) override;
public:
    energy_t calcCoreEnergy(timestamp_t timestamp) override;
    interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) override;
    SimulationStats getWindowStats(timestamp_t timestamp) override;
    uint64_t getBankCount() override;
    uint64_t getRankCount() override;
    uint64_t getDeviceCount() override;

#ifdef DRAMPOWER_TESTING
public:
    const std::vector<Rank>& getCoreRanks() const {
        return m_core.getRanks();
    }
    std::vector<Rank>& getCoreRanks() {
        return m_core.getRanks();
    }
    const DDR4Interface& getInterface() const {
        return m_interface;
    }
    DDR4Interface& getInterface() {
        return m_interface;
    }
#endif

// Private member functions
private:
    void registerCommands();
    void registerExtensions();
    void endOfSimulation(timestamp_t timestamp);

// Private member variables
private:
    MemSpecDDR4 m_memSpec;
    DDR4Interface m_interface;
    DDR4Core m_core;
};

};

#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4_H */
