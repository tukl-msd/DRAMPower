#include "LPDDR6.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/lpddr6/core_calculation_LPDDR6.h>
#include <DRAMPower/standards/lpddr6/interface_calculation_LPDDR6.h>
#include <DRAMPower/util/extensions.h>

#include <iostream>


namespace DRAMPower {

    using namespace DRAMUtils::Config;

    LPDDR6::LPDDR6(const MemSpecLPDDR6 &memSpec)
        : m_memSpec(memSpec)
        , m_interface(m_memSpec, getImplicitCommandHandler().createInserter())
        , m_core(m_memSpec, getImplicitCommandHandler().createInserter())
    {
        registerCommands();
        registerExtensions();
    }

// Extensions
    void LPDDR6::registerExtensions() {
        getExtensionManager().registerExtension<extensions::DBI>([this](const timestamp_t, const bool enable){
            // Assumption: the enabling of the DBI does not interleave with previous data on the bus
            m_interface.m_dbi.enable(enable);
            return true;
        }, false);
    }

// Commands
    void LPDDR6::registerCommands() {
        auto interfaceregistrar = m_interface.getRegisterHelper();
        auto coreregistrar = m_core.getRegisterHelper();
        // ACT
        routeCoreCommand<CmdType::ACT>(coreregistrar.registerBankHandler(&LPDDR6Core::handleAct));
        routeInterfaceCommand<CmdType::ACT>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // PRE
        routeCoreCommand<CmdType::PRE>(coreregistrar.registerBankHandler(&LPDDR6Core::handlePre));
        routeInterfaceCommand<CmdType::PRE>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // PREA
        routeCoreCommand<CmdType::PREA>(coreregistrar.registerRankHandler(&LPDDR6Core::handlePreAll));
        routeInterfaceCommand<CmdType::PREA>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // REFB
        routeCoreCommand<CmdType::REFB>(coreregistrar.registerBankHandler(&LPDDR6Core::handleRefPerBank));
        routeInterfaceCommand<CmdType::REFB>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // RD
        routeCoreCommand<CmdType::RD>(coreregistrar.registerBankHandler(&LPDDR6Core::handleRead));
        routeInterfaceCommand<CmdType::RD>([this](const Command &cmd) { m_interface.handleData(cmd, true); });
        // RDA
        routeCoreCommand<CmdType::RDA>(coreregistrar.registerBankHandler(&LPDDR6Core::handleReadAuto));
        routeInterfaceCommand<CmdType::RDA>([this](const Command &cmd) { m_interface.handleData(cmd, true); });
        // WR
        routeCoreCommand<CmdType::WR>(coreregistrar.registerBankHandler(&LPDDR6Core::handleWrite));
        routeInterfaceCommand<CmdType::WR>([this](const Command &cmd) { m_interface.handleData(cmd, false); });
        // WRA
        routeCoreCommand<CmdType::WRA>(coreregistrar.registerBankHandler(&LPDDR6Core::handleWriteAuto));
        routeInterfaceCommand<CmdType::WRA>([this](const Command &cmd) { m_interface.handleData(cmd, false); });
        // REFP2B
        if (m_memSpec.bank_arch == MemSpecLPDDR6::MBG || m_memSpec.bank_arch == MemSpecLPDDR6::M16B) {
            routeCoreCommand<CmdType::REFP2B>(coreregistrar.registerBankGroupHandler(&LPDDR6Core::handleRefPerTwoBanks));
            routeInterfaceCommand<CmdType::REFP2B>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        } else {
            // If the command is executed for an invalid bank architecture, throw an exception
            routeCoreCommand<CmdType::REFP2B>([](const Command &) {
                throw Exception(std::string("REFP2B command is not supported for this bank architecture: ") + CmdTypeUtil::to_string(CmdType::REFP2B));
            });
            routeInterfaceCommand<CmdType::REFP2B>([](const Command &) {
                throw Exception(std::string("REFP2B command is not supported for this bank architecture: ") + CmdTypeUtil::to_string(CmdType::REFP2B));
            });
        }
        // REFA
        routeCoreCommand<CmdType::REFA>(coreregistrar.registerRankHandler(&LPDDR6Core::handleRefAll));
        routeInterfaceCommand<CmdType::REFA>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // SREFEN
        routeCoreCommand<CmdType::SREFEN>(coreregistrar.registerRankHandler(&LPDDR6Core::handleSelfRefreshEntry));
        routeInterfaceCommand<CmdType::SREFEN>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // SREFEX
        routeCoreCommand<CmdType::SREFEX>(coreregistrar.registerRankHandler(&LPDDR6Core::handleSelfRefreshExit));
        routeInterfaceCommand<CmdType::SREFEX>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // PDEA
        routeCoreCommand<CmdType::PDEA>(coreregistrar.registerRankHandler(&LPDDR6Core::handlePowerDownActEntry));
        routeInterfaceCommand<CmdType::PDEA>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // PDEP
        routeCoreCommand<CmdType::PDEP>(coreregistrar.registerRankHandler(&LPDDR6Core::handlePowerDownPreEntry));
        routeInterfaceCommand<CmdType::PDEP>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // PDXA
        routeCoreCommand<CmdType::PDXA>(coreregistrar.registerRankHandler(&LPDDR6Core::handlePowerDownActExit));
        routeInterfaceCommand<CmdType::PDXA>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // PDXP
        routeCoreCommand<CmdType::PDXP>(coreregistrar.registerRankHandler(&LPDDR6Core::handlePowerDownPreExit));
        routeInterfaceCommand<CmdType::PDXP>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // DSMEN
        routeCoreCommand<CmdType::DSMEN>(coreregistrar.registerRankHandler(&LPDDR6Core::handleDSMEntry));
        routeInterfaceCommand<CmdType::DSMEN>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // DSMEX
        routeCoreCommand<CmdType::DSMEX>(coreregistrar.registerRankHandler(&LPDDR6Core::handleDSMExit));
        routeInterfaceCommand<CmdType::DSMEX>(interfaceregistrar.registerHandler(&LPDDR6Interface::handleCommandBus));
        // EOS
        getCommandCoreRouter().routeCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
        getCommandInterfaceRouter().routeCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) {
            this->endOfSimulation(cmd.timestamp);
            m_interface.endOfSimulation(cmd.timestamp);
        });
        
    }

    void LPDDR6::endOfSimulation(timestamp_t) {
        if (this->implicitCommandCount() > 0) {
            std::cout << ("[WARN] End of simulation but still implicit commands left!") << std::endl;
        }
    }

// Getters for CLI
    util::CLIArchitectureConfig LPDDR6::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec.numberOfDevices,
            m_memSpec.numberOfRanks,
            m_memSpec.numberOfBanks
        };
    }

// Update toggling rate
    timestamp_t LPDDR6::update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleratedefinition)
    {
        return m_interface.updateTogglingRate(timestamp, toggleratedefinition);
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
        processImplicitCommandQueue(timestamp);
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
