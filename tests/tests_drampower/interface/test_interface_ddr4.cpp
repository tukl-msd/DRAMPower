#include <gtest/gtest.h>

#include <memory>
#include <fstream>

#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecDDR4.h"
#include "DRAMPower/standards/ddr4/DDR4.h"
#include "DRAMPower/standards/ddr4/interface_calculation_DDR4.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::DDR4;
using DRAMPower::interface_energy_info_t;
using DRAMPower::InterfaceCalculation_DDR4;
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

class DDR4_WindowStats_Tests : public ::testing::Test {
   public:
    DDR4_WindowStats_Tests() {
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 16}, wr_data, SZ_BITS(wr_data)},
            {11, CmdType::RD, {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)},
            {17, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });
        initSpec();
        ddr = std::make_unique<DDR4>(spec);
    }

    void initSpec() {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "ddr4.json");

        if(!f.is_open()){
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }

        json data = json::parse(f);
        spec = MemSpecDDR4{data["memspec"]};
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCommand(command);
            ddr->handleInterfaceCommand(command);
        }
    }

    std::vector<std::vector<Command>> test_patterns;
    MemSpecDDR4 spec;
    std::unique_ptr<DDR4> ddr;
};

// Tests for the window stats (ones to zeros, ones etc)
// Write and Read bus are trivial
// Command bus needs to be calculated from command patterns
TEST_F(DDR4_WindowStats_Tests, Pattern_0) {
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(stats.writeBus.ones, 8);
    EXPECT_EQ(stats.writeBus.zeroes, 184);  // 24 (time) * 8 (bus width) - 8 (ones)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 8);  // 0 -> 255 = 8 transitions,
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 8);  // back to 0

    EXPECT_EQ(stats.readBus.ones, 1);
    EXPECT_EQ(stats.readBus.zeroes, 191);  // 24 (time) * 8 (bus width) - 1 (ones)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 1);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 1);

    // Notes
    // Pattern.h: first 4 bits of column (C0-C3) are set to 0 (for reads and writes)
    //            "V" bits are 0
    //            Rank doesn't matter
    EXPECT_EQ(stats.commandBus.ones, 14);  // taken by applying the parameters to the command patterns and counting
    EXPECT_EQ(stats.commandBus.zeroes, 661);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 14);  // Since interval between commands is > 2 all commands are interleaved with idle states (all 0's)
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 14);  // so the only possibility of a bit going staying 1 (1->1) is if it stays 1 within the command pattern itself

    // For write and read the number of clock cycles the strobes stay on is
    // currently ("size in bits" / bus_size) / bus_rate
    int number_of_cycles = (SZ_BITS(wr_data) / 8) / spec.dataRate;

    // In this example read data and write data are the same size, so stats should be the same
    int DQS_ones = number_of_cycles; // TODO wrong in LPDDR4?
    int DQS_zeros = DQS_ones;
    int DQS_zeros_to_ones = DQS_ones;
    int DQS_ones_to_zeros = DQS_zeros;
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
    auto prepos = stats.rank_total[0].prepos;
    EXPECT_EQ(prepos.readSeamless, 0);
    EXPECT_EQ(prepos.writeSeamless, 0);
    EXPECT_EQ(prepos.readMerged, 0);
    EXPECT_EQ(prepos.readMergedTime, 0);
    EXPECT_EQ(prepos.writeMerged, 0);
    EXPECT_EQ(prepos.writeMergedTime, 0);
}