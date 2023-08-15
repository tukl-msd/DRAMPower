#include <gtest/gtest.h>

#include <memory>
#include <fstream>

#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecLPDDR5.h"
#include "DRAMPower/standards/lpddr5/LPDDR5.h"
#include "DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::LPDDR5;
using DRAMPower::interface_energy_info_t;
using DRAMPower::InterfaceCalculation_LPDDR5;
using DRAMPower::MemSpecLPDDR5;
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

class LPDDR5_WindowStats_Tests : public ::testing::Test {
   public:
    LPDDR5_WindowStats_Tests() {
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {5, CmdType::WR, {1, 0, 0, 0, 4}, wr_data, SZ_BITS(wr_data)},
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
        ddr = std::make_unique<LPDDR5>(spec);
    }

    void initSpec() {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "lpddr5.json");

        if(!f.is_open()){
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }

        json data = json::parse(f);
        spec = MemSpecLPDDR5{data["memspec"]};
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCommand(command);
            ddr->handleInterfaceCommand(command);
        }
    }

    std::vector<std::vector<Command>> test_patterns;
    MemSpecLPDDR5 spec;
    std::unique_ptr<LPDDR5> ddr;
};

// Tests for the window stats (ones to zeros, ones etc)
// Write and Read bus are trivial
// Command bus needs to be calculated from command patterns
TEST_F(LPDDR5_WindowStats_Tests, Pattern_0) {
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
    EXPECT_EQ(stats.commandBus.ones, 18);  // taken by applying the parameters to the command patterns and counting
    EXPECT_EQ(stats.commandBus.zeroes, 157);  // 7 (bus width) * 25 (time for CA bus is simulation time + 1) - (ones)
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 14);  // Since interval between commands is > 2 all commands are interleaved with idle states (all 0's)
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 14);  // so the only possibility of a bit going staying 1 (1->1) is if it stays 1 within the command pattern itself

    // For read the number of clock cycles the strobes stay on is
    // currently ("size in bits" / bus_size) / bus_rate
    int number_of_cycles = (SZ_BITS(wr_data) / 16) / spec.dataRate;

    int DQS_ones = number_of_cycles * spec.dataRate;
    int DQS_zeros = DQS_ones;
    int DQS_zeros_to_ones = DQS_ones;
    int DQS_ones_to_zeros = DQS_zeros;

    EXPECT_EQ(stats.readDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.readDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes_to_ones, DQS_ones_to_zeros);
}

TEST_F(LPDDR5_WindowStats_Tests, Pattern_1) {
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

    EXPECT_EQ(stats.commandBus.ones, 24);
    EXPECT_EQ(stats.commandBus.zeroes, 158);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 20);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 20);

    EXPECT_EQ(stats.readDQSStats.ones, 0);
    EXPECT_EQ(stats.readDQSStats.zeroes, 0);
    EXPECT_EQ(stats.readDQSStats.ones_to_zeroes, 0);
    EXPECT_EQ(stats.readDQSStats.zeroes_to_ones, 0);
}

TEST_F(LPDDR5_WindowStats_Tests, Pattern_2) {
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

    EXPECT_EQ(stats.commandBus.ones, 28);
    EXPECT_EQ(stats.commandBus.zeroes, 189);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 23);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 23);
}

// Write clock tests
TEST_F(LPDDR5_WindowStats_Tests, WriteClockAlwaysOn) {
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    uint64_t wck_rate = spec.dataRate / spec.memTimingSpec.WCKtoCK;

    // Number of cycles for always on is the simulation time
    uint64_t wck_ones = 24 * wck_rate;
    EXPECT_EQ(stats.wClockStats.ones, wck_ones);
    EXPECT_EQ(stats.wClockStats.zeroes, wck_ones);
    EXPECT_EQ(stats.wClockStats.ones_to_zeroes, wck_ones);
    EXPECT_EQ(stats.wClockStats.zeroes_to_ones, wck_ones);
}

TEST_F(LPDDR5_WindowStats_Tests, WriteClockOnDemand) {
    spec.wckAlwaysOnMode = false;
    ddr = std::make_unique<LPDDR5>(spec);

    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();
    uint64_t wck_rate = spec.dataRate / spec.memTimingSpec.WCKtoCK;

    // Number of clocks of WCK is the length of the write data
    uint64_t cycles = SZ_BITS(wr_data) / 16;

    uint64_t wck_ones = cycles * wck_rate;
    EXPECT_EQ(stats.wClockStats.ones, wck_ones);
    EXPECT_EQ(stats.wClockStats.zeroes, wck_ones);
    EXPECT_EQ(stats.wClockStats.ones_to_zeroes, wck_ones);
    EXPECT_EQ(stats.wClockStats.zeroes_to_ones, wck_ones);
}
