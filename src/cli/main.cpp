#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/energy.h>

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
// #include <spdlog>
#include <DRAMUtils/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR4.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR5.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>
#include <variant>
#include <type_traits>

using namespace DRAMPower;


std::unique_ptr<dram_base<CmdType>> getMemory(const json &data)
{
	try
	{
		DRAMPower::MemSpecContainer memspec = data; // Can throw an excption
		std::unique_ptr<dram_base<CmdType>> result = nullptr;
		std::visit( [&result] (auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, DRAMUtils::Config::MemSpecDDR4>)
			{
				MemSpecDDR4 ddr (static_cast<DRAMUtils::Config::MemSpecDDR4>(arg));
				result = std::make_unique<DDR4>(ddr);
			}
			else if constexpr (std::is_same_v<T, DRAMUtils::Config::MemSpecDDR5>)
			{
				MemSpecDDR5 ddr (static_cast<DRAMUtils::Config::MemSpecDDR5>(arg));
				result = std::make_unique<DDR5>(ddr);
			}
			else if constexpr (std::is_same_v<T, DRAMUtils::Config::MemSpecLPDDR4>)
			{
				MemSpecLPDDR4 ddr (static_cast<DRAMUtils::Config::MemSpecLPDDR4>(arg));
				result = std::make_unique<LPDDR4>(ddr);
			}
			else if constexpr (std::is_same_v<T, DRAMUtils::Config::MemSpecLPDDR5>)
			{
				MemSpecLPDDR5 ddr (static_cast<DRAMUtils::Config::MemSpecLPDDR5>(arg));
				result = std::make_unique<LPDDR5>(ddr);
			}
		}, memspec.memspec.getVariant());
		return result;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return nullptr;
	}
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
		if ( DRAMPower::CmdTypeUtil::needs_data(cmd) )
		{
			if ( row.size() < 8 ){
				std::cerr << "Invalid command structure. Row " << rowcounter << std::endl;
				exit(1);
			}
			uint64_t length = 0;
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
		else
		{
			commandList.emplace_back(Command{ timestamp, cmd, { bank_id, bank_group_id, rank_id, row_id, column_id} }, nullptr);
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
	std::unique_ptr<dram_base<CmdType>> ddr = getMemory(data);
	if (!ddr)
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
		ddr->doCoreInterfaceCommand(command.first);
	}

	// Calculate energy and stats
	energy_t core_energy = ddr->calcCoreEnergy(commandList.back().first.timestamp);
    interface_energy_info_t interface_energy = ddr->calcInterfaceEnergy(commandList.back().first.timestamp);
	auto stats = ddr->getStats();

	if(to_json == false)
	{
		// Setup output format
		std::cout << std::defaultfloat << std::setprecision(3);

		// Print stats
		auto bankcount = ddr->getBankCount();
		auto rankcount = ddr->getRankCount();
		auto devicecount = ddr->getDeviceCount();
		size_t energy_offset = 0;
		// TODO assumed this order in interface calculation
		std::cout << "Rank,Device,Bank -> bank_energy:" << std::endl;
		for ( std::size_t r = 0; r < rankcount; r++ ) {
			for ( std::size_t d = 0; d < devicecount; d++ ) {
				energy_offset = r * bankcount * devicecount + d * bankcount;
				for ( std::size_t b = 0; b < bankcount; b++ ) {
					// Rank,Device,Bank -> bank_energy
					std::cout << r << "," << d << "," << b << " -> ";
					std::cout << core_energy.bank_energy[energy_offset + b] << "\n";
				}
			}
		}
		std::cout << "\n";
		// All banks summed up and bg act shared added
		std::cout << "Cumulated bank energy with bg_act_shared -> " << core_energy.total_energy() << "\n";


		// Background energy of all ranks
		std::cout << "Shared energy -> " << core_energy << "\n\n";

        // Interface energy
        std::cout << "Interface Energy: " << "\n";
        std::cout << interface_energy << std::endl;

		// Total energy
		std::cout << "Total Energy -> " << core_energy.total() + interface_energy.total();
		// \n and flush
		std::cout << std::endl;
	}
	else
	{
		// Assume out is valid
		json j;
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

	return 0;
};
