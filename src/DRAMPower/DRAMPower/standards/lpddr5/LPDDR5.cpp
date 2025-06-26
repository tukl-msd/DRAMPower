#include "LPDDR5.h"
#include "DRAMPower/Types.h"
#include "DRAMPower/data/stats.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/lpddr5/core_calculation_LPDDR5.h>
#include <DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.h>
#include <DRAMPower/util/extensions.h>

#include <iostream>


namespace DRAMPower {

    LPDDR5::LPDDR5(const MemSpecLPDDR5 &memSpec)
        : m_memSpec(std::make_shared<MemSpecLPDDR5>(memSpec))
        , m_interface(m_memSpec, getImplicitCommandHandler().createInserter())
        , m_core(m_memSpec, getImplicitCommandHandler().createInserter())
    {
        this->registerCommands();
        this->registerExtensions();
    }

// Extensions
    void LPDDR5::registerExtensions() {

    }

// Commands
    void LPDDR5::registerCommands() {
        auto interfaceregistrar = m_interface.getRegisterHelper();
        auto coreregistrar = m_core.getRegisterHelper();
        // ACT
        routeCoreCommand<CmdType::ACT>(coreregistrar.registerBankHandler(&LPDDR5Core::handleAct));
        routeInterfaceCommand<CmdType::ACT>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // PRE
        routeCoreCommand<CmdType::PRE>(coreregistrar.registerBankHandler(&LPDDR5Core::handlePre));
        routeInterfaceCommand<CmdType::PRE>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // PREA
        routeCoreCommand<CmdType::PREA>(coreregistrar.registerRankHandler(&LPDDR5Core::handlePreAll));
        routeInterfaceCommand<CmdType::PREA>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // REFB
        routeCoreCommand<CmdType::REFB>(coreregistrar.registerBankHandler(&LPDDR5Core::handleRefPerBank));
        routeInterfaceCommand<CmdType::REFB>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // RD
        routeCoreCommand<CmdType::RD>(coreregistrar.registerBankHandler(&LPDDR5Core::handleRead));
        routeInterfaceCommand<CmdType::RD>([this](const Command &cmd) { m_interface.handleData(cmd, true); });
        // RDA
        routeCoreCommand<CmdType::RDA>(coreregistrar.registerBankHandler(&LPDDR5Core::handleReadAuto));
        routeInterfaceCommand<CmdType::RDA>([this](const Command &cmd) { m_interface.handleData(cmd, true); });
        // WR
        routeCoreCommand<CmdType::WR>(coreregistrar.registerBankHandler(&LPDDR5Core::handleWrite));
        routeInterfaceCommand<CmdType::WR>([this](const Command &cmd) { m_interface.handleData(cmd, false); });
        // WRA
        routeCoreCommand<CmdType::WRA>(coreregistrar.registerBankHandler(&LPDDR5Core::handleWriteAuto));
        routeInterfaceCommand<CmdType::WRA>([this](const Command &cmd) { m_interface.handleData(cmd, false); });
        // REFP2B TODO
        routeCoreCommand<CmdType::REFP2B>(coreregistrar.registerBankGroupHandler(&LPDDR5Core::handleRefPerTwoBanks));
        if (m_memSpec->bank_arch == MemSpecLPDDR5::MBG || m_memSpec->bank_arch == MemSpecLPDDR5::M16B) {
            routeInterfaceCommand<CmdType::REFP2B>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        }
        // REFA
        routeCoreCommand<CmdType::REFA>(coreregistrar.registerRankHandler(&LPDDR5Core::handleRefAll));
        routeInterfaceCommand<CmdType::REFA>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // SREFEN
        routeCoreCommand<CmdType::SREFEN>(coreregistrar.registerRankHandler(&LPDDR5Core::handleSelfRefreshEntry));
        routeInterfaceCommand<CmdType::SREFEN>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // SREFEX
        routeCoreCommand<CmdType::SREFEX>(coreregistrar.registerRankHandler(&LPDDR5Core::handleSelfRefreshExit));
        routeInterfaceCommand<CmdType::SREFEX>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // PDEA
        routeCoreCommand<CmdType::PDEA>(coreregistrar.registerRankHandler(&LPDDR5Core::handlePowerDownActEntry));
        routeInterfaceCommand<CmdType::PDEA>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // PDEP
        routeCoreCommand<CmdType::PDEP>(coreregistrar.registerRankHandler(&LPDDR5Core::handlePowerDownPreEntry));
        routeInterfaceCommand<CmdType::PDEP>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // PDXA
        routeCoreCommand<CmdType::PDXA>(coreregistrar.registerRankHandler(&LPDDR5Core::handlePowerDownActExit));
        routeInterfaceCommand<CmdType::PDXA>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // PDXP
        routeCoreCommand<CmdType::PDXP>(coreregistrar.registerRankHandler(&LPDDR5Core::handlePowerDownPreExit));
        routeInterfaceCommand<CmdType::PDXP>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // DSMEN
        routeCoreCommand<CmdType::DSMEN>(coreregistrar.registerRankHandler(&LPDDR5Core::handleDSMEntry));
        routeInterfaceCommand<CmdType::DSMEN>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // DSMEX
        routeCoreCommand<CmdType::DSMEX>(coreregistrar.registerRankHandler(&LPDDR5Core::handleDSMExit));
        routeInterfaceCommand<CmdType::DSMEX>(interfaceregistrar.registerHandler(&LPDDR5Interface::handleCommandBus));
        // EOS
        getCommandCoreRouter().routeCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
        getCommandInterfaceRouter().routeCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
    }

    void LPDDR5::endOfSimulation(timestamp_t) {
        if (this->implicitCommandCount() > 0) {
            std::cout << ("[WARN] End of simulation but still implicit commands left!") << std::endl;
        }
    }

// Getters for CLI
    util::CLIArchitectureConfig LPDDR5::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec->numberOfDevices,
            m_memSpec->numberOfRanks,
            m_memSpec->numberOfBanks
        };
    }

// Update toggling rate
    timestamp_t LPDDR5::update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleratedefinition)
    {
        return m_interface.updateTogglingRate(timestamp, toggleratedefinition);
    }

// Calculation
    energy_t LPDDR5::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_LPDDR5 calculation(*m_memSpec);
        return calculation.calcEnergy(getWindowStats(timestamp));
    }

    interface_energy_info_t LPDDR5::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_LPDDR5 calculation(*m_memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

// Stats
    SimulationStats LPDDR5::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        processImplicitCommandQueue(timestamp);
        SimulationStats stats;
        m_core.getWindowStats(timestamp, stats);
        m_interface.getWindowStats(timestamp, stats);
        return stats;
    }

} // namespace DRAMPower
