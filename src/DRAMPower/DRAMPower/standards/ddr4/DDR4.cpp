#include "DDR4.h"
#include "DRAMPower/Types.h"
#include "DRAMPower/util/cli_architecture_config.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/ddr4/core_calculation_DDR4.h>
#include <DRAMPower/standards/ddr4/interface_calculation_DDR4.h>
#include <DRAMPower/util/extensions.h>
#include <optional>

namespace DRAMPower {

    using namespace DRAMUtils::Config;

    DDR4::DDR4(const MemSpecDDR4 &memSpec)
        : dram_base<CmdType>({
            {pattern_descriptor::V, PatternEncoderBitSpec::H},
            {pattern_descriptor::X, PatternEncoderBitSpec::H},
          }, DDR4Interface::cmdBusInitPattern)
        , m_memSpec(memSpec)
        , m_interface(m_memSpec, getImplicitCommandHandler().createInserter(), getPatternHandler())
        , m_core(m_memSpec, getImplicitCommandHandler().createInserter())
    {
        this->registerCommands();
        this->registerExtensions();
    }

    std::optional<const uint8_t *> DDR4Interface::handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read) {
        if (0 == n_bits || !data || !m_dbi.isEnabled()) {
            // No DBI or no data to process
            return std::nullopt;
        }
        timestamp_t virtual_time = timestamp * m_memSpec.dataRate;
        // updateDBI calls the given callback to handle pin changes
        auto dbiResult = m_dbi.updateDBI(virtual_time, n_bits, data, read);
        if (!dbiResult) {
            // No data to return
            return std::nullopt;
        }
        // Return the inverted data
        return dbiResult;
    }

    void DDR4::registerExtensions() {
        using namespace pattern_descriptor;
        // DRAMPowerExtensionDBI
        getExtensionManager().registerExtension<extensions::DBI>([this]([[maybe_unused]] const timestamp_t timestamp, const bool enable) {
            // NOTE: Assumption: the enabling of the DBI does not interleave with previous data on the bus
            m_interface.m_dbi.enable(enable);
        }, false);
    }

    void DDR4::registerCommands(){
        auto interfaceregistrar = m_interface.getRegisterHelper();
        auto coreregistrar = m_core.getRegisterHelper();
        // ACT
        routeCoreCommand<CmdType::ACT>(coreregistrar.registerBankHandler(&DDR4Core::handleAct));
        routeInterfaceCommand<CmdType::ACT>(interfaceregistrar.registerHandler(&DDR4Interface::handleCommandBus));
        // PRE
        routeCoreCommand<CmdType::PRE>(coreregistrar.registerBankHandler(&DDR4Core::handlePre));
        routeInterfaceCommand<CmdType::PRE>(interfaceregistrar.registerHandler(&DDR4Interface::handleCommandBus));
        // PREA
        routeCoreCommand<CmdType::PREA>(coreregistrar.registerRankHandler(&DDR4Core::handlePreAll));
        routeInterfaceCommand<CmdType::PREA>(interfaceregistrar.registerHandler(&DDR4Interface::handleCommandBus));
        // REFA
        routeCoreCommand<CmdType::REFA>(coreregistrar.registerRankHandler(&DDR4Core::handleRefAll));
        routeInterfaceCommand<CmdType::REFA>(interfaceregistrar.registerHandler(&DDR4Interface::handleCommandBus));
        // RD
        routeCoreCommand<CmdType::RD>(coreregistrar.registerBankHandler(&DDR4Core::handleRead));
        routeInterfaceCommand<CmdType::RD>([this](const Command &command){m_interface.handleData(command, true);});
        // RDA
        routeCoreCommand<CmdType::RDA>(coreregistrar.registerBankHandler(&DDR4Core::handleReadAuto));
        routeInterfaceCommand<CmdType::RDA>([this](const Command &command){m_interface.handleData(command, true);});
        // WR
        routeCoreCommand<CmdType::WR>(coreregistrar.registerBankHandler(&DDR4Core::handleWrite));
        routeInterfaceCommand<CmdType::WR>([this](const Command &command){m_interface.handleData(command, false);});
        // WRA
        routeCoreCommand<CmdType::WRA>(coreregistrar.registerBankHandler(&DDR4Core::handleWriteAuto));
        routeInterfaceCommand<CmdType::WRA>([this](const Command &command){m_interface.handleData(command, false);});
        // SREFEN
        routeCoreCommand<CmdType::SREFEN>(coreregistrar.registerRankHandler(&DDR4Core::handleSelfRefreshEntry));
        routeInterfaceCommand<CmdType::SREFEN>(interfaceregistrar.registerHandler(&DDR4Interface::handleCommandBus));
        // SREFEX
        routeCoreCommand<CmdType::SREFEX>(coreregistrar.registerRankHandler(&DDR4Core::handleSelfRefreshExit));
        routeInterfaceCommand<CmdType::SREFEX>(interfaceregistrar.registerHandler(&DDR4Interface::handleCommandBus));
        // PDEA
        routeCoreCommand<CmdType::PDEA>(coreregistrar.registerRankHandler(&DDR4Core::handlePowerDownActEntry));
        routeInterfaceCommand<CmdType::PDEA>(interfaceregistrar.registerHandler(&DDR4Interface::handleCommandBus));
        // PDEP
        routeCoreCommand<CmdType::PDEP>(coreregistrar.registerRankHandler(&DDR4Core::handlePowerDownPreEntry));
        routeInterfaceCommand<CmdType::PDEP>(interfaceregistrar.registerHandler(&DDR4Interface::handleCommandBus));
        // PDXA
        routeCoreCommand<CmdType::PDXA>(coreregistrar.registerRankHandler(&DDR4Core::handlePowerDownActExit));
        routeInterfaceCommand<CmdType::PDXA>(interfaceregistrar.registerHandler(&DDR4Interface::handleCommandBus));
        // PDXP
        routeCoreCommand<CmdType::PDXP>(coreregistrar.registerRankHandler(&DDR4Core::handlePowerDownPreExit));
        routeInterfaceCommand<CmdType::PDXP>(interfaceregistrar.registerHandler(&DDR4Interface::handleCommandBus));
        // EOS
        routeCoreCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
        routeInterfaceCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
    }

// Getters for CLI
    util::CLIArchitectureConfig DDR4::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec.numberOfDevices,
            m_memSpec.numberOfRanks,
            m_memSpec.numberOfBanks
        };
    }

    void DDR4::endOfSimulation(timestamp_t) {
        assert(this->implicitCommandCount() == 0);
    }

    timestamp_t DDR4::update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) {
        return m_interface.updateTogglingRate(timestamp, toggleRateDefinition);
    }

// Calculation
    energy_t DDR4::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_DDR4 calculation(m_memSpec);
        return calculation.calcEnergy(getWindowStats(timestamp));
    }

    interface_energy_info_t DDR4::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_DDR4 calculation(m_memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

// Stats
    SimulationStats DDR4::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        processImplicitCommandQueue(timestamp);
        SimulationStats stats;
        m_core.getWindowStats(timestamp, stats);
        m_interface.getWindowStats(timestamp, stats);
        return stats;
    }

} // namespace DRAMPower
