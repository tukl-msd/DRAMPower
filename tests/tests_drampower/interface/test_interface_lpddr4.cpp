#include <gtest/gtest.h>

#include <DRAMPower/command/Command.h>

#include <DRAMPower/standards/lpddr4/LPDDR4.h>
#include <DRAMPower/standards/lpddr4/interface_calculation_LPDDR4.h>

#include <memory>
#include <fstream>
#include <string>
#include <stdint.h>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>
#include <variant>

using namespace DRAMPower;

#define SZ_BITS(x) sizeof(x)*8

class DramPowerTest_Interface_LPDDR4 : public ::testing::Test {
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
			{  0, CmdType::ACT, { 1, 0, 0, 2 } },
			{  5, CmdType::WR,  { 1, 0, 0, 0, 16 }, wr_data, SZ_BITS(wr_data) },
			{ 14, CmdType::RD,  { 1, 0, 0, 0, 16 }, rd_data, SZ_BITS(rd_data) },
			{ 23, CmdType::PRE, { 1, 0, 0, 2 } },
			{ 26,  CmdType::END_OF_SIMULATION },
	};

	// Test pattern #2
	std::vector<Command> testPattern_2 = {
		// Timestamp,   Cmd,  { Bank, BG, Rank, Row, Co-lumn}
			{  0, CmdType::ACT, { 1, 0, 0, 2 } },
			{  5, CmdType::WR,  { 1, 0, 0, 0, 16 }, wr_data, SZ_BITS(wr_data) },
			{ 14, CmdType::RD,  { 1, 0, 0, 0, 16 }, rd_data, SZ_BITS(rd_data) },
			{ 23, CmdType::WR,  { 1, 0, 0, 0, 16 }, wr_data, SZ_BITS(wr_data) },
			{ 31, CmdType::WR,  { 1, 0, 0, 0, 16 }, wr_data, SZ_BITS(wr_data) },
			{ 40, CmdType::RD,  { 1, 0, 0, 0, 16 }, rd_data, SZ_BITS(rd_data) },
			{ 49, CmdType::WR,  { 1, 0, 0, 0, 16 }, wr_data, SZ_BITS(wr_data) },
			{ 57, CmdType::RD,  { 1, 0, 0, 0, 16 }, rd_data, SZ_BITS(rd_data) },
			{ 70,  CmdType::END_OF_SIMULATION },
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
	}

	virtual void TearDown()
	{
	}
};

TEST_F(DramPowerTest_Interface_LPDDR4, TestStats)
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

TEST_F(DramPowerTest_Interface_LPDDR4, TestPower)
{
	for (const auto& command : testPattern) {
		ddr->doCoreCommand(command);
		ddr->doInterfaceCommand(command);
	};

	auto stats = ddr->getStats();

	// TODO add tests

	InterfaceCalculation_LPDDR4 interface_calc(this->ddr->memSpec);

	// auto interface_stats = interface_calc.calcEnergy(stats);
	// auto dqs_stats = interface_calc.calcDQSEnergy(stats);
}

TEST_F(DramPowerTest_Interface_LPDDR4, TestDQS)
{
	for (const auto& command : testPattern_2) {
		auto stats = ddr->getWindowStats(command.timestamp);

		ddr->doCoreCommand(command);
		ddr->doInterfaceCommand(command);
	};

	auto stats = ddr->getStats();
	// DQs bus
    EXPECT_EQ(sizeof(wr_data), sizeof(rd_data));
    EXPECT_EQ(ddr->dataBus.getWidth(), spec->bitWidth);
    int number_of_cycles = (SZ_BITS(wr_data) / spec->bitWidth);
	uint_fast8_t NumDQsPairs = spec->bitWidth == 16 ? 2 : 1;
    uint_fast8_t scale = NumDQsPairs * 2; // Differential_Pairs * 2(pairs of 2)
    // f(t) = t / 2;
    int DQS_ones = scale * (number_of_cycles / 2); // scale * (cycles / 2)
    int DQS_zeros = DQS_ones;
    int DQS_zeros_to_ones = DQS_ones;
    int DQS_ones_to_zeros = DQS_zeros;

	// 4 writes
    EXPECT_EQ(stats.writeDQSStats.ones, DQS_ones * 4);
    EXPECT_EQ(stats.writeDQSStats.zeroes, DQS_zeros * 4);
    EXPECT_EQ(stats.writeDQSStats.ones_to_zeroes, DQS_zeros_to_ones * 4);
    EXPECT_EQ(stats.writeDQSStats.zeroes_to_ones, DQS_ones_to_zeros * 4);
	// 3 reads
    EXPECT_EQ(stats.readDQSStats.ones, DQS_ones * 3);
    EXPECT_EQ(stats.readDQSStats.zeroes, DQS_zeros * 3);
    EXPECT_EQ(stats.readDQSStats.ones_to_zeroes, DQS_zeros_to_ones * 3);
    EXPECT_EQ(stats.readDQSStats.zeroes_to_ones, DQS_ones_to_zeros * 3);

	// ASSERT_EQ(stats.readDQSStats.ones, 16 *3); // 16 (burst length) * 3 (reads)
	// ASSERT_EQ(stats.readDQSStats.zeroes, 16*3);

	// ASSERT_EQ(stats.writeDQSStats.ones, 16*4); // 16 (burst length) * 4 (writes)
	// ASSERT_EQ(stats.writeDQSStats.zeroes, 16*4);
}


TEST_F(DramPowerTest_Interface_LPDDR4, Test_Detailed)
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
	ASSERT_EQ(window.commandBus.ones, 3);
	ASSERT_EQ(window.commandBus.zeroes, 27);

	// Cycle 0 to 6 (6 delta)
	window = iterate_to_timestamp(6);
	ASSERT_EQ(window.commandBus.ones, 4);
	ASSERT_EQ(window.commandBus.zeroes, 32);

	// Cycle 0 to 7 (7 delta)
	window = iterate_to_timestamp(7);
	ASSERT_EQ(window.commandBus.ones, 5);
	ASSERT_EQ(window.commandBus.zeroes, 37);

	// Cycle 0 to 8 (8 delta)
	window = iterate_to_timestamp(8);
	ASSERT_EQ(window.commandBus.ones, 7);
	ASSERT_EQ(window.commandBus.zeroes, 41);

	// Cycle 0 to 9 (9 delta)
	window = iterate_to_timestamp(9);
	ASSERT_EQ(window.commandBus.ones, 8);
	ASSERT_EQ(window.commandBus.zeroes, 46);

	// Cycle 0 to 10 (10 delta)
	window = iterate_to_timestamp(10);
	ASSERT_EQ(window.commandBus.ones, 8);
	ASSERT_EQ(window.commandBus.zeroes, 52);

	// Cycle 0 to 14 (14 delta)
	window = iterate_to_timestamp(14);
	ASSERT_EQ(window.commandBus.ones, 8);
	ASSERT_EQ(window.commandBus.zeroes, 76);

	// Cycle 0 to 15 (15 delta)
	window = iterate_to_timestamp(15);
	ASSERT_EQ(window.commandBus.ones, 9);
	ASSERT_EQ(window.commandBus.zeroes, 81);

	// Cycle 0 to 16 (16 delta)
	window = iterate_to_timestamp(16);
	ASSERT_EQ(window.commandBus.ones, 10);
	ASSERT_EQ(window.commandBus.zeroes, 86);

	// Cycle 0 to 17 (17 delta)
	window = iterate_to_timestamp(17);
	ASSERT_EQ(window.commandBus.ones, 12);
	ASSERT_EQ(window.commandBus.zeroes, 90);

	// Cycle 0 to 18 (18 delta)
	window = iterate_to_timestamp(18);
	ASSERT_EQ(window.commandBus.ones, 13);
	ASSERT_EQ(window.commandBus.zeroes, 95);

	// Cycle 0 to 19 (19 delta)
	window = iterate_to_timestamp(19);
	ASSERT_EQ(window.commandBus.ones, 13);
	ASSERT_EQ(window.commandBus.zeroes, 101);

	// Cycle 0 to 23 (23 delta)
	window = iterate_to_timestamp(23);
	ASSERT_EQ(window.commandBus.ones, 13);
	ASSERT_EQ(window.commandBus.zeroes, 125);

	// Cycle 0 to 24 (24 delta)
	window = iterate_to_timestamp(24);
	ASSERT_EQ(window.commandBus.ones, 14);
	ASSERT_EQ(window.commandBus.zeroes, 130);

	
	window = iterate_to_timestamp(25);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 135);

	window = iterate_to_timestamp(26);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 141);

	window = iterate_to_timestamp(26);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 141);

	window = iterate_to_timestamp(26);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 141);

	// EOS
	window = iterate_to_timestamp(26);
	ASSERT_EQ(window.commandBus.ones, 15);
	ASSERT_EQ(window.commandBus.zeroes, 141);

	// After EOS
	for(auto i = 1; i <= 100; i++) {
		window = iterate_to_timestamp(26+i);
		ASSERT_EQ(window.commandBus.ones, 15);
		ASSERT_EQ(window.commandBus.zeroes, 141+i*6);
	}
}

// Tests for power consumption (given a known SimulationStats)
class LPDDR4_Energy_Tests : public ::testing::Test {
   public:
	LPDDR4_Energy_Tests() {
		auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr4.json");
		spec = std::make_unique<DRAMPower::MemSpecLPDDR4>(DRAMPower::MemSpecLPDDR4::from_memspec(*data));

		t_CK = spec->memTimingSpec.tCK;
		voltage = spec->vddq;

		// Change impedances to different values from each other
		spec->memImpedanceSpec.cb_R_eq = 2;
        spec->memImpedanceSpec.ck_R_eq = 3;
        spec->memImpedanceSpec.rdqs_R_eq = 4;
        spec->memImpedanceSpec.wdqs_R_eq = 5;
		spec->memImpedanceSpec.rb_R_eq = 6;
		spec->memImpedanceSpec.wb_R_eq = 7;

        spec->memImpedanceSpec.cb_dyn_E = 8;
		spec->memImpedanceSpec.ck_dyn_E = 9;
		spec->memImpedanceSpec.rdqs_dyn_E = 10;
		spec->memImpedanceSpec.wdqs_dyn_E = 11;
		spec->memImpedanceSpec.rb_dyn_E = 12;
		spec->memImpedanceSpec.wb_dyn_E = 13;

		io_calc = std::make_unique<InterfaceCalculation_LPDDR4>(*spec);
	}

	std::unique_ptr<MemSpecLPDDR4> spec;
	double t_CK;
	double voltage;
	std::unique_ptr<InterfaceCalculation_LPDDR4> io_calc;
};

TEST_F(LPDDR4_Energy_Tests, Parameters) {
	ASSERT_TRUE(t_CK > 0.0);
	ASSERT_TRUE(voltage > 0.0);
}

TEST_F(LPDDR4_Energy_Tests, Clock_Energy) {
	// TODO implement test with new interface model
	SimulationStats stats;
	stats.clockStats.ones = 43;
	stats.clockStats.zeroes_to_ones = 47;
	stats.clockStats.zeroes = 53;
	stats.clockStats.ones_to_zeroes = 59;

	interface_energy_info_t result = io_calc->calculateEnergy(stats);
	// Clock is provided by the controller not the device
	EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, 0.0);
	EXPECT_DOUBLE_EQ(result.dram.staticEnergy, 0.0);

	// Clock is differential so there is always going to be one signal that consumes power
	// Calculation is done considering number of ones but could also be zeroes, since clock is
	// a symmetrical signal
	// f(t) = t / 2
	// Differential pair 2 limes in N = 2 * N_single
	// E = f(N * t_CK) * U^2/R
	double expected_static =
		stats.clockStats.ones * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.ck_R_eq;
	// E = N * 1/2 * C * U^2
	double expected_dynamic =
		stats.clockStats.zeroes_to_ones * spec->memImpedanceSpec.ck_dyn_E;

	EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static);  // value itself doesn't matter, only that it matches the formula
	EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic);
	EXPECT_TRUE(result.controller.staticEnergy > 0.0);
	EXPECT_TRUE(result.controller.dynamicEnergy > 0.0);
}

TEST_F(LPDDR4_Energy_Tests, DQS_Energy) {
	// TODO implement test with new interface model
	SimulationStats stats;
	stats.readDQSStats.ones = 200;
	stats.readDQSStats.zeroes = 400;
	stats.readDQSStats.zeroes_to_ones = 700;
	stats.readDQSStats.ones_to_zeroes = 1000;

	stats.writeDQSStats.ones = 300;
	stats.writeDQSStats.zeroes = 100;
	stats.writeDQSStats.ones_to_zeroes = 2000;
	stats.writeDQSStats.zeroes_to_ones = 999;

	// Term power if consumed by 1's
	// Controller -> write power
	// Dram -> read power
	double expected_static_controller = stats.writeDQSStats.ones * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.wdqs_R_eq;
	double expected_static_dram = stats.readDQSStats.ones * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.rdqs_R_eq;


	// Dynamic power is consumed on 0 -> 1 transition
	double expected_dynamic_controller = stats.writeDQSStats.zeroes_to_ones * spec->memImpedanceSpec.wdqs_dyn_E;
	double expected_dynamic_dram = stats.readDQSStats.zeroes_to_ones * spec->memImpedanceSpec.rdqs_dyn_E;

	interface_energy_info_t result = io_calc->calculateEnergy(stats);
	EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
	EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
	EXPECT_DOUBLE_EQ(result.dram.staticEnergy, expected_static_dram);
	EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, expected_dynamic_dram);

	EXPECT_TRUE(result.dram.staticEnergy > 0.0);
	EXPECT_TRUE(result.dram.dynamicEnergy > 0.0);
}

TEST_F(LPDDR4_Energy_Tests, DQ_Energy) {
	// TODO implement test with new interface model
	SimulationStats stats;
	stats.readBus.ones = 7;
	stats.readBus.zeroes = 11;
	stats.readBus.zeroes_to_ones = 19;
	stats.readBus.ones_to_zeroes = 39;

	stats.writeBus.ones = 43;
	stats.writeBus.zeroes = 59;
	stats.writeBus.zeroes_to_ones = 13;
	stats.writeBus.ones_to_zeroes = 17;

	// Term power if consumed by 1's on LPDDR4 (pulldown terminated)
	// Controller -> write power
	// Dram -> read power
	double expected_static_controller =
		stats.writeBus.ones * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.wb_R_eq;
	double expected_static_dram =
		stats.readBus.ones * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.rb_R_eq;

	// Dynamic power is consumed on 0 -> 1 transition
	double expected_dynamic_controller = stats.writeBus.zeroes_to_ones * spec->memImpedanceSpec.wb_dyn_E;
	double expected_dynamic_dram = stats.readBus.zeroes_to_ones * spec->memImpedanceSpec.rb_dyn_E;

	interface_energy_info_t result = io_calc->calculateEnergy(stats);
	EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
	EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
	EXPECT_DOUBLE_EQ(result.dram.staticEnergy, expected_static_dram);
	EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, expected_dynamic_dram);

	EXPECT_TRUE(result.controller.staticEnergy > 0.0);
	EXPECT_TRUE(result.controller.dynamicEnergy > 0.0);
	EXPECT_TRUE(result.dram.staticEnergy > 0.0);
	EXPECT_TRUE(result.dram.dynamicEnergy > 0.0);
}

TEST_F(LPDDR4_Energy_Tests, CA_Energy) {
	// TODO implement test with new interface model
	SimulationStats stats;
	stats.commandBus.ones = 11;
	stats.commandBus.zeroes = 29;
	stats.commandBus.zeroes_to_ones = 39;
	stats.commandBus.ones_to_zeroes = 49;

	double expected_static_controller =
		stats.commandBus.ones * voltage * voltage * t_CK / spec->memImpedanceSpec.cb_R_eq;
	double expected_dynamic_controller = stats.commandBus.zeroes_to_ones * spec->memImpedanceSpec.cb_dyn_E;

	interface_energy_info_t result = io_calc->calculateEnergy(stats);

	// CA bus power is provided by the controller
	EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, 0.0);
	EXPECT_DOUBLE_EQ(result.dram.staticEnergy, 0.0);

	EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
	EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);

	EXPECT_TRUE(result.controller.staticEnergy > 0.0);
	EXPECT_TRUE(result.controller.dynamicEnergy > 0.0);
}
