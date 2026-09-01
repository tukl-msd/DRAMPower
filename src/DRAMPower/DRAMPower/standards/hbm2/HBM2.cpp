#include "HBM2.h"
#include "DRAMPower/Types.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecHBM2.h"
#include "DRAMPower/standards/hbm2/HBM2Interface.h"
#include "DRAMPower/standards/hbm2/core_calculation_HBM2.h"
#include "DRAMPower/standards/hbm2/interface_calculation_HBM2.h"
#include "DRAMPower/util/cli_architecture_config.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/util/extensions.h>

namespace DRAMPower {

    using namespace DRAMUtils::Config;

    HBM2::HBM2(const MemSpecHBM2 &memSpec, const config::SimConfig& simConfig)
        : m_memSpec(memSpec)
        , m_interface(m_memSpec, simConfig)
        , m_core(m_memSpec)
    {
        this->registerExtensions();
    }

// Extensions
    void HBM2::registerExtensions() {
        using namespace pattern_descriptor;
        // DRAMPowerExtensionDBI
        getExtensionManager().registerExtension<extensions::DBI>([this](const timestamp_t, const bool enable) {
            m_interface.enableDBI(enable);
            return true;
        }, false);
    }

// Getters for CLI
    util::CLIArchitectureConfig HBM2::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec.numberOfDevices,
            1,
            m_memSpec.numberOfStacks * m_memSpec.numberOfBanks
        };
    }

// Calculation
    energy_t HBM2::calcCoreEnergyStats(const SimulationStats& stats) const {
        Calculation_HBM2 calculation(m_memSpec);
        return calculation.calcEnergy(stats);
    }

    interface_energy_info_t HBM2::calcInterfaceEnergyStats(const SimulationStats& stats) const {
        InterfaceCalculation_HBM2 calculation(m_memSpec);
        return calculation.calculateEnergy(stats);
    }

// Stats
    SimulationStats HBM2::getWindowStats(timestamp_t timestamp) {
        SimulationStats stats;
        m_core.getWindowStats(timestamp, stats);
        m_interface.getWindowStats(timestamp, stats);
        return stats;
    }

// Serialization
    void HBM2::serialize_impl(std::ostream& stream) const {
        m_core.serialize(stream);
        m_interface.serialize(stream);
    }

    void HBM2::deserialize_impl(std::istream& stream) {
        m_core.deserialize(stream);
        m_interface.deserialize(stream);
    }

} // namespace DRAMPower
