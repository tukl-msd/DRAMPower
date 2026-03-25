#include "LPDDR6.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/lpddr6/core_calculation_LPDDR6.h>
#include <DRAMPower/standards/lpddr6/interface_calculation_LPDDR6.h>
#include <DRAMPower/util/extensions.h>

#include <iostream>


namespace DRAMPower {

    using namespace DRAMUtils::Config;

    LPDDR6::LPDDR6(const MemSpecLPDDR6 &memSpec, const config::SimConfig &simConfig)
        : m_memSpec(memSpec)
        , m_interface(m_memSpec, simConfig)
        , m_core(m_memSpec)
    {
        registerExtensions();
    }

// Extensions
    void LPDDR6::registerExtensions() {
        getExtensionManager().registerExtension<extensions::DBI>([this](const timestamp_t, const bool enable) -> bool {
            // Assumption: the enabling of the DBI does not interleave with previous data on the bus
            m_interface.enableDBI(enable);
            return true;
        }, false);
        getExtensionManager().registerExtension<extensions::MetaData>([this](uint16_t metaData) -> void {
            m_interface.setMetaData(metaData);
        },
        [this]() -> uint16_t {
            return m_interface.getMetaData();
        }, m_interface.getMetaData());
    }

// Getters for CLI
    util::CLIArchitectureConfig LPDDR6::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec.numberOfDevices,
            m_memSpec.numberOfRanks,
            m_memSpec.numberOfBanks
        };
    }

// Calculation
    energy_t LPDDR6::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_LPDDR6 calculation(m_memSpec);
        return calculation.calcEnergy(getWindowStats(timestamp));
    }

    interface_energy_info_t LPDDR6::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_LPDDR6 calculation(m_memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

// Stats
    SimulationStats LPDDR6::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        SimulationStats stats;
        m_core.getWindowStats(timestamp, stats);
        m_interface.getWindowStats(timestamp, stats);
        return stats;
    }

// Serialization
    void LPDDR6::serialize_impl(std::ostream& stream) const {
        m_core.serialize(stream);
        m_interface.serialize(stream);
    }

    void LPDDR6::deserialize_impl(std::istream& stream) {
        m_core.deserialize(stream);
        m_interface.deserialize(stream);
    }

} // namespace DRAMPower
