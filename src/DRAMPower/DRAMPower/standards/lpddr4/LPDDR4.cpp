#include "LPDDR4.h"
#include "DRAMPower/command/CmdType.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/lpddr4/core_calculation_LPDDR4.h>
#include <DRAMPower/standards/lpddr4/interface_calculation_LPDDR4.h>
#include <DRAMPower/util/extensions.h>


namespace DRAMPower {

    using namespace DRAMUtils::Config;

    LPDDR4::LPDDR4(const MemSpecLPDDR4 &memSpec)
        : m_memSpec(memSpec)
        , m_interface(m_memSpec, getImplicitCommandHandler().createInserter())
        , m_core(m_memSpec, getImplicitCommandHandler().createInserter())
    {
        registerCommands();
        registerExtensions();
    }

// Extensions
    void LPDDR4::registerExtensions() {
        getExtensionManager().registerExtension<extensions::DBI>([this](const timestamp_t, const bool enable){
            // Assumption: the enabling of the DBI does not interleave with previous data on the bus
            // x8,x16 devices: support dbi
            m_interface.m_dbi.enable(enable);
            return true;
        }, false);
    }

// Commands
    void LPDDR4::registerCommands() {
        auto interfaceregistrar = m_interface.getRegisterHelper();
        auto coreregistrar = m_core.getRegisterHelper();
        // ACT
        routeCoreCommand<CmdType::ACT>(coreregistrar.registerBankHandler(&LPDDR4Core::handleAct));
        routeInterfaceCommand<CmdType::ACT>(interfaceregistrar.registerHandler(&LPDDR4Interface::handleCommandBus));
        // PRE
        routeCoreCommand<CmdType::PRE>(coreregistrar.registerBankHandler(&LPDDR4Core::handlePre));
        routeInterfaceCommand<CmdType::PRE>(interfaceregistrar.registerHandler(&LPDDR4Interface::handleCommandBus));
        // PREA
        routeCoreCommand<CmdType::PREA>(coreregistrar.registerRankHandler(&LPDDR4Core::handlePreAll));
        routeInterfaceCommand<CmdType::PREA>(interfaceregistrar.registerHandler(&LPDDR4Interface::handleCommandBus));
        // REFB
        routeCoreCommand<CmdType::REFB>(coreregistrar.registerBankHandler(&LPDDR4Core::handleRefPerBank));
        routeInterfaceCommand<CmdType::REFB>(interfaceregistrar.registerHandler(&LPDDR4Interface::handleCommandBus));
        // RD
        routeCoreCommand<CmdType::RD>(coreregistrar.registerBankHandler(&LPDDR4Core::handleRead));
        routeInterfaceCommand<CmdType::RD>([this](const Command &cmd) { m_interface.handleData(cmd, true); });
        // RDA
        routeCoreCommand<CmdType::RDA>(coreregistrar.registerBankHandler(&LPDDR4Core::handleReadAuto));
        routeInterfaceCommand<CmdType::RDA>([this](const Command &cmd) { m_interface.handleData(cmd, true); });
        // WR
        routeCoreCommand<CmdType::WR>(coreregistrar.registerBankHandler(&LPDDR4Core::handleWrite));
        routeInterfaceCommand<CmdType::WR>([this](const Command &cmd) { m_interface.handleData(cmd, false); });
        // WRA
        routeCoreCommand<CmdType::WRA>(coreregistrar.registerBankHandler(&LPDDR4Core::handleWriteAuto));
        routeInterfaceCommand<CmdType::WRA>([this](const Command &cmd) { m_interface.handleData(cmd, false); });
        // REFA
        routeCoreCommand<CmdType::REFA>(coreregistrar.registerRankHandler(&LPDDR4Core::handleRefAll));
        routeInterfaceCommand<CmdType::REFA>(interfaceregistrar.registerHandler(&LPDDR4Interface::handleCommandBus));
        // PDEA
        routeCoreCommand<CmdType::PDEA>(coreregistrar.registerRankHandler(&LPDDR4Core::handlePowerDownActEntry));
        // PDXA
        routeCoreCommand<CmdType::PDXA>(coreregistrar.registerRankHandler(&LPDDR4Core::handlePowerDownActExit));
        // PDEP
        routeCoreCommand<CmdType::PDEP>(coreregistrar.registerRankHandler(&LPDDR4Core::handlePowerDownPreEntry));
        // PDXP
        routeCoreCommand<CmdType::PDXP>(coreregistrar.registerRankHandler(&LPDDR4Core::handlePowerDownPreExit));
        // SREFEN
        routeCoreCommand<CmdType::SREFEN>(coreregistrar.registerRankHandler(&LPDDR4Core::handleSelfRefreshEntry));
        routeInterfaceCommand<CmdType::SREFEN>(interfaceregistrar.registerHandler(&LPDDR4Interface::handleCommandBus));
        // SREFEX
        routeCoreCommand<CmdType::SREFEX>(coreregistrar.registerRankHandler(&LPDDR4Core::handleSelfRefreshExit));
        routeInterfaceCommand<CmdType::SREFEX>(interfaceregistrar.registerHandler(&LPDDR4Interface::handleCommandBus));
        // EOS
        routeCoreCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
        routeInterfaceCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
    };

// Getters for CLI
    util::CLIArchitectureConfig LPDDR4::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec.numberOfDevices,
            m_memSpec.numberOfRanks,
            m_memSpec.numberOfBanks
        };
    }

// Update toggling rate
    timestamp_t LPDDR4::update_toggling_rate(timestamp_t timestamp, const std::optional<ToggleRateDefinition> &toggleratedefinition)
    {
        return m_interface.updateTogglingRate(timestamp, toggleratedefinition);
    }

// Core
    

    void LPDDR4::endOfSimulation(timestamp_t) {
        if (this->implicitCommandCount() > 0)
			std::cout << ("[WARN] End of simulation but still implicit commands left!") << std::endl;
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
        processImplicitCommandQueue(timestamp);
        SimulationStats stats;
        m_core.getWindowStats(timestamp, stats);
        m_interface.getWindowStats(timestamp, stats);
        return stats;
    }

} // namespace DRAMPower
