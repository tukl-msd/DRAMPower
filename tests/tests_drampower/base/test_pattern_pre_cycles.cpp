#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/ddr4/DDR4.h>

#include <memory>
#include <fstream>
#include <string>

using namespace DRAMPower;

class DramPowerTest_Pre_Cycles : public ::testing::Test {
protected:
	// Test pattern
	// TODO invalid state transitions
	std::vector<Command> testPattern = {
		// Timestamp,   Cmd,  { Channel, Bank, BG, Rank}
			{   0, CmdType::ACT,  { 0, 0, 0, 0 } },
			{  30, CmdType::PDEA, { 0, 0, 0, 0 } },
			{  40, CmdType::PDXA, { 0, 0, 0, 0 } },
			{  70, CmdType::PRE,  { 0, 0, 0, 0 } },
			{  80, CmdType::ACT,  { 0, 1, 0, 0 } },
			{  90, CmdType::PRE,  { 0, 1, 0, 0 } },
			{ 100,  CmdType::END_OF_SIMULATION },
	};

	// Test variables
	std::unique_ptr<DRAMPower::DDR4> ddr;

	virtual void SetUp()
	{
		auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "ddr4.json");
        auto memSpec = DRAMPower::MemSpecDDR4::from_memspec(*data);

		memSpec.numberOfRanks = 1;
		memSpec.numberOfBanks = 2;
		memSpec.numberOfBankGroups = 2;
		//memSpec.banksPerGroup = 4;

		memSpec.memTimingSpec.tRAS = 10;
		memSpec.memTimingSpec.tRTP = 10;
		memSpec.memTimingSpec.tRCD = 20;
		memSpec.memTimingSpec.tRFC = 10;
		//memSpec.memTimingSpec.tRFCPB = 25;
		//memSpec.memTimingSpec.tRFCsb_slr = 25;

		memSpec.memTimingSpec.tWR = 20;
		memSpec.memTimingSpec.tRP = 20;
		memSpec.memTimingSpec.tWL = 0;
		memSpec.memTimingSpec.tCK = 1;
		//memSpec.memTimingSpec.tREFI = 1;

		memSpec.memPowerSpec[0].vXX = 1;
		memSpec.memPowerSpec[0].iXX0 = 64;
		memSpec.memPowerSpec[0].iXX2N = 8;
		memSpec.memPowerSpec[0].iXX2P = 6;
		memSpec.memPowerSpec[0].iXX3N = 32;
		memSpec.memPowerSpec[0].iXX3P = 20;
		memSpec.memPowerSpec[0].iXX4R = 72;
		memSpec.memPowerSpec[0].iXX4W = 72;
		//memSpec.memPowerSpec[0].iXX5C = 28;
		//memSpec.memPowerSpec[0].iXX5PB_B = 30;

		memSpec.bwParams.bwPowerFactRho = 0.333333333;

		memSpec.burstLength = 16;
		memSpec.dataRate = 2;

		ddr = std::make_unique<DDR4>(memSpec);
	}

	virtual void TearDown()
	{
	}
};

TEST_F(DramPowerTest_Pre_Cycles, Test)
{
	for (const auto& command : testPattern) {
		ddr->doCoreCommand(command);
	};

	auto stats = ddr->getStats();

	// Check bank specific ACT cycle count
	ASSERT_EQ(stats.bank[0].cycles.act, 60);
	ASSERT_EQ(stats.bank[1].cycles.act, 10);

	// Check per-bank PRE count
	ASSERT_EQ(stats.bank[0].cycles.pre, 30);
	ASSERT_EQ(stats.bank[1].cycles.pre, 80);

	// Check global cycles count
	ASSERT_EQ(stats.rank_total[0].cycles.act, 70);
	ASSERT_EQ(stats.rank_total[0].cycles.pre, 20);
}

TEST_F(DramPowerTest_Pre_Cycles, Test_Detailed)
{
	SimulationStats window;

	auto iterate_to_timestamp = [this, command = testPattern.begin()](timestamp_t timestamp) mutable {
		while (command != this->testPattern.end() && command->timestamp <= timestamp) {
			ddr->doCoreCommand(*command);
			++command;
		}

		return this->ddr->getWindowStats(timestamp);
	};

	// Cycle 5
	window = iterate_to_timestamp(5);
	ASSERT_EQ(window.rank_total[0].cycles.act, 5);    ASSERT_EQ(window.rank_total[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[0].cycles.act, 5);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 0);  ASSERT_EQ(window.bank[1].cycles.pre, 5);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 0); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 0);

	// Cycle 10
	window = iterate_to_timestamp(10);
	ASSERT_EQ(window.rank_total[0].cycles.act, 10);    ASSERT_EQ(window.rank_total[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[0].cycles.act, 10);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 0);  ASSERT_EQ(window.bank[1].cycles.pre, 10);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 0); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 0);

	// Cycle 30
	window = iterate_to_timestamp(30);
	ASSERT_EQ(window.rank_total[0].cycles.act, 30);    ASSERT_EQ(window.rank_total[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 0);  ASSERT_EQ(window.bank[1].cycles.pre, 30);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 0); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 0);

	// Cycle 35
	window = iterate_to_timestamp(35);
	ASSERT_EQ(window.rank_total[0].cycles.act, 30);    ASSERT_EQ(window.rank_total[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 0);  ASSERT_EQ(window.bank[1].cycles.pre, 30);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 5); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 0);

	// Cycle 40
	window = iterate_to_timestamp(40);
	ASSERT_EQ(window.rank_total[0].cycles.act, 30);    ASSERT_EQ(window.rank_total[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 0);  ASSERT_EQ(window.bank[1].cycles.pre, 30);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 0);

}
