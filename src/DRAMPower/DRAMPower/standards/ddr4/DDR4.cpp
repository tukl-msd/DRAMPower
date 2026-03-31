#include "DDR4.h"
#include "DRAMPower/Types.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecDDR4.h"
#include "DRAMPower/standards/ddr4/DDR4Interface.h"
#include "DRAMPower/util/cli_architecture_config.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/util/extensions.h>

namespace DRAMPower {

    using namespace DRAMUtils::Config;

    DDR4::DDR4(const MemSpecDDR4 &memSpec, const config::SimConfig &simConfig)
        : m_memSpec(memSpec)
        , m_interface(m_memSpec, simConfig)
        , m_core(m_memSpec)
        , m_calc_interface(m_memSpec)
        , m_calc_core(m_memSpec)
    {
        this->registerExtensions();
    }

// Extensions
    void DDR4::registerExtensions() {
        getExtensionManager().registerExtension<extensions::DBI>([this](const timestamp_t, const bool enable) {
            // Assumption: the enabling of the DBI does not interleave with previous data on the bus
            // x4,x8,x16 devices: only x8 and x16 support dbi
            if (4 == m_memSpec.bitWidth) {
                std::cout << "[WARN] x4 devices don't support DBI" << std::endl;
                return false;
            }
            m_interface.enableDBI(enable);
            return true;
        }, false);
    }

// Getters for CLI
    util::CLIArchitectureConfig DDR4::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec.numberOfDevices,
            m_memSpec.numberOfRanks,
            m_memSpec.numberOfBanks
        };
    }

// Calculation
    energy_t DDR4::calcCoreEnergyStats(const SimulationStats& stats) const {
        return m_calc_core.calcEnergy(stats);
    }

    interface_energy_info_t DDR4::calcInterfaceEnergyStats(const SimulationStats& stats) const {
        return m_calc_interface.calculateEnergy(stats);
    }

// Stats
    SimulationStats DDR4::getWindowStats(timestamp_t timestamp) {
        SimulationStats stats;
        m_core.getWindowStats(timestamp, stats);
        m_interface.getWindowStats(timestamp, stats);
        return stats;
    }

// Serialization
    void DDR4::serialize_impl(std::ostream& stream) const {
        m_core.serialize(stream);
        m_interface.serialize(stream);
    }

    void DDR4::deserialize_impl(std::istream& stream) {
        m_core.deserialize(stream);
        m_interface.deserialize(stream);
    }

} // namespace DRAMPower
