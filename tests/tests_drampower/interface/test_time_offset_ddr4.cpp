#include <gtest/gtest.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR4.h>
#include <DRAMPower/standards/test_accessor.h>

#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecDDR4.h"
#include "DRAMPower/standards/ddr4/DDR4.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::DDR4;
using DRAMPower::MemSpecDDR4;
using DRAMPower::SimulationStats;

#define SZ_BITS(x) sizeof(x)*8

// burst length = 8 for x8 devices
static constexpr uint8_t wr_data[] = {
    0, 0, 0, 0,  0, 0, 0, 255,
};

// burst length = 8 for x8 devices
static constexpr uint8_t rd_data[] = {
    0, 0, 0, 0,  0, 0, 0, 1,
};

class DDR4_TimeOffset_Tests : public ::testing::Test {
   public:
    DDR4_TimeOffset_Tests() {
        test_patterns.push_back({
            {0 + 1'000'000, CmdType::ACT, {1, 0, 0, 2}},
            {4 + 1'000'000, CmdType::WR, {1, 0, 0, 0, 16}, wr_data, SZ_BITS(wr_data)},
            {11 + 1'000'000, CmdType::RD, {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)},
            {16 + 1'000'000, CmdType::PRE, {1, 0, 0, 2}},
            {24 + 1'000'000, CmdType::END_OF_SIMULATION},
        });

        initSpec();
        ddr = std::make_unique<DDR4>(*spec);
        ddr->setSimulationTime(1'000'000);
    }

    void initSpec() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "ddr4.json");
        spec = std::make_unique<DRAMPower::MemSpecDDR4>(DRAMPower::MemSpecDDR4::from_memspec(*data));
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCoreCommand(command);
            ddr->doInterfaceCommand(command);
        }
    }

    std::vector<std::vector<Command>> test_patterns;
    std::unique_ptr<MemSpecDDR4> spec;
    std::unique_ptr<DDR4> ddr;
};

// Test patterns for stats (counter)
TEST_F(DDR4_TimeOffset_Tests, Pattern_0) {
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);

    // Clock
    EXPECT_EQ(stats.clockStats.ones, 48);
    EXPECT_EQ(stats.clockStats.zeroes, 48);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 48);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 48);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 328); // 2 (datarate) * 24 (time) * 8 (bus width) - 56 (zeroes) 
    EXPECT_EQ(stats.writeBus.zeroes, 56);  // 7 (length) * 8 (bus width)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 8);  // 8 transitions 1 -> 0
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 8);  // back to 1

    EXPECT_EQ(stats.readBus.ones, 321);  // 2 (datarate) * 24 (time) * 8 (bus width) - 63 (zeroes) 
    EXPECT_EQ(stats.readBus.zeroes, 63);  // 7 (length zero bursts) * 8 (bus width) + 7 (zeroes in last burst)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 8);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 8);

    EXPECT_EQ(stats.commandBus.ones, 591);
    EXPECT_EQ(stats.commandBus.zeroes, 57);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 57);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 57);

    // DQs bus
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

    // PrePostamble
    // No seamless preambles or postambles
    auto prepos = stats.rank_total[0].prepos;
    EXPECT_EQ(prepos.readSeamless, 0);
    EXPECT_EQ(prepos.writeSeamless, 0);
    EXPECT_EQ(prepos.readMerged, 0);
    EXPECT_EQ(prepos.readMergedTime, 0);
    EXPECT_EQ(prepos.writeMerged, 0);
    EXPECT_EQ(prepos.writeMergedTime, 0);
}
