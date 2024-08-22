
#include <stdint.h>
#include <unordered_map>
#include <array>
#include <utility>
#include <iostream>
#include <fstream>
#include <variant>
#include <optional>
#include <vector>
#include <filesystem>
#include <string_view>
#include <type_traits>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/standards/ddr4/DDR4.h>
#include <DRAMPower/memspec/MemSpecDDR4.h>
#include <DRAMPower/standards/ddr5/DDR5.h>
#include <DRAMPower/memspec/MemSpecDDR5.h>
#include <DRAMPower/memspec/MemSpecLPDDR4.h>
#include <DRAMPower/memspec/MemSpecLPDDR5.h>
#include <DRAMPower/standards/lpddr4/LPDDR4.h>
#include <DRAMPower/standards/lpddr5/LPDDR5.h>
#include <DRAMUtils/util/json_utils.h>
#include <DRAMUtils/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR4.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR5.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>

#include <cli11/CLI11.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include "csv.hpp"
#include "util.hpp"
#include "config.h"

using namespace DRAMPower;


std::unique_ptr<dram_base<CmdType>> getMemory(const std::string_view &data)
{
	try
	{
		std::unique_ptr<dram_base<CmdType>> result = nullptr;
		// Get memspec
        auto memspec = DRAMUtils::parse_memspec_from_file(std::filesystem::path(data));
        if (!memspec) {
            return result;
		}
		// Get ddr
		std::visit( [&result] (auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecDDR4>)
			{
				MemSpecDDR4 ddr (static_cast<DRAMUtils::MemSpec::MemSpecDDR4>(arg));
				result = std::make_unique<DDR4>(ddr);
			}
			else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecDDR5>)
			{
				MemSpecDDR5 ddr (static_cast<DRAMUtils::MemSpec::MemSpecDDR5>(arg));
				result = std::make_unique<DDR5>(ddr);
			}
			else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecLPDDR4>)
			{
				MemSpecLPDDR4 ddr (static_cast<DRAMUtils::MemSpec::MemSpecLPDDR4>(arg));
				result = std::make_unique<LPDDR4>(ddr);
			}
			else if constexpr (std::is_same_v<T, DRAMUtils::MemSpec::MemSpecLPDDR5>)
			{
				MemSpecLPDDR5 ddr (static_cast<DRAMUtils::MemSpec::MemSpecLPDDR5>(arg));
				result = std::make_unique<LPDDR5>(ddr);
			}
		}, memspec->getVariant());
		return result;
	}
	catch(const std::exception& e)
	{
		spdlog::error(e.what());
		return nullptr;
	}
}

std::vector<std::pair<Command, std::unique_ptr<uint8_t[]>>> parse_command_list(std::string_view csv_file)
{
	std::vector<std::pair<Command, std::unique_ptr<uint8_t[]>>> commandList;

	// Read csv file
	csv::CSVFormat format;
	format.no_header();
	format.trim({ ' ', '\t' });

	csv::CSVReader reader{ csv_file, format };
	
	// loop variables
	uint64_t rowcounter = 0;
	std::size_t rowidx, size, rank_id, bank_group_id, bank_id, row_id, column_id = 0;

	timestamp_t timestamp = 0;
	csv::string_view cmdType;
	std::unordered_map<csv::string_view, DRAMPower::CmdType>::iterator cmdit;
	CmdType cmd;

	// Parse csv file
	// timestamp, command, rank, bank_group, bank, row, column, [data]
	constexpr std::size_t MINCSVSIZE = 7;
	for ( csv::CSVRow& row : reader ) {
		rowidx = 0;

		// Read csv row
		if ( row.size() < MINCSVSIZE )
		{
			std::cerr << "Invalid command structure. Row " << rowcounter << std::endl;
			exit(1);
		}
			
		timestamp = row[rowidx++].get<timestamp_t>();
		cmdType = row[rowidx++].get_sv();
		rank_id = row[rowidx++].get<std::size_t>();
		bank_group_id = row[rowidx++].get<std::size_t>();
		bank_id = row[rowidx++].get<std::size_t>();
		row_id = row[rowidx++].get<std::size_t>();
		column_id = row[rowidx++].get<std::size_t>();

		// Get command
		cmd = DRAMPower::CmdTypeUtil::from_string(cmdType);

		// Get data if needed
		if ( DRAMPower::CmdTypeUtil::needs_data(cmd) ) {
			if ( row.size() < MINCSVSIZE + 1 ) {
				std::cerr << "Invalid command structure. Row " << rowcounter << std::endl;
				exit(1);
			}
			// uint64_t length = 0;
			csv::string_view data = row[rowidx++].get_sv();
			std::unique_ptr<uint8_t[]> arr;
			try
			{
				arr = CLIutil::hexStringToUint8Array(data, size);
			}
			catch (std::exception &e)
			{
				std::cerr << e.what() << std::endl;
				std::cerr << "Invalid data field in row " << rowcounter << std::endl;
				exit(1);
			}
			commandList.emplace_back(Command{ timestamp, cmd, { bank_id, bank_group_id, rank_id, row_id, column_id}, arr.get(), size * 8}, std::move(arr));
		}
		else {
			commandList.emplace_back(Command{ timestamp, cmd, { bank_id, bank_group_id, rank_id, row_id, column_id} }, nullptr);
		}
		// Increment row counter
		++rowcounter;
	}

	return commandList;
};

void jsonFileResult(const std::string &jsonfile, const std::unique_ptr<dram_base<CmdType>> &ddr, const energy_t &core_energy, const interface_energy_info_t &interface_energy)
{
	std::ofstream out;
	out = std::ofstream(jsonfile);
	if ( !out.is_open() ) {
		std::cerr << "Could not open file " << jsonfile << std::endl;
		exit(1);
	}
	json_t j;
	size_t energy_offset = 0;
	auto bankcount = ddr->getBankCount();
	auto rankcount = ddr->getRankCount();
	auto devicecount = ddr->getDeviceCount();

	j["RankCount"] = ddr->getRankCount();
	j["DeviceCount"] = ddr->getDeviceCount();
	j["BankCount"] = ddr->getBankCount();
	j["TotalEnergy"] = core_energy.total() + interface_energy.total();

	// Energy object to json
	core_energy.to_json(j["CoreEnergy"]);
	interface_energy.to_json(j["InterfaceEnergy"]);

	// Validate array length
	if ( !j["CoreEnergy"][core_energy.get_Bank_energy_keyword()].is_array() || j["CoreEnergy"][core_energy.get_Bank_energy_keyword()].size() != rankcount * bankcount * devicecount )
	{
		assert(false); // (should not happen)
		std::cerr << "Invalid energy array length" << std::endl;
		exit(1);
	}
	
	// Add rank,device,bank description
	for ( std::size_t r = 0; r < rankcount; r++ ) {
		for ( std::size_t d = 0; d < devicecount; d++ ) {
			energy_offset = r * bankcount * devicecount + d * bankcount;
			for ( std::size_t b = 0; b < bankcount; b++ ) {
				// Rank,Device,Bank -> bank_energy
				j["CoreEnergy"][core_energy.get_Bank_energy_keyword()].at(energy_offset + b)["Rank"] = r;
				j["CoreEnergy"][core_energy.get_Bank_energy_keyword()].at(energy_offset + b)["Device"] = d;
				j["CoreEnergy"][core_energy.get_Bank_energy_keyword()].at(energy_offset + b)["Bank"] = b;
			}
		}
	}
	
	out << j.dump(4) << std::endl;
	out.close();
}

void stdoutResult(const std::unique_ptr<dram_base<CmdType>> &ddr, const energy_t &core_energy, const interface_energy_info_t &interface_energy)
{
	// Setup output format
	std::cout << std::defaultfloat << std::setprecision(3);
	// Print stats
	auto bankcount = ddr->getBankCount();
	auto rankcount = ddr->getRankCount();
	auto devicecount = ddr->getDeviceCount();
	size_t energy_offset = 0;
	// NOTE: ensure the same order of calculation in the interface
	spdlog::info("Rank,Device,Bank -> bank_energy:");
	for ( std::size_t r = 0; r < rankcount; r++ ) {
		for ( std::size_t d = 0; d < devicecount; d++ ) {
			energy_offset = r * bankcount * devicecount + d * bankcount;
			for ( std::size_t b = 0; b < bankcount; b++ ) {
				// Rank,Device,Bank -> bank_energy
				spdlog::info("{},{},{} -> {}",
					r, d, b,
					core_energy.bank_energy[energy_offset + b]
				);
			}
		}
	}
	spdlog::info("Cumulated bank energy with bg_act_shared -> {}", core_energy.total_energy());
	spdlog::info("Shared energy -> {}", core_energy);
	spdlog::info("Interface Energy:\n{}", interface_energy);
	spdlog::info("Total Energy -> {}", core_energy.total() + interface_energy.total());
}

int main(int argc, char *argv[])
{
	// Application description
	CLI::App app{"DRAMPower v" DRAMPOWER_VERSION_STRING};
	argv = app.ensure_utf8(argv);
	// Options
	std::string configfile;
	std::string tracefile;
	std::string memspec;
	std::optional<std::string> jsonfile = std::nullopt;
	// Configfile
	app.add_option("-c,--config", configfile, "config")
		->required(true)
		->check(CLI::ExistingFile);
	// Tracefile
	app.add_option("-t,--trace", tracefile, "csv trace file")
		->required(true)
		->check(CLI::ExistingFile);
	// Memspec
	app.add_option("-m,--memspec", memspec, "json memspec file")
		->required(true)
		->check(CLI::ExistingFile);
	// JSON output file
	app.add_option("-j,--json", jsonfile, "json output file path")
		->required(false)
		->check(CLI::ExistingFile);
	// Parse arguments
	CLI11_PARSE(app, argc, argv);

	// Set spdlog pattern
	spdlog::set_pattern("%v");

	CLIConfig config;
	// Get config file and parse
	try
	{
		std::ifstream file(configfile);
		if (!file.is_open()) {
			spdlog::info("Cannot open config file");
			exit(1);
		}
		json_t json_obj = json_t::parse(file);
		config = json_obj;
		from_json(json_obj, config);
    }
    catch (std::exception&)
    {
		spdlog::info("Invalid config file");
		exit(1);
    }

	// Parse command list (load command list in memory)
	auto commandList = parse_command_list(tracefile);

	// Initialize memory / Create memory object
	std::unique_ptr<dram_base<CmdType>> ddr = getMemory(std::string_view(memspec));
	if (!ddr) {
		std::cerr << "Invalid memory specification" << std::endl;
		exit(1);
	}

    // Set togglingrate
	if (config.useToggleRate) {
		if (!config.toggleRateConfig) {
			spdlog::info("toggleRateConfig missing in config file");
			exit(1);
		}
		ddr->setToggleRate(config.toggleRateConfig);
	}

	// Execute commands
	for ( auto &command : commandList ) {
		ddr->doCoreInterfaceCommand(command.first);
	}

	// Calculate energy and stats
	energy_t core_energy = ddr->calcCoreEnergy(commandList.back().first.timestamp);
    interface_energy_info_t interface_energy = ddr->calcInterfaceEnergy(commandList.back().first.timestamp);
	auto stats = ddr->getStats();

	if(jsonfile)
	{
		jsonFileResult(*jsonfile, std::move(ddr), core_energy, interface_energy);
	}
	else
	{
		stdoutResult(std::move(ddr), core_energy, interface_energy);
	}
	return 0;
};
