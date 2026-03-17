#include "DRAMPower/standards/ddr5/DDR5.h"

#include <iostream>

#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/standards/ddr5/DDR5Interface.h"
#include "DRAMPower/standards/ddr5/core_calculation_DDR5.h"
#include "DRAMPower/standards/ddr5/interface_calculation_DDR5.h"
#include <DRAMPower/util/extensions.h>

namespace DRAMPower {

    using namespace DRAMUtils::Config;

    DDR5::DDR5(const MemSpecDDR5 &memSpec, const DRAMUtils::Config::ToggleRateDefinition& toggleRate)
        : m_memSpec(memSpec)
        , m_interface(m_memSpec, toggleRate)
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
    energy_t DDR5::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_DDR5 calculation(m_memSpec);
        return calculation.calcEnergy(getWindowStats(timestamp));
    }

    interface_energy_info_t DDR5::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_DDR5 calculation(m_memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

// Stats
    SimulationStats DDR5::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
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
