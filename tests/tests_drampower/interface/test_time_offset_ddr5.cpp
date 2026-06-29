#include <gtest/gtest.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR5.h>
#include <DRAMPower/standards/test_accessor.h>

#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/standards/ddr5/DDR5.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::DDR5;
using DRAMPower::MemSpecDDR5;
using DRAMPower::SimulationStats;

#define SZ_BITS(x) sizeof(x)*8

static constexpr uint8_t wr_data[] = {
    0, 0, 0, 0,  0, 0, 0, 255,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 255,
};

static constexpr uint8_t rd_data[] = {
    0, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
};

class DDR5_TimeOffset_Tests : public ::testing::Test {
   public:
    DDR5_TimeOffset_Tests() {
        test_patterns.push_back({
            {0 + 1'000'000, CmdType::ACT, {1, 0, 0, 2}},
            {3 + 1'000'000, CmdType::WR, {1, 0, 0, 0, 4}, wr_data, SZ_BITS(wr_data)},
            {12 + 1'000'000, CmdType::RD, {1, 0, 0, 0, 4}, rd_data, SZ_BITS(rd_data)},
            {21 + 1'000'000, CmdType::PRE, {1, 0, 0, 2}},
            {24 + 1'000'000, CmdType::END_OF_SIMULATION},
        });

        initSpec();
        ddr = std::make_unique<DDR5>(*spec);
        ddr->setSimulationTime(1'000'000);
    }

    void initSpec() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "ddr5.json");
        spec = std::make_unique<DRAMPower::MemSpecDDR5>(DRAMPower::MemSpecDDR5::from_memspec(*data));

        spec->bitWidth = 16;
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCoreCommand(command);
            ddr->doInterfaceCommand(command);
        }
    }

    std::vector<std::vector<Command>> test_patterns;
    std::unique_ptr<MemSpecDDR5> spec;
    std::unique_ptr<DDR5> ddr;
};

// Tests for the window stats (ones to zeros, ones etc)
// Write and Read bus are trivial
// Command bus needs to be calculated from command patterns
TEST_F(DDR5_TimeOffset_Tests, Pattern_0) {
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(stats.writeBus.ones, 528); // 2 (datarate) * 16 (bus width) * 24 (time) - 240 (zeroes)
    EXPECT_EQ(stats.writeBus.zeroes, 240);  // 14 (bursts) * 16 (bus width) + 2 (bursts) * 8 (zeroes in data burst)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 24);  // 16 (first burst) + 8 (data ones to zeroes in bursts)
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 24);  // 8 (data ones to zeroes in bursts) + 8 (last burst data) + 8 (end last burst)

    EXPECT_EQ(stats.readBus.ones, 513); // 2 (datarate) * 16 (bus width) * 24 (time) - 255 (zeroes)
    EXPECT_EQ(stats.readBus.zeroes, 255);  // 15 (bursts) * 16 (bus width) + 15 (zeroes in data burst)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 17); // 16 (first burst) + 1 (data ones to zeroes in bursts)
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 17); // 1 (data ones to zeroes in bursts) + 16 (end last burst)

    // Clock
    EXPECT_EQ(stats.clockStats.ones, 48);
    EXPECT_EQ(stats.clockStats.zeroes, 48);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 48);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 48);

    // Notes
    // Pattern.h: first 4 bits of column (C0-C3) are set to 0 (for reads and writes) // TODO correct???
    EXPECT_EQ(stats.commandBus.ones, 282);
    EXPECT_EQ(stats.commandBus.zeroes, 54);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 39);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 39);

    // In this example read data and write data are the same size, so stats should be the same
    EXPECT_EQ(SZ_BITS(wr_data), SZ_BITS(rd_data));
    uint_fast8_t NumDQsPairs = spec->bitWidth == 16 ? 2 : 1;
    uint64_t number_of_cycles = (SZ_BITS(wr_data) / spec->bitWidth);
    uint_fast8_t scale = NumDQsPairs * 2; // Differential_Pairs * 2(pairs of 2)
    // f(t) = t / 2;
    uint64_t DQS_ones = scale * (number_of_cycles / 2); // scale * (cycles / 2)
    uint64_t DQS_zeros = DQS_ones;
    uint64_t DQS_zeros_to_ones = DQS_ones;
    uint64_t DQS_ones_to_zeros = DQS_zeros;
    EXPECT_EQ(stats.writeDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.writeDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.writeDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.writeDQSStats.zeroes_to_ones, DQS_ones_to_zeros);

    // Read strobe should be the same (only because wr_data is same as rd_data in this test)
    EXPECT_EQ(stats.readDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.readDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes_to_ones, DQS_ones_to_zeros);
}
