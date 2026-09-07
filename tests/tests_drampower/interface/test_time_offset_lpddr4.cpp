#include <gtest/gtest.h>

#include <DRAMPower/command/Command.h>

#include <DRAMPower/standards/lpddr4/LPDDR4.h>
#include <DRAMPower/standards/lpddr4/interface_calculation_LPDDR4.h>

#include <memory>
#include <string>
#include <stdint.h>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>
#include <DRAMPower/standards/test_accessor.h>

using namespace DRAMPower;

#define SZ_BITS(x) sizeof(x)*8

class LPDDR4_TimeOffset_Tests : public ::testing::Test {
protected:
	static constexpr uint8_t wr_data[] = {
		0, 0, 0, 0,  0, 0, 0, 255,  0, 0, 0, 0,  0, 0, 0, 0,
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 255,
	};
	// Cycles: 16
	// Ones: 16
	// Zeroes: 240
	// 0 -> 1: 16
	// 1 -> 0: 16
	// Toggles: 32

	static constexpr uint8_t rd_data[] = {
		0, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	};
	// Cycles: 16
	// Ones: 1
	// Zeroes: 255
	// 0 -> 1: 1
	// 1 -> 0: 1
	// Toggles: 2

	// Test pattern #1
	std::vector<Command> testPattern = {
		// Timestamp,   Cmd,  { Bank, BG, Rank, Row, Co-lumn}
			{  0 + 1'000'000, CmdType::ACT, { 1, 0, 0, 2 } },
			{  5 + 1'000'000, CmdType::WR,  { 1, 0, 0, 0, 16 }, wr_data, SZ_BITS(wr_data) },
			{ 14 + 1'000'000, CmdType::RD,  { 1, 0, 0, 0, 16 }, rd_data, SZ_BITS(rd_data) },
			{ 23 + 1'000'000, CmdType::PRE, { 1, 0, 0, 2 } },
			{ 26 + 1'000'000,  CmdType::END_OF_SIMULATION },
	};


	// Test variables
	std::unique_ptr<LPDDR4> ddr;
    std::unique_ptr<MemSpecLPDDR4> spec;

	virtual void SetUp()
	{
		auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr4.json");
        auto memSpec = MemSpecLPDDR4::from_memspec(*data);

		memSpec.numberOfRanks = 1;
		memSpec.numberOfBanks = 2;
		memSpec.bitWidth = 16;
		memSpec.burstLength = 16;
		memSpec.dataRate = 2;
		memSpec.numberOfDevices = 1;

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
		memSpec.memPowerSpec[0].vDDX = 1;
		memSpec.memPowerSpec[0].iDD0X = 64;
		memSpec.memPowerSpec[0].iDD2NX = 8;
		memSpec.memPowerSpec[0].iDD2PX = 6;
		memSpec.memPowerSpec[0].iDD3NX = 32;
		memSpec.memPowerSpec[0].iDD3PX = 20;
		memSpec.memPowerSpec[0].iDD4RX = 72;
		memSpec.memPowerSpec[0].iDD4WX = 72;

		memSpec.memPowerSpec[1].vDDX = 1;
		memSpec.memPowerSpec[1].iDD0X = 64;
		memSpec.memPowerSpec[1].iDD2NX = 8;
		memSpec.memPowerSpec[1].iDD2PX = 6;
		memSpec.memPowerSpec[1].iDD3NX = 32;
		memSpec.memPowerSpec[1].iDD3PX = 20;
		memSpec.memPowerSpec[1].iDD4RX = 72;
		memSpec.memPowerSpec[1].iDD4WX = 72;

		memSpec.bwParams.bwPowerFactRho = 0.333333333;
	
		spec = std::make_unique<MemSpecLPDDR4>(memSpec);
		ddr = std::make_unique<LPDDR4>(memSpec);
        ddr->setSimulationTime(1'000'000);
	}

	virtual void TearDown()
	{
	}
};

TEST_F(LPDDR4_TimeOffset_Tests, TestStats)
{
	for (const auto& command : testPattern) {
		ddr->doCoreCommand(command);
		ddr->doInterfaceCommand(command);
	};

	auto stats = ddr->getStats();

	ASSERT_EQ(stats.commandBus.ones, 15);
	ASSERT_EQ(stats.commandBus.zeroes, 141);
	ASSERT_EQ(stats.commandBus.ones_to_zeroes, 14);
	ASSERT_EQ(stats.commandBus.zeroes_to_ones, 14);

	// Verify read bus stats
	ASSERT_EQ(stats.readBus.ones, 1);	// 1
	ASSERT_EQ(stats.readBus.zeroes, 831);  // 2 (datarate) * 26 (time) * 16 (bus width) - 1 (ones) 
	ASSERT_EQ(stats.readBus.ones_to_zeroes, 1);
	ASSERT_EQ(stats.readBus.zeroes_to_ones, 1);
	ASSERT_EQ(stats.readBus.bit_changes, 2);

	// Verify write bus stats
	ASSERT_EQ(stats.writeBus.ones, 16);	   // 2 (bursts) * 8 (ones per burst)
	ASSERT_EQ(stats.writeBus.zeroes, 816); // 2 (datarate) * 26 (time) * 16 (bus width) - 16 (ones) 
	ASSERT_EQ(stats.writeBus.ones_to_zeroes, 16);
	ASSERT_EQ(stats.writeBus.zeroes_to_ones, 16);
	ASSERT_EQ(stats.writeBus.bit_changes, 32);
}


TEST_F(LPDDR4_TimeOffset_Tests, Test_Detailed)
{
	SimulationStats window;

	auto iterate_to_timestamp = [this, command = testPattern.begin()](timestamp_t timestamp) mutable {
		while (command != this->testPattern.end() && command->timestamp <= timestamp) {
			ddr->doCoreCommand(*command);
			ddr->doInterfaceCommand(*command);
			++command;
		}

		return this->ddr->getWindowStats(timestamp);
	};

	// Cycle 0 to 0 (0 delta)
	window = iterate_to_timestamp(0 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 0);
	ASSERT_EQ(window.commandBus.zeroes, 0);

	// Cycle 0 to 1 (1 delta)
	window = iterate_to_timestamp(1 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 1);
	ASSERT_EQ(window.commandBus.zeroes, 5);

	// Cycle 0 to 2 (2 delta)
	window = iterate_to_timestamp(2 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 2);
	ASSERT_EQ(window.commandBus.zeroes, 10);

	// Cycle 0 to 3 (3 delta)
	window = iterate_to_timestamp(3 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 2);
	ASSERT_EQ(window.commandBus.zeroes, 16);

	// Cycle 0 to 4 (4 delta)
	window = iterate_to_timestamp(4 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 3);
	ASSERT_EQ(window.commandBus.zeroes, 21);

	// Cycle 0 to 5 (5 delta)
	window = iterate_to_timestamp(5 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 3);
	ASSERT_EQ(window.commandBus.zeroes, 27);

	// Cycle 0 to 6 (6 delta)
	window = iterate_to_timestamp(6 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 4);
	ASSERT_EQ(window.commandBus.zeroes, 32);

	// Cycle 0 to 7 (7 delta)
	window = iterate_to_timestamp(7 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 5);
	ASSERT_EQ(window.commandBus.zeroes, 37);

	// Cycle 0 to 8 (8 delta)
	window = iterate_to_timestamp(8 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 7);
	ASSERT_EQ(window.commandBus.zeroes, 41);

	// Cycle 0 to 9 (9 delta)
	window = iterate_to_timestamp(9 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 8);
	ASSERT_EQ(window.commandBus.zeroes, 46);

	// Cycle 0 to 10 (10 delta)
	window = iterate_to_timestamp(10 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 8);
	ASSERT_EQ(window.commandBus.zeroes, 52);

	// Cycle 0 to 14 (14 delta)
	window = iterate_to_timestamp(14 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 8);
	ASSERT_EQ(window.commandBus.zeroes, 76);

	// Cycle 0 to 15 (15 delta)
	window = iterate_to_timestamp(15 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 9);
	ASSERT_EQ(window.commandBus.zeroes, 81);

	// Cycle 0 to 16 (16 delta)
	window = iterate_to_timestamp(16 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 10);
	ASSERT_EQ(window.commandBus.zeroes, 86);

	// Cycle 0 to 17 (17 delta)
	window = iterate_to_timestamp(17 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 12);
	ASSERT_EQ(window.commandBus.zeroes, 90);

	// Cycle 0 to 18 (18 delta)
	window = iterate_to_timestamp(18 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 13);
	ASSERT_EQ(window.commandBus.zeroes, 95);

	// Cycle 0 to 19 (19 delta)
	window = iterate_to_timestamp(19 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 13);
	ASSERT_EQ(window.commandBus.zeroes, 101);

	// Cycle 0 to 23 (23 delta)
	window = iterate_to_timestamp(23 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 13);
	ASSERT_EQ(window.commandBus.zeroes, 125);

	// Cycle 0 to 24 (24 delta)
	window = iterate_to_timestamp(24 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 14);
	ASSERT_EQ(window.commandBus.zeroes, 130);

	
	window = iterate_to_timestamp(25 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 135);

	window = iterate_to_timestamp(26 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 141);

	window = iterate_to_timestamp(26 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 141);

	window = iterate_to_timestamp(26 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 141);

	// EOS
	window = iterate_to_timestamp(26 + 1'000'000);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 141);

	// After EOS
	for(auto i = 1; i <= 100; i++) {
		window = iterate_to_timestamp(26 + i + 1'000'000);
		ASSERT_EQ(window.commandBus.ones, 15);
		ASSERT_EQ(window.commandBus.zeroes, 141+i*6);
	}
}