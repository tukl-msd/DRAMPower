#include <gtest/gtest.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>
#include <DRAMPower/standards/test_accessor.h>

#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecLPDDR5.h"
#include "DRAMPower/standards/lpddr5/LPDDR5.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::LPDDR5;
using DRAMPower::MemSpecLPDDR5;
using DRAMPower::SimulationStats;

#define SZ_BITS(x) sizeof(x)*8

using namespace DRAMPower;

static constexpr uint8_t wr_data[] = {
    0, 0, 0, 0,  0, 0, 0, 255,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 255,
};

static constexpr uint8_t rd_data[] = {
    0, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
};

class LPDDR5_TimeOffset_Tests : public ::testing::Test {
   public:
    LPDDR5_TimeOffset_Tests() {
		// Timestamp,   Cmd,  { Bank, BG, Rank, Row, Co-lumn}
        test_patterns.push_back({
            {0 + 1'000'000, CmdType::ACT, {1, 0, 0, 2}},
            {3 + 1'000'000, CmdType::WR, {1, 0, 0, 0, 8}, wr_data, SZ_BITS(wr_data)},
            {12 + 1'000'000, CmdType::RD, {1, 0, 0, 0, 8}, rd_data, SZ_BITS(rd_data)},
            {21 + 1'000'000, CmdType::PRE, {1, 0, 0, 2}},
            {24 + 1'000'000, CmdType::END_OF_SIMULATION},
        });

        initSpec();
        ddr = std::make_unique<LPDDR5>(*spec);
        ddr->setSimulationTime(1'000'000);
    }

    void initSpec() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr5.json");
        spec = std::make_unique<DRAMPower::MemSpecLPDDR5>(DRAMPower::MemSpecLPDDR5::from_memspec(*data));

        spec->numberOfDevices = 1;
        spec->bitWidth = 16;
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCoreCommand(command);
            ddr->doInterfaceCommand(command);
        }
    }

    std::vector<std::vector<Command>> test_patterns;
    std::unique_ptr<MemSpecLPDDR5> spec;
    std::unique_ptr<LPDDR5> ddr;
};

// Tests for the window stats (ones to zeros, ones etc)
// Write and Read bus are trivial
// Command bus needs to be calculated from command patterns
TEST_F(LPDDR5_TimeOffset_Tests, Pattern_0) {
    // auto test_pattern = test_patterns[0];
    
    // auto iterate_to_timestamp = [this, command = test_pattern.begin(), end = test_pattern.end()](timestamp_t timestamp) mutable {
	// 	while (command != end && command->timestamp <= timestamp) {
	// 		ddr->doCommand(*command);
	// 		ddr->handleInterfaceCommand(*command);
	// 		++command;
	// 	}

	// 	return this->ddr->getWindowStats(timestamp);
	// };
    // for(auto i = 0; i < 26; i++)
    // {
    //     SimulationStats window = iterate_to_timestamp(i);
    //     std::cout << "Timestamp: " << i << std::endl;
    // }

    // iterate_to_timestamp(test_patterns[0].back().timestamp);
    runCommands(test_patterns[0]);
    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(stats.writeBus.ones, 16);
    EXPECT_EQ(stats.writeBus.zeroes, 752);  // 2 (datarate) * 24 (time) * 16 (bus width) - 16 (ones)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 16);  // 0 -> 255 = 8 transitions, *2 = 16
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 16);  // back to 0

    EXPECT_EQ(stats.readBus.ones, 1);
    EXPECT_EQ(stats.readBus.zeroes, 767);  // 2 (datarate) * 24 (time) * 16 (bus width) - 1 (ones)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 1);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 1);

    EXPECT_EQ(stats.commandBus.ones, 19);
    EXPECT_EQ(stats.commandBus.zeroes, 317);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 15);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 15);

    // For read the number of clock cycles the strobes stay on is
    EXPECT_EQ(sizeof(wr_data), sizeof(rd_data));
    uint64_t number_of_cycles = (SZ_BITS(wr_data) / spec->bitWidth);
    uint_fast8_t scale = 1 * 2; // Differential_Pairs * 2(pairs of 2)
    // f(t) = t / 2;
    uint64_t DQS_ones = scale * (number_of_cycles / 2); // scale * (cycles / 2)
    uint64_t DQS_zeros = DQS_ones;
    uint64_t DQS_zeros_to_ones = DQS_ones;
    uint64_t DQS_ones_to_zeros = DQS_zeros;

    EXPECT_EQ(stats.readDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.readDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes_to_ones, DQS_ones_to_zeros);

    // TODO WCK
}

// Write clock tests
TEST_F(LPDDR5_TimeOffset_Tests, WriteClockAlwaysOn) {
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    uint64_t wck_rate = spec->dataRate / spec->memTimingSpec.WCKtoCK;

    // Number of cycles for always on is the simulation time
    uint64_t wck_ones = 24 * wck_rate;
    EXPECT_EQ(stats.wClockStats.ones, wck_ones);
    EXPECT_EQ(stats.wClockStats.zeroes, wck_ones);
    EXPECT_EQ(stats.wClockStats.ones_to_zeroes, wck_ones);
    EXPECT_EQ(stats.wClockStats.zeroes_to_ones, wck_ones);
}

TEST_F(LPDDR5_TimeOffset_Tests, WriteClockOnDemand) {
    spec->wckAlwaysOnMode = false;
    ddr = std::make_unique<LPDDR5>(*spec);

    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();
    uint64_t wck_rate = spec->dataRate / spec->memTimingSpec.WCKtoCK;

    // Number of clocks of WCK is the length of the write data
    uint64_t cycles = SZ_BITS(wr_data) / 16;

    uint64_t wck_ones = cycles * wck_rate;
    EXPECT_EQ(stats.wClockStats.ones, wck_ones);
    EXPECT_EQ(stats.wClockStats.zeroes, wck_ones);
    EXPECT_EQ(stats.wClockStats.ones_to_zeroes, wck_ones);
    EXPECT_EQ(stats.wClockStats.zeroes_to_ones, wck_ones);
}
