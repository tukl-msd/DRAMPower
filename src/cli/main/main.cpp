
#include <stdint.h>
#include <unordered_map>
#include <array>
#include <utility>
#include <iostream>
#include <variant>
#include <optional>
#include <filesystem>
#include <string_view>
#include <type_traits>
#include <exception>


#include <DRAMPower/cli/run.hpp>
#include <DRAMPower/cli/config.h>

#include <CLI/CLI.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include "validators.h"


namespace cli11 = ::CLI; 
using namespace DRAMPower;

int parseArgs(int argc, char *argv[], std::string &configfile, std::string &tracefile, std::string &memspec, std::optional<std::string> &jsonfile)
{
	// Application description
	cli11::App app{"DRAMPower v" DRAMPOWER_VERSION_STRING};
	argv = app.ensure_utf8(argv);
	
	// Configfile
	app.add_option("-c,--config", configfile, "config")
		->required(true)
		->check(cli11::ExistingFile);
	// Tracefile
	app.add_option("-t,--trace", tracefile, "csv trace file")
		->required(true)
		->check(cli11::ExistingFile);
	// Memspec
	app.add_option("-m,--memspec", memspec, "json memspec file")
		->required(true)
		->check(cli11::ExistingFile);
	// JSON output file
	app.add_option("-j,--json", jsonfile, "json output file path")
		->required(false)
		->check(validators::EnsureFileExists);
	// Parse arguments
	try { 
		app.parse(argc, argv); 
	} catch(const cli11::ParseError &e) {
		return app.exit(e);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	// Options
	std::string configfile;
	std::string tracefile;
	std::string memspec;
	std::optional<std::string> jsonfile = std::nullopt;
	int res = parseArgs(argc, argv, configfile, tracefile, memspec, jsonfile);
	if(res != 0)
	{
		return res;
	}

	// Set spdlog pattern
	spdlog::set_pattern("%v");

	// Read config
	DRAMPower::DRAMPowerCLI::config::CLIConfig config;
	if (!DRAMPower::DRAMPowerCLI::getConfig(configfile, config)) {
		spdlog::info("Invalid config file");
		return 1;
	}

	// Parse command list (load command list in memory)
	std::vector<std::pair<Command, std::unique_ptr<uint8_t[]>>> commandList;
	if(!DRAMPower::DRAMPowerCLI::parse_command_list(tracefile, commandList))
	{
		spdlog::error("Error while parsing command list. Exiting application");
		return 1;
	}

	// Initialize memory / Create memory object
	std::unique_ptr<dram_base<CmdType>> ddr = DRAMPower::DRAMPowerCLI::getMemory(std::string_view(memspec), config.toggleRateConfig);
	if (!ddr) {
		spdlog::error("Invalid memory specification");
		return 1;
	}

	// Execute commands
	if(!DRAMPower::DRAMPowerCLI::runCommands(ddr, commandList))
	{
		spdlog::error("Error while running commands. Exiting application");
		return 1;
	}

	// Calculate energy and stats
	if(!DRAMPower::DRAMPowerCLI::makeResult(jsonfile, std::move(ddr)))
	{
		spdlog::error("Error while creating result. Exiting application");
		return 1;
	}
	return 0;
};
