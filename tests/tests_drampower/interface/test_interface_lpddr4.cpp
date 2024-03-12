#include <gtest/gtest.h>

#include <DRAMPower/command/Command.h>

#include <DRAMPower/standards/lpddr4/LPDDR4.h>
#include <DRAMPower/standards/lpddr4/interface_calculation_LPDDR4.h>

#include <memory>
#include <stdint.h>

using namespace DRAMPower;

class DramPowerTest_Interface_LPDDR4 : public ::testing::Test {
protected:
	static constexpr uint8_t wr_data[] = {
		0, 0, 0, 0,  0, 0, 0, 255,  0, 0, 0, 0,  0, 0, 0, 0,
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 255,
	};

	// Ones: 16
	// Zeroes: 240
	// 0 -> 1: 16
	// 1 -> 0: 16
	// Toggles: 32

	static constexpr uint8_t rd_data[] = {
		0, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	};

	// Test pattern #1
	std::vector<Command> testPattern = {
		// Timestamp,   Cmd,  { Bank, BG, Rank, Row, Co-lumn}
			{  0, CmdType::ACT, { 1, 0, 0, 2 } },
			{  4, CmdType::WR,  { 1, 0, 0, 0, 4 }, wr_data, sizeof(wr_data) * 8 },
			{ 10, CmdType::RD,  { 1, 0, 0, 0, 4 }, rd_data, sizeof(rd_data) * 8 },
			{ 17, CmdType::PRE, { 1, 0, 0, 2 } },
			{ 24,  CmdType::END_OF_SIMULATION },
	};

	// Test pattern #2
	std::vector<Command> testPattern_2 = {
		// Timestamp,   Cmd,  { Bank, BG, Rank, Row, Co-lumn}
			{  0, CmdType::WR,  { 1, 0, 0, 0, 4 }, wr_data, sizeof(wr_data) * 8 },
			{ 10, CmdType::RD,  { 1, 0, 0, 0, 4 }, rd_data, sizeof(rd_data) * 8 },
			{ 20, CmdType::WR,  { 1, 0, 0, 0, 4 }, wr_data, sizeof(wr_data) * 8 },
			{ 28, CmdType::WR,  { 1, 0, 0, 0, 4 }, wr_data, sizeof(wr_data) * 8 },
			{ 38, CmdType::RD,  { 1, 0, 0, 0, 4 }, rd_data, sizeof(rd_data) * 8 },
			{ 50, CmdType::WR,  { 1, 0, 0, 0, 4 }, wr_data, sizeof(wr_data) * 8 },
			{ 58, CmdType::RD,  { 1, 0, 0, 0, 4 }, rd_data, sizeof(rd_data) * 8 },
			{ 70,  CmdType::END_OF_SIMULATION },
	};


	// Test variables
	std::unique_ptr<DRAMPower::LPDDR4> ddr;

	virtual void SetUp()
	{
		MemSpecLPDDR4 memSpec;
		memSpec.numberOfRanks = 1;
		memSpec.numberOfBanks = 2;
		memSpec.bitWidth = 16;
		memSpec.burstLength = 16;
		memSpec.dataRate = 2;

		// mem timings
		memSpec.memTimingSpec.tRAS = 10;
		memSpec.memTimingSpec.tRTP = 10;
		memSpec.memTimingSpec.tRCD = 20;
		memSpec.memTimingSpec.tRFC = 10;
		memSpec.memTimingSpec.tRFCPB = 25;

		memSpec.memTimingSpec.tWR = 20;
		memSpec.memTimingSpec.tRP = 20;
		memSpec.memTimingSpec.tWL = 0;
		memSpec.memTimingSpec.tCK = 1;
		memSpec.memTimingSpec.tREFI = 1;

		// Voltage domains
		memSpec.memPowerSpec.resize(8);
		memSpec.memPowerSpec[0].vDDX = 1;
		memSpec.memPowerSpec[0].iDD0X = 64;
		memSpec.memPowerSpec[0].iDD2NX = 8;
		memSpec.memPowerSpec[0].iDD2PX = 6;
		memSpec.memPowerSpec[0].iDD3NX = 32;
		memSpec.memPowerSpec[0].iDD3PX = 20;
		memSpec.memPowerSpec[0].iDD4RX = 72;
		memSpec.memPowerSpec[0].iDD4WX = 72;

		memSpec.memPowerSpec[2].vDDX = 1;
		memSpec.memPowerSpec[2].iDD0X = 64;
		memSpec.memPowerSpec[2].iDD2NX = 8;
		memSpec.memPowerSpec[2].iDD2PX = 6;
		memSpec.memPowerSpec[2].iDD3NX = 32;
		memSpec.memPowerSpec[2].iDD3PX = 20;
		memSpec.memPowerSpec[2].iDD4RX = 72;
		memSpec.memPowerSpec[2].iDD4WX = 72;


		// Impedance specs
		memSpec.memImpedanceSpec.C_total_ck = 5.0;
		memSpec.memImpedanceSpec.C_total_cb = 5.0;
		memSpec.memImpedanceSpec.C_total_rb = 5.0;
		memSpec.memImpedanceSpec.C_total_wb = 5.0;

		memSpec.memImpedanceSpec.R_eq_ck = 2.0;
		memSpec.memImpedanceSpec.R_eq_cb = 2.0;
		memSpec.memImpedanceSpec.R_eq_rb = 2.0;
		memSpec.memImpedanceSpec.R_eq_wb = 2.0;

		memSpec.bwParams.bwPowerFactRho = 0.333333333;

		ddr = std::make_unique<LPDDR4>(memSpec);
	}

	virtual void TearDown()
	{
	}
};

TEST_F(DramPowerTest_Interface_LPDDR4, TestStats)
{
	Rank & rank_1 = ddr->ranks[0];

	for (const auto& command : testPattern) {
		auto stats = ddr->getWindowStats(command.timestamp);
		int j = 2;

		ddr->doCommand(command);
		ddr->handleInterfaceCommand(command);
	};

	auto stats = ddr->getStats();

	ASSERT_EQ(stats.commandBus.ones, 15);
	//ASSERT_EQ(stats.commandBus.zeroes, 129);
	ASSERT_EQ(stats.commandBus.ones_to_zeroes, 14);
	ASSERT_EQ(stats.commandBus.zeroes_to_ones, 14);

	// Verify read bus stats
	ASSERT_EQ(stats.readBus.ones, 1);
	ASSERT_EQ(stats.readBus.zeroes, 383);
	ASSERT_EQ(stats.readBus.ones_to_zeroes, 1);
	ASSERT_EQ(stats.readBus.zeroes_to_ones, 1);
	ASSERT_EQ(stats.readBus.bit_changes, 2);

	// Verify write bus stats
	ASSERT_EQ(stats.writeBus.ones, 16);
	ASSERT_EQ(stats.writeBus.zeroes, 368);
	ASSERT_EQ(stats.writeBus.ones_to_zeroes, 16);
	ASSERT_EQ(stats.writeBus.zeroes_to_ones, 16);
	ASSERT_EQ(stats.writeBus.bit_changes, 32);
}

TEST_F(DramPowerTest_Interface_LPDDR4, TestPower)
{
	Rank& rank_1 = ddr->ranks[0];

	for (const auto& command : testPattern) {
		auto stats = ddr->getWindowStats(command.timestamp);
		int j = 2;

		ddr->doCommand(command);
		ddr->handleInterfaceCommand(command);
	};

	auto stats = ddr->getStats();

	InterfacePowerCalculation_LPPDR4 interface_calc(this->ddr->memSpec);

	auto interface_stats = interface_calc.calcEnergy(stats);
	auto dqs_stats = interface_calc.calcDQSEnergy(stats);
}

TEST_F(DramPowerTest_Interface_LPDDR4, TestDQS)
{
	Rank& rank_1 = ddr->ranks[0];

	for (const auto& command : testPattern_2) {
		auto stats = ddr->getWindowStats(command.timestamp);

		ddr->doCommand(command);
		ddr->handleInterfaceCommand(command);
	};

	auto stats = ddr->getStats();

	ASSERT_EQ(stats.readDQSStats.ones, 24*2);
	ASSERT_EQ(stats.readDQSStats.zeroes, 24*2);

	ASSERT_EQ(stats.writeDQSStats.ones, 32*2);
	ASSERT_EQ(stats.writeDQSStats.zeroes, 32*2);
}


TEST_F(DramPowerTest_Interface_LPDDR4, Test_Detailed)
{
	SimulationStats window;

	auto iterate_to_timestamp = [this, command = testPattern.begin()](timestamp_t timestamp) mutable {
		while (command != this->testPattern.end() && command->timestamp <= timestamp) {
			ddr->doCommand(*command);
			ddr->handleInterfaceCommand(*command);
			++command;
		}

		return this->ddr->getWindowStats(timestamp);
	};

	// Inspect first rank
	auto & rank_1 = ddr->ranks[0];

	// Cycle 0 to 0 (0 delta)
	window = iterate_to_timestamp(0);
	ASSERT_EQ(window.commandBus.ones, 0);
	ASSERT_EQ(window.commandBus.zeroes, 0);

	// Cycle 0 to 1 (1 delta)
	window = iterate_to_timestamp(1);
	ASSERT_EQ(window.commandBus.ones, 1);
	ASSERT_EQ(window.commandBus.zeroes, 5);

	// Cycle 0 to 2 (2 delta)
	window = iterate_to_timestamp(2);
	ASSERT_EQ(window.commandBus.ones, 2);
	ASSERT_EQ(window.commandBus.zeroes, 10);

	// Cycle 0 to 3 (3 delta)
	window = iterate_to_timestamp(3);
	ASSERT_EQ(window.commandBus.ones, 2);
	ASSERT_EQ(window.commandBus.zeroes, 16);

	// Cycle 0 to 4 (4 delta)
	window = iterate_to_timestamp(4);
	ASSERT_EQ(window.commandBus.ones, 3);
	ASSERT_EQ(window.commandBus.zeroes, 21);

	// Cycle 0 to 5 (5 delta)
	window = iterate_to_timestamp(5);
	ASSERT_EQ(window.commandBus.ones, 5);
	ASSERT_EQ(window.commandBus.zeroes, 25);

	// Cycle 0 to 6 (6 delta)
	window = iterate_to_timestamp(6);
	ASSERT_EQ(window.commandBus.ones, 6);
	ASSERT_EQ(window.commandBus.zeroes, 30);

	// Cycle 0 to 10 (10 delta)
	window = iterate_to_timestamp(10);
	ASSERT_EQ(window.commandBus.ones, 8);
	ASSERT_EQ(window.commandBus.zeroes, 52);

	// Cycle 0 to 11 (11 delta)
	window = iterate_to_timestamp(11);
	ASSERT_EQ(window.commandBus.ones, 10);
	ASSERT_EQ(window.commandBus.zeroes, 56);

	// Cycle 0 to 12 (12 delta)
	window = iterate_to_timestamp(12);
	ASSERT_EQ(window.commandBus.ones, 11);
	ASSERT_EQ(window.commandBus.zeroes, 61);

	// Cycle 0 to 13 (13 delta)
	window = iterate_to_timestamp(13);
	ASSERT_EQ(window.commandBus.ones, 13);
	ASSERT_EQ(window.commandBus.zeroes, 65);

	// Cycle 0 to 14 (14 delta)
	window = iterate_to_timestamp(14);
	ASSERT_EQ(window.commandBus.ones, 13);
	ASSERT_EQ(window.commandBus.zeroes, 71);

	// Cycle 0 to 17 (17 delta)
	window = iterate_to_timestamp(17);
	ASSERT_EQ(window.commandBus.ones, 13);
	ASSERT_EQ(window.commandBus.zeroes, 89);

	// Cycle 0 to 18 (18 delta)
	window = iterate_to_timestamp(18);
	ASSERT_EQ(window.commandBus.ones, 14);
	ASSERT_EQ(window.commandBus.zeroes, 94);

	// Cycle 0 to 19 (19 delta)
	window = iterate_to_timestamp(19);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 99);

	// Cycle 0 to 23 (23 delta)
	window = iterate_to_timestamp(23);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 123);

	// Cycle 0 to 24 (24 delta)
	window = iterate_to_timestamp(24);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 129);
}
