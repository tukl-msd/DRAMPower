#include "DRAMPower/standards/ddr5/DDR5.h"

#include <iostream>

#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/standards/ddr5/DDR5Interface.h"
#include "DRAMPower/standards/ddr5/core_calculation_DDR5.h"
#include "DRAMPower/standards/ddr5/interface_calculation_DDR5.h"
#include <DRAMPower/util/extensions.h>

namespace DRAMPower {

    using namespace DRAMUtils::Config;

    DDR5::DDR5(const MemSpecDDR5 &memSpec, const config::SimConfig &simConfig)
        : m_memSpec(memSpec)
        , m_interface(m_memSpec, simConfig)
        , m_core(m_memSpec)
    {}

// Getters for CLI
    util::CLIArchitectureConfig DDR5::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec.numberOfDevices,
            m_memSpec.numberOfRanks,
            m_memSpec.numberOfBanks
        };
    }

// Calculation
    energy_t DDR5::calcCoreEnergyStats(const SimulationStats& stats) const {
        Calculation_DDR5 calculation(m_memSpec);
        return calculation.calcEnergy(stats);
    }

    interface_energy_info_t DDR5::calcInterfaceEnergyStats(const SimulationStats& stats) const {
        InterfaceCalculation_DDR5 calculation(m_memSpec);
        return calculation.calculateEnergy(stats);
    }

// Stats
    SimulationStats DDR5::getWindowStats(timestamp_t timestamp) {
        SimulationStats stats;
        m_core.getWindowStats(timestamp, stats);
        m_interface.getWindowStats(timestamp, stats);
        return stats;
    }

// Serialization
    void DDR5::serialize_impl(std::ostream &stream) const {
        m_core.serialize(stream);
        m_interface.serialize(stream);
    }

    void DDR5::deserialize_impl(std::istream &stream) {
        m_core.deserialize(stream);
        m_interface.deserialize(stream);
    }

} // namespace DRAMPower
