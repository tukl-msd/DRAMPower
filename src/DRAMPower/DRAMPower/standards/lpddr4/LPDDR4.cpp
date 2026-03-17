#include "LPDDR4.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/lpddr4/core_calculation_LPDDR4.h>
#include <DRAMPower/standards/lpddr4/interface_calculation_LPDDR4.h>
#include <DRAMPower/util/extensions.h>


namespace DRAMPower {

    using namespace DRAMUtils::Config;

    LPDDR4::LPDDR4(const MemSpecLPDDR4 &memSpec, const DRAMUtils::Config::ToggleRateDefinition& toggleRate)
        : m_memSpec(memSpec)
        , m_interface(m_memSpec, toggleRate)
        , m_core(m_memSpec)
    {
        registerExtensions();
    }

// Extensions
    void LPDDR4::registerExtensions() {
        getExtensionManager().registerExtension<extensions::DBI>([this](const timestamp_t, const bool enable){
            // Assumption: the enabling of the DBI does not interleave with previous data on the bus
            // x8,x16 devices: support dbi
            m_interface.enableDBI(enable);
            return true;
        }, false);
    }

// Getters for CLI
    util::CLIArchitectureConfig LPDDR4::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec.numberOfDevices,
            m_memSpec.numberOfRanks,
            m_memSpec.numberOfBanks
        };
    }

// Calculation
    energy_t LPDDR4::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_LPDDR4 calculation(m_memSpec);
        return calculation.calcEnergy(getWindowStats(timestamp));
    }

    interface_energy_info_t LPDDR4::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_LPDDR4 interface_calc(m_memSpec);
        return interface_calc.calculateEnergy(getWindowStats(timestamp));
    }

// Stats
    SimulationStats LPDDR4::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        SimulationStats stats;
        m_core.getWindowStats(timestamp, stats);
        m_interface.getWindowStats(timestamp, stats);
        return stats;
    }

// Serialization
    void LPDDR4::serialize_impl(std::ostream& stream) const {
        m_core.serialize(stream);
        m_interface.serialize(stream);
    }

    void LPDDR4::deserialize_impl(std::istream& stream) {
        m_core.deserialize(stream);
        m_interface.deserialize(stream);
    }

} // namespace DRAMPower
