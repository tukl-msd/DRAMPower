#include <DRAMPower/command/Command.h>

#include <DRAMPower/standards/ddr4/DDR4.h>
#include <DRAMPower/memspec/MemSpecDDR4.h>
#include <DRAMPower/standards/ddr5/DDR5.h>
#include <DRAMPower/memspec/MemSpecDDR5.h>
#include <DRAMPower/standards/lpddr4/LPDDR4.h>
#include <DRAMPower/memspec/MemSpecLPDDR4.h>
#include <DRAMPower/standards/lpddr5/LPDDR5.h>
#include <DRAMPower/memspec/MemSpecLPDDR5.h>

#include <DRAMPower/util/json.h>
#include "csv.hpp"
#include "util.hpp"
#include <stdint.h>
#include <unordered_map>
#include <array>
#include <utility>

#include <iostream>

#include <vector>
#include <filesystem>
#include <string_view>

using namespace DRAMPower;


bool getMemory(const json data, std::unique_ptr<dram_base<CmdType>> &ddr)
{
	if ( !data.contains("memspec") )
	{
		return false;
	}
	auto memspec = data["memspec"];
	if ( !memspec.contains("memoryId") )
	{
		return false;
	}
	auto memoryId = memspec["memoryId"].get<std::string>();

	if ( memoryId == "ddr4" )
	{
		MemSpecDDR4 ddr4(memspec);
		ddr = std::make_unique<DDR4>(ddr4);
		return true;
	}
	else if ( memoryId == "ddr5" )
	{
		MemSpecDDR5 ddr5(memspec);
		ddr = std::make_unique<DDR5>(ddr5);
		return true;
	}
	else if ( memoryId == "lpddr4" )
	{
		MemSpecLPDDR4 lpddr4(memspec);
		ddr = std::make_unique<LPDDR4>(lpddr4);
		return true;
	}
	else if ( memoryId == "lpddr5" )
	{
		MemSpecLPDDR5 lpddr5(memspec);
		ddr = std::make_unique<LPDDR5>(lpddr5);
		return true;
	}
	return false;
}

std::vector<std::pair<Command, std::unique_ptr<uint8_t[]>>> parse_command_list(std::string_view csv_file)
{
	// Check if csv file exists
	if ( !std::filesystem::exists(csv_file) || std::filesystem::is_directory(csv_file) )
	{
		std::cerr << "CSV file was not found!" << std::endl;
		exit(1);
	}

	std::vector<std::pair<Command, std::unique_ptr<uint8_t[]>>> commandList;

	// Read csv file
	csv::CSVFormat format;
	format.no_header();
	format.trim({ ' ', '\t' });

	csv::CSVReader reader{ csv_file, format };
	
	// loop variables
	uint64_t rowcounter = 0;
	std::size_t rowidx = 0;
	std::size_t size = 0;

	std::size_t rank_id = 0;
	std::size_t bank_group_id = 0;
	std::size_t bank_id = 0;
	std::size_t row_id = 0;
	std::size_t column_id = 0;

	timestamp_t timestamp = 0;
	csv::string_view cmdType;
	std::unordered_map<csv::string_view, DRAMPower::CmdType>::iterator cmdit;
	DRAMPower::CmdType *datait;
	CmdType cmd;

	// Parse csv file
	// timestamp, command, rank, bank_group, bank, row, column, [data]
	constexpr std::size_t MINCSVSIZE = 7;
	for ( csv::CSVRow& row : reader ) {
		rowidx = 0;

		// Read csv row
		if ( row.size() < 7 )
		{
			std::cout << "Invalid command structure. Row " << rowcounter << std::endl;
			++rowcounter;
			continue;
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
		if ( DRAMPower::CmdTypeUtil::needs_data(cmd) )
		{
			if ( row.size() < 8 ){
				std::cout << "Invalid command structure. Row " << rowcounter << std::endl;
				++rowcounter;
				continue;
			}
			uint64_t length = 0;
			std::unique_ptr<uint8_t[]> arr;
			if ( !CLIutil::hexStringToUint8Array(row[rowidx++].get_sv(), arr, length) )
			{
				std::cout << "Invalid data field in row " << rowcounter << std::endl;
				arr.reset();
				++rowcounter;
				continue;
			}
			
			commandList.push_back({{ timestamp, cmd, { bank_id, bank_group_id, rank_id, row_id, column_id}, arr.get(), length*8}, std::move(arr) });
		}
		else
		{
			commandList.push_back({{ timestamp, cmd, { bank_id, bank_group_id, rank_id, row_id, column_id} }, nullptr });
		}
		// Increment row counter
		++rowcounter;
	}

	return commandList;
};

int main(int argc, char *argv[])
{
	bool to_json = false;
	std::ofstream out;

	// Check number of arguments
	if ( argc != 3 && argc != 5) {
		std::cerr << "Usage: ./CLI command_list.csv memspec [--json output_file]" << std::endl;
		exit(1);
	}

	// Parse command list
	auto commandList = parse_command_list(argv[1]);

	// Initialize memory
	// Read memory spec
	std::ifstream f((std::string(argv[2])));
	if ( !f.is_open() ) {
		std::cerr << "Could not open file " << argv[2] << std::endl;
		exit(1);
	}
	// Parse json
	json data = json::parse(f);
	// Create memory object
	std::unique_ptr<dram_base<CmdType>> ddr;
	if ( !getMemory(data, ddr) )
	{
		std::cerr << "Invalid memory specification" << std::endl;
		exit(1);
	}

	// Get json argument
	if ( argc == 5 )
	{
		std::string_view arg(argv[3]);
		if ( arg == "--json" )
		{
			to_json = true;
			out = std::ofstream(argv[4]);
			if ( !out.is_open() )
			{
				std::cerr << "Could not open file " << argv[4] << std::endl;
				exit(1);
			}
		}
	}

	// Execute commands
	for ( auto &command : commandList ) {
		ddr.get()->doCommand(command.first);
		ddr.get()->handleInterfaceCommand(command.first);
	}

	// Calculate energy and stats
	auto energy = ddr.get()->calcEnergyBase(commandList.back().first.timestamp);
	auto stats = ddr.get()->getStatsBase();

	if(to_json == false)
	{
		// Setup output format
		std::cout << std::fixed;

		// Print stats
		auto bankcount = ddr.get()->getBankCount();
		auto rankcount = ddr.get()->getRankCount();
		auto devicecount = ddr.get()->getDeviceCount();
		size_t energy_offset = 0;
		std::cout << "Rank,Device,Bank -> bank_energy" << std::endl;
		for ( std::size_t r = 0; r < rankcount; r++ ) {
			for ( std::size_t d = 0; d < devicecount; d++ ) {
				energy_offset = r * bankcount * devicecount + d * bankcount;
				for ( std::size_t b = 0; b < bankcount; b++ ) {
					// Rank,Device,Bank -> bank_energy
					std::cout << r << "," << d << "," << b << " -> ";
					std::cout << energy.bank_energy[energy_offset + b] << "\n";
				}
			}
		}
		std::cout << "\n";
		// All banks summed up and bg act shared added
		std::cout << "Cumulated bank energy with bg_act_shared -> " << energy.total_energy() << "\n";
		// Background energy of all ranks
		std::cout << "Shared energy -> " << energy << "\n";
		// Total energy
		std::cout << "Total Energy -> " << energy.total();
		// \n and flush
		std::cout << std::endl;
	}
	else
	{
		// Assume out is valid
		json j;
		size_t energy_offset = 0;
		auto bankcount = ddr.get()->getBankCount();
		auto rankcount = ddr.get()->getRankCount();
		auto devicecount = ddr.get()->getDeviceCount();

		j["RankCount"] = ddr.get()->getRankCount();
		j["DeviceCount"] = ddr.get()->getDeviceCount();
		j["BankCount"] = ddr.get()->getBankCount();
		j["TotalEnergy"] = energy.total();

		// Energy object to json
		energy.to_json(j["Energy"]);

		// Validate array length
		if ( !j["Energy"][energy.get_Bank_energy_keyword()].is_array() || j["Energy"][energy.get_Bank_energy_keyword()].size() != rankcount * bankcount * devicecount )
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
					j["Energy"][energy.get_Bank_energy_keyword()].at(energy_offset + b)["Rank"] = r;
					j["Energy"][energy.get_Bank_energy_keyword()].at(energy_offset + b)["Device"] = d;
					j["Energy"][energy.get_Bank_energy_keyword()].at(energy_offset + b)["Bank"] = b;
				}
			}
		}
		
		out << j.dump(4) << std::endl;
		out.close();
	}

	return 0;
};
