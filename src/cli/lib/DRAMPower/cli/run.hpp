#ifndef LIB_DRAMPOWERCLI_RUN_H
#define LIB_DRAMPOWERCLI_RUN_H

#include <optional>
#include <vector>
#include <memory>
#include <string>

#include <DRAMUtils/config/toggling_rate.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/command/CmdType.h>

#include "config.h"

namespace DRAMPower::CLI {

std::unique_ptr<dram_base<CmdType>> getMemory(const std::string_view &data, std::optional<DRAMUtils::Config::ToggleRateDefinition> togglingRate = std::nullopt);
bool parse_command_list(std::string_view csv_file, std::vector<std::pair<Command, std::unique_ptr<uint8_t[]>>> &commandList);
bool makeResult(std::optional<std::string> jsonfile, const std::unique_ptr<dram_base<CmdType>> &ddr);
bool jsonFileResult(const std::string &jsonfile, const std::unique_ptr<dram_base<CmdType>> &ddr, const energy_t &core_energy, const interface_energy_info_t &interface_energy);
bool stdoutResult(const std::unique_ptr<dram_base<CmdType>> &ddr, const energy_t &core_energy, const interface_energy_info_t &interface_energy);
bool getConfig(const std::string &configfile, config::CLIConfig &config);
bool runCommands(std::unique_ptr<dram_base<CmdType>> &ddr, const std::vector<std::pair<Command, std::unique_ptr<uint8_t[]>>> &commandList);


} // namespace DRAMPower::CLI

#endif /* LIB_DRAMPOWERCLI_RUN_H */
