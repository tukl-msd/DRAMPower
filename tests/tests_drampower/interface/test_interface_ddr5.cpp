#include <gtest/gtest.h>

#include <memory>
#include <fstream>

#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/standards/ddr5/DDR5.h"
#include "DRAMPower/standards/ddr5/interface_calculation_DDR5.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::DDR5;
using DRAMPower::InterfaceCalculation_DDR5;
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

class DDR5_WindowStats_Tests : public ::testing::Test {
   public:
    DDR5_WindowStats_Tests() {
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 4}, wr_data, SZ_BITS(wr_data)},
            {10, CmdType::RD, {1, 0, 0, 0, 4}, rd_data, SZ_BITS(rd_data)},
            {17, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        test_patterns.push_back({
            {0, CmdType::ACT, {2, 0, 0, 372}},
            {5, CmdType::PRE, {2, 0, 0, 372}},
            {10, CmdType::WRA, {2, 0, 0, 372, 27}, wr_data, SZ_BITS(wr_data)},
            {15, CmdType::SREFEN},
            {25, CmdType::END_OF_SIMULATION}
        });

        test_patterns.push_back({
            {0, CmdType::ACT, {2, 0, 0, 372}},
            {5, CmdType::PRE, {2, 0, 0, 372}},
            {10, CmdType::WRA, {2, 0, 0, 372, 27}, wr_data, SZ_BITS(wr_data)},
            {15, CmdType::RD, {2, 0, 0, 372, 27}, rd_data, SZ_BITS(rd_data)},
            {20, CmdType::SREFEN},
            {30, CmdType::END_OF_SIMULATION}  // RD needs time to finish fully
        });

        initSpec();
        ddr = std::make_unique<DDR5>(spec);
    }

    void initSpec() {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "ddr5.json");

        if(!f.is_open()){
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }

        json data = json::parse(f);
        spec = MemSpecDDR5{data["memspec"]};
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCommand(command);
            ddr->handleInterfaceCommand(command);
        }
    }

    std::vector<std::vector<Command>> test_patterns;
    MemSpecDDR5 spec;
    std::unique_ptr<DDR5> ddr;
};

// Tests for the window stats (ones to zeros, ones etc)
// Write and Read bus are trivial
// Command bus needs to be calculated from command patterns
TEST_F(DDR5_WindowStats_Tests, Pattern_0) {
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(stats.writeBus.ones, 16);
    EXPECT_EQ(stats.writeBus.zeroes, 368);  // 24 (time) * 16 (bus width) - 16 (ones)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 16);  // 0 -> 255 = 8 transitions, *2 = 16
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 16);  // back to 0

    EXPECT_EQ(stats.readBus.ones, 1);
    EXPECT_EQ(stats.readBus.zeroes, 383);  // 24 (time) * 16 (bus width) - 1 (ones)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 1);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 1);

    // Notes
    // Pattern.h: first 4 bits of column (C0-C3) are set to 0 (for reads and writes)
    //            "V" bits are 0
    //            CID and Rank doesn't matter
    EXPECT_EQ(stats.commandBus.ones, 21);  // taken by applying the parameters to the command patterns and counting
    EXPECT_EQ(stats.commandBus.zeroes, 329);  // 14 (bus width) * 25 (time for CA bus is simulation time + 1) - 21 (ones)
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 21);  // Since interval between commands is > 2 all commands are interleaved with idle states (all 0's)
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 21);  // so the only possibility of a bit going staying 1 (1->1) is if it stays 1 within the command pattern itself
}

TEST_F(DDR5_WindowStats_Tests, Pattern_1) {
    runCommands(test_patterns[1]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(stats.writeBus.ones, 16);
    EXPECT_EQ(stats.writeBus.zeroes, 384);
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 16);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 16);

    EXPECT_EQ(stats.readBus.ones, 0);
    EXPECT_EQ(stats.readBus.zeroes, 400);
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 0);

    EXPECT_EQ(stats.commandBus.ones, 23);
    EXPECT_EQ(stats.commandBus.zeroes, 341);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 21);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 21);
}

TEST_F(DDR5_WindowStats_Tests, Pattern_2) {
    runCommands(test_patterns[2]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(stats.writeBus.ones, 16);
    EXPECT_EQ(stats.writeBus.zeroes, 464);
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 16);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 16);

    EXPECT_EQ(stats.readBus.ones, 1);
    EXPECT_EQ(stats.readBus.zeroes, 479);
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 1);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 1);

    EXPECT_EQ(stats.commandBus.ones, 31);
    EXPECT_EQ(stats.commandBus.zeroes, 403);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 28);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 28);
}

// Tests for power consumption (given a known windows stats)
