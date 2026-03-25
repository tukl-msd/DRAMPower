#include "LPDDR5.h"
#include "DRAMPower/Types.h"
#include "DRAMPower/data/stats.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/lpddr5/core_calculation_LPDDR5.h>
#include <DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.h>
#include <DRAMPower/util/extensions.h>
#include <DRAMPower/Exceptions.h>

#include <iostream>


namespace DRAMPower {

    LPDDR5::LPDDR5(const MemSpecLPDDR5 &memSpec, const config::SimConfig& simConfig)
        : m_memSpec(memSpec)
        , m_interface(m_memSpec, simConfig)
        , m_core(m_memSpec)
    {
        registerExtensions();
    }

// Extensions
    void LPDDR5::registerExtensions() {
        getExtensionManager().registerExtension<extensions::DBI>([this](const timestamp_t, const bool enable){
            // Assumption: the enabling of the DBI does not interleave with previous data on the bus
            m_interface.enableDBI(enable);
            return true;
        }, false);
    }

// Getters for CLI
    util::CLIArchitectureConfig LPDDR5::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec.numberOfDevices,
            m_memSpec.numberOfRanks,
            m_memSpec.numberOfBanks
        };
    }

// Calculation
    energy_t LPDDR5::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_LPDDR5 calculation(m_memSpec);
        return calculation.calcEnergy(getWindowStats(timestamp));
    }

    interface_energy_info_t LPDDR5::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_LPDDR5 calculation(m_memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

// Stats
    SimulationStats LPDDR5::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        SimulationStats stats;
        m_core.getWindowStats(timestamp, stats);
        m_interface.getWindowStats(timestamp, stats);
        return stats;
    }

// Serialization
    void LPDDR5::serialize_impl(std::ostream& stream) const {
        m_core.serialize(stream);
        m_interface.serialize(stream);
    }

    void LPDDR5::deserialize_impl(std::istream& stream) {
        m_core.deserialize(stream);
        m_interface.deserialize(stream);
    }


} // namespace DRAMPower
