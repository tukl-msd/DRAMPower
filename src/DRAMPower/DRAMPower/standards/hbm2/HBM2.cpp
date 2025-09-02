#include "HBM2.h"
#include "DRAMPower/Types.h"
#include "DRAMPower/memspec/MemSpecHBM2.h"
#include "DRAMPower/standards/hbm2/HBM2Interface.h"
#include "DRAMPower/util/cli_architecture_config.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/hbm2/core_calculation_HBM2.h>
#include <DRAMPower/standards/hbm2/interface_calculation_HBM2.h>
#include <DRAMPower/util/extensions.h>
#include <optional>

namespace DRAMPower {

    using namespace DRAMUtils::Config;

    HBM2::HBM2(const MemSpecHBM2 &memSpec)
        : dram_base<CmdType>()
        , m_memSpec(memSpec)
        , m_interface(m_memSpec, getImplicitCommandHandler().createInserter())
        , m_core(m_memSpec, getImplicitCommandHandler().createInserter())
    {
        this->registerCommands();
        this->registerExtensions();
    }

// Extensions
    void HBM2::registerExtensions() {
        using namespace pattern_descriptor;
        // DRAMPowerExtensionDBI
        getExtensionManager().registerExtension<extensions::DBI>([this](const timestamp_t, const bool enable) {
            m_interface.m_dbi.enable(enable);
            return true;
        }, false);
    }

// Commands
    void HBM2::registerCommands(){
        auto interfaceregistrar = m_interface.getRegisterHelper();
        auto coreregistrar = m_core.getRegisterHelper();
        // Row commands
        // ACT
        routeCoreCommand<CmdType::ACT>(coreregistrar.registerBankHandler(&HBM2Core::handleAct));
        routeInterfaceCommand<CmdType::ACT>(interfaceregistrar.registerHandler(&HBM2Interface::handleRowCommandBus));
        // PRE
        routeCoreCommand<CmdType::PRE>(coreregistrar.registerBankHandler(&HBM2Core::handlePre));
        routeInterfaceCommand<CmdType::PRE>(interfaceregistrar.registerHandler(&HBM2Interface::handleRowCommandBus));
        // PREA
        routeCoreCommand<CmdType::PREA>(coreregistrar.registerRankHandler(&HBM2Core::handlePreAll));
        routeInterfaceCommand<CmdType::PREA>(interfaceregistrar.registerHandler(&HBM2Interface::handleRowCommandBus));
        // REFSB
        routeCoreCommand<CmdType::REFSB>(coreregistrar.registerBankHandler(&HBM2Core::handleRefSingleBank));
        routeInterfaceCommand<CmdType::REFSB>(interfaceregistrar.registerHandler(&HBM2Interface::handleRowCommandBus));
        // REFA
        routeCoreCommand<CmdType::REFA>(coreregistrar.registerRankHandler(&HBM2Core::handleRefAll));
        routeInterfaceCommand<CmdType::REFA>(interfaceregistrar.registerHandler(&HBM2Interface::handleRowCommandBus));
        // PDEA
        routeCoreCommand<CmdType::PDEA>(coreregistrar.registerRankHandler(&HBM2Core::handlePowerDownActEntry));
        routeInterfaceCommand<CmdType::PDEA>(interfaceregistrar.registerHandler(&HBM2Interface::handleRowCommandBus));
        // PDEP
        routeCoreCommand<CmdType::PDEP>(coreregistrar.registerRankHandler(&HBM2Core::handlePowerDownPreEntry));
        routeInterfaceCommand<CmdType::PDEP>(interfaceregistrar.registerHandler(&HBM2Interface::handleRowCommandBus));
        // SREFEN
        routeCoreCommand<CmdType::SREFEN>(coreregistrar.registerRankHandler(&HBM2Core::handleSelfRefreshEntry));
        routeInterfaceCommand<CmdType::SREFEN>(interfaceregistrar.registerHandler(&HBM2Interface::handleRowCommandBus));
        // SREFEX
        routeCoreCommand<CmdType::SREFEX>(coreregistrar.registerRankHandler(&HBM2Core::handleSelfRefreshExit));
        routeInterfaceCommand<CmdType::SREFEX>(interfaceregistrar.registerHandler(&HBM2Interface::handleRowCommandBus));
        // PDXA
        routeCoreCommand<CmdType::PDXA>(coreregistrar.registerRankHandler(&HBM2Core::handlePowerDownActExit));
        routeInterfaceCommand<CmdType::PDXA>(interfaceregistrar.registerHandler(&HBM2Interface::handleRowCommandBus));
        // PDXP
        routeCoreCommand<CmdType::PDXP>(coreregistrar.registerRankHandler(&HBM2Core::handlePowerDownPreExit));
        routeInterfaceCommand<CmdType::PDXP>(interfaceregistrar.registerHandler(&HBM2Interface::handleRowCommandBus));

        // Column commands
        // RD
        routeCoreCommand<CmdType::RD>(coreregistrar.registerBankHandler(&HBM2Core::handleRead));
        routeInterfaceCommand<CmdType::RD>([this](const Command &command){m_interface.handleData(command, true);});
        // RDA
        routeCoreCommand<CmdType::RDA>(coreregistrar.registerBankHandler(&HBM2Core::handleReadAuto));
        routeInterfaceCommand<CmdType::RDA>([this](const Command &command){m_interface.handleData(command, true);});
        // WR
        routeCoreCommand<CmdType::WR>(coreregistrar.registerBankHandler(&HBM2Core::handleWrite));
        routeInterfaceCommand<CmdType::WR>([this](const Command &command){m_interface.handleData(command, false);});
        // WRA
        routeCoreCommand<CmdType::WRA>(coreregistrar.registerBankHandler(&HBM2Core::handleWriteAuto));
        routeInterfaceCommand<CmdType::WRA>([this](const Command &command){m_interface.handleData(command, false);});

        // EOS
        routeCoreCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
        routeInterfaceCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
    }

    void HBM2::endOfSimulation(timestamp_t) {
        assert(this->implicitCommandCount() == 0);
    }

// Getters for CLI
    util::CLIArchitectureConfig HBM2::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec.numberOfDevices,
            m_memSpec.numberOfStacks, // TODO
            m_memSpec.numberOfBanks
        };
    }

    timestamp_t HBM2::update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) {
        return m_interface.updateTogglingRate(timestamp, toggleRateDefinition);
    }

// Calculation
    energy_t HBM2::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_HBM2 calculation(m_memSpec);
        return calculation.calcEnergy(getWindowStats(timestamp));
    }

    interface_energy_info_t HBM2::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_HBM2 calculation(m_memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

// Stats
    SimulationStats HBM2::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        processImplicitCommandQueue(timestamp);
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
