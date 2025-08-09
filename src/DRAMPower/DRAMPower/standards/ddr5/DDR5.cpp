#include "DRAMPower/standards/ddr5/DDR5.h"

#include <iostream>

#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/standards/ddr5/DDR5Interface.h"
#include "DRAMPower/standards/ddr5/core_calculation_DDR5.h"
#include "DRAMPower/standards/ddr5/interface_calculation_DDR5.h"
#include <DRAMPower/util/extensions.h>

namespace DRAMPower {

    using namespace DRAMUtils::Config;

    DDR5::DDR5(const MemSpecDDR5 &memSpec)
        : m_memSpec(memSpec)
        , m_interface(m_memSpec, getImplicitCommandHandler().createInserter())
        , m_core(m_memSpec, getImplicitCommandHandler().createInserter())
    {
        registerCommands();
        registerExtensions();
    }

// Extensions
    void DDR5::registerExtensions() {
    }

// Commands
    void DDR5::registerCommands() {
        auto interfaceregistrar = m_interface.getRegisterHelper();
        auto coreregistrar = m_core.getRegisterHelper();
        // ACT
        routeCoreCommand<CmdType::ACT>(coreregistrar.registerBankHandler(&DDR5Core::handleAct));
        routeInterfaceCommand<CmdType::ACT>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // PRE
        routeCoreCommand<CmdType::PRE>(coreregistrar.registerBankHandler(&DDR5Core::handlePre));
        routeInterfaceCommand<CmdType::PRE>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // RD
        routeCoreCommand<CmdType::RD>(coreregistrar.registerBankHandler(&DDR5Core::handleRead));
        routeInterfaceCommand<CmdType::RD>([this](const Command &command){m_interface.handleData(command, true);});
        // RDA
        routeCoreCommand<CmdType::RDA>(coreregistrar.registerBankHandler(&DDR5Core::handleReadAuto));
        routeInterfaceCommand<CmdType::RDA>([this](const Command &command){m_interface.handleData(command, true);});
        // WR
        routeCoreCommand<CmdType::WR>(coreregistrar.registerBankHandler(&DDR5Core::handleWrite));
        routeInterfaceCommand<CmdType::WR>([this](const Command &command){m_interface.handleData(command, false);});
        // WRA
        routeCoreCommand<CmdType::WRA>(coreregistrar.registerBankHandler(&DDR5Core::handleWriteAuto));
        routeInterfaceCommand<CmdType::WRA>([this](const Command &command){m_interface.handleData(command, false);});
        // PRESB
        routeCoreCommand<CmdType::PRESB>(coreregistrar.registerBankGroupHandler(&DDR5Core::handlePreSameBank));
        routeInterfaceCommand<CmdType::PRESB>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // REFSB
        routeCoreCommand<CmdType::REFSB>(coreregistrar.registerBankGroupHandler(&DDR5Core::handleRefSameBank));
        routeInterfaceCommand<CmdType::REFSB>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // REFA
        routeCoreCommand<CmdType::REFA>(coreregistrar.registerRankHandler(&DDR5Core::handleRefAll));
        routeInterfaceCommand<CmdType::REFA>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // PREA
        routeCoreCommand<CmdType::PREA>(coreregistrar.registerRankHandler(&DDR5Core::handlePreAll));
        routeInterfaceCommand<CmdType::PREA>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // SREFEN
        routeCoreCommand<CmdType::SREFEN>(coreregistrar.registerRankHandler(&DDR5Core::handleSelfRefreshEntry));
        routeInterfaceCommand<CmdType::SREFEN>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // SREFEX
        routeCoreCommand<CmdType::SREFEX>(coreregistrar.registerRankHandler(&DDR5Core::handleSelfRefreshExit));
        // PDEA
        routeCoreCommand<CmdType::PDEA>(coreregistrar.registerRankHandler(&DDR5Core::handlePowerDownActEntry));
        routeInterfaceCommand<CmdType::PDEA>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // PDEP
        routeCoreCommand<CmdType::PDEP>(coreregistrar.registerRankHandler(&DDR5Core::handlePowerDownPreEntry));
        routeInterfaceCommand<CmdType::PDEP>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // PDXA
        routeCoreCommand<CmdType::PDXA>(coreregistrar.registerRankHandler(&DDR5Core::handlePowerDownActExit));
        routeInterfaceCommand<CmdType::PDXA>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // PDXP
        routeCoreCommand<CmdType::PDXP>(coreregistrar.registerRankHandler(&DDR5Core::handlePowerDownPreExit));
        routeInterfaceCommand<CmdType::PDXP>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // NOP
        routeInterfaceCommand<CmdType::NOP>(interfaceregistrar.registerHandler(&DDR5Interface::handleCommandBus));
        // EOS
        routeCoreCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
        routeInterfaceCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
        // ---------------------------------:

        // Power-down mode is different in DDR5. There is no distinct PDEA and PDEP but instead it depends on
        // bank state upon command issue. Check standard
    }

    void DDR5::endOfSimulation(timestamp_t) {
        assert(this->implicitCommandCount() == 0);
        if (this->implicitCommandCount() > 0) {
            std::cout << ("[WARN] End of simulation but still implicit commands left!") << std::endl;
        }
    }

// Getters for CLI
    util::CLIArchitectureConfig DDR5::getCLIArchitectureConfig() {
        return util::CLIArchitectureConfig{
            m_memSpec.numberOfDevices,
            m_memSpec.numberOfRanks,
            m_memSpec.numberOfBanks
        };
    }

    timestamp_t DDR5::update_toggling_rate(timestamp_t timestamp, const std::optional<DRAMUtils::Config::ToggleRateDefinition> &toggleRateDefinition) {
        return m_interface.updateTogglingRate(timestamp, toggleRateDefinition);
    }

// Calculation
    energy_t DDR5::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_DDR5 calculation(m_memSpec);
        return calculation.calcEnergy(getWindowStats(timestamp));
    }

    interface_energy_info_t DDR5::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_DDR5 calculation(m_memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

// Stats
    SimulationStats DDR5::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        processImplicitCommandQueue(timestamp);
        SimulationStats stats;
        m_core.getWindowStats(timestamp, stats);
        m_interface.getWindowStats(timestamp, stats);
        return stats;
    }

// Serialization
    void DDR5::serialize_impl(std::ostream &stream) const {
        m_core.serialize(stream);
        m_interface.serialize(stream);
    }

    void DDR5::deserialize_impl(std::istream &stream) {
        m_core.deserialize(stream);
        m_interface.deserialize(stream);
    }

} // namespace DRAMPower
