#include <DRAMPower/command/Command.h>
#include <DRAMPower/standards/lpddr5/LPDDR5.h>
#include <DRAMPower/util/json.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "csv.hpp"

#include <vector>
#include <filesystem>
#include <string_view>

using namespace DRAMPower;

std::vector<Command> parse_command_list(std::string_view csv_file)
{
	if (!std::filesystem::exists(csv_file) || std::filesystem::is_directory(csv_file))
	{
		spdlog::error("MemSpec file was not found!");
		exit(1);
	}

	std::vector<Command> commandList;

	csv::CSVFormat format;
	format.no_header();
	format.trim({ ' ', '\t' });

	csv::CSVReader reader{ csv_file, format };

	for (csv::CSVRow& row : reader) { // Input iterator
		// timestamp, command, bank, bank group, rank
		auto timestamp = row[0].get<timestamp_t>();
		auto cmdType = row[1].get_sv();
		auto bank_id = row[2].get<std::size_t>();
		std::size_t bank_group_id = 0;//row[3].get<std::size_t>();
		std::size_t rank_id = 0;//row[4].get<std::size_t>();

		if (cmdType == "ACT") {
			commandList.push_back({ timestamp, CmdType::ACT, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "PRE") {
			commandList.push_back({ timestamp, CmdType::PRE, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "PREA") {
			commandList.push_back({ timestamp, CmdType::PREA, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "PRESB") {
			commandList.push_back({ timestamp, CmdType::PRESB, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "REFA" || cmdType == "REF") {
			commandList.push_back({ timestamp, CmdType::REFA, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "REFB") {
			commandList.push_back({ timestamp, CmdType::REFB, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "REFSB") {
			commandList.push_back({ timestamp, CmdType::REFSB, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "RD") {
			commandList.push_back({ timestamp, CmdType::RD, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "RDA") {
			commandList.push_back({ timestamp, CmdType::RDA, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "WR") {
			commandList.push_back({ timestamp, CmdType::WR, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "WRA") {
			commandList.push_back({ timestamp, CmdType::WRA, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "PDEA") {
			commandList.push_back({ timestamp, CmdType::PDEA, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "PDEP") {
			commandList.push_back({ timestamp, CmdType::PDEP, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "PDXA") {
			commandList.push_back({ timestamp, CmdType::PDXA, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "PDXP") {
			commandList.push_back({ timestamp, CmdType::PDXP, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "SREN") {
			commandList.push_back({ timestamp, CmdType::SREFEN, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "SREX") {
			commandList.push_back({ timestamp, CmdType::SREFEX, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "DSMEN") {
			commandList.push_back({ timestamp, CmdType::DSMEN, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "DSMEX") {
			commandList.push_back({ timestamp, CmdType::DSMEX, { bank_id, bank_group_id, rank_id } });
		}
		else if (cmdType == "END") {
			commandList.push_back({ timestamp, CmdType::END_OF_SIMULATION });
		};
	}

	return commandList;
};

int main(int argc, char *argv[])
{
	if (argc != 3) {
		spdlog::error("Usage: ./CLI command_list ddr4_spec");
		exit(1);
	}

	auto commandList = parse_command_list(argv[1]);

	std::ifstream f((std::string(argv[2])));
	if (!f.is_open()) {
		spdlog::error("Could not open file {}", argv[2]);
		exit(1);
	}

	json data = json::parse(f);
	MemSpecLPDDR5 lpddr5(data["memspec"]);
	LPDDR5 ddr(lpddr5);

	size_t count = 0;

	std::vector<Command> testPattern = {
			{  0, CmdType::ACT,  { 0, 0, 0}},
			{  0, CmdType::ACT,  { 1, 0, 0}},
			{  0, CmdType::ACT,  { 2, 0, 0}},
			{ 15, CmdType::PRE,  { 0, 0, 0}},
			{ 20, CmdType::REFB, { 0, 0, 0}},
			{ 25, CmdType::PRE,  { 1, 0, 0}},
			{ 30, CmdType::RDA,  { 2, 0, 0}},
			{ 50, CmdType::REFB, { 2, 0, 0}},
			{ 60, CmdType::ACT,  { 1, 0, 0}},
			{ 80, CmdType::PRE,  { 1, 0, 0}},
			{ 85, CmdType::REFB, { 1, 0, 0}},
			{ 85, CmdType::REFB, { 0, 0, 0}},
			{ 125, CmdType::END_OF_SIMULATION },
	};

	for (auto &command : testPattern) {
		ddr.doCommand(command);
		count += 1;
	}

	auto energy = ddr.calcEnergy(commandList.back().timestamp);
	auto stats = ddr.getStats();

	std::cout << std::fixed;

	for (int b = 0; b < ddr.memSpec.numberOfBanks; b++) {
		fmt::print("{} -> ACT: {} PRE: {} RD: {}: WR:{} RDA: {} WRA: {} REF: {} BG_ACT*: {} BG_PRE_ {}\n",
			b,
			energy.bank_energy[b].E_act,
			energy.bank_energy[b].E_pre,
			energy.bank_energy[b].E_RD,
			energy.bank_energy[b].E_WR,
			energy.bank_energy[b].E_RDA,
			energy.bank_energy[b].E_WRA,
			energy.bank_energy[b].E_ref_AB,
			energy.bank_energy[b].E_bg_act,
			energy.bank_energy[b].E_bg_pre
		);
	}
	fmt::print("\n");
	fmt::print("## E_bg_act: {}\n", energy.total_energy().E_bg_act);
	fmt::print("## E_bg_pre: {}\n", energy.total_energy().E_bg_pre);
	fmt::print("## E_bg_act_shared: {}\n", energy.E_bg_act_shared);
	fmt::print("## E_PDNA: {}\n", energy.E_PDNA);
	fmt::print("## E_PDNP: {}\n", energy.E_PDNP);
	fmt::print("\n");

	fmt::print("TOTAL ENERGY: {}\n", energy.total_energy().total() + energy.E_sref + energy.E_PDNA + energy.E_PDNP);

	return 0;
};
