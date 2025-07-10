#include <gtest/gtest.h>

#include <fstream>
#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR5.h>
#include <variant>

#include "DRAMPower/command/Command.h"
#include "DRAMPower/standards/ddr5/DDR5.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::DDR5;
using DRAMPower::MemSpecDDR5;
using DRAMPower::SimulationStats;
using DRAMPower::TargetCoordinate;

#define SZ_BITS(x) sizeof(x)*8

static constexpr uint8_t wr_data[] = {
    0, 0, 0, 0,  0, 0, 0, 255,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 255,
};

static constexpr uint8_t rd_data[] = {
    0, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
};

class DDR5_MultirankTests : public ::testing::Test {
  public:
    void SetUp()
    {
        initSpec();
        ddr = std::make_unique<DDR5>(*spec);
    }

    void initSpec() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "ddr5.json");
        spec = std::make_unique<DRAMPower::MemSpecDDR5>(DRAMPower::MemSpecDDR5::from_memspec(*data));

        spec->numberOfRanks = 2;
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCoreCommand(command);
            ddr->doInterfaceCommand(command);
        }
    }

    inline size_t bankIndex(int bank, int rank) {
        return rank * spec->numberOfBanks + bank;
    }

    std::unique_ptr<MemSpecDDR5> spec;
    std::unique_ptr<DDR5> ddr;
};

TEST_F(DDR5_MultirankTests, Pattern_1) {
    runCommands({
        {0, CmdType::ACT, {1, 0, 0}},
        {2, CmdType::ACT, {1, 0, 1}},  // rank 1
        {4, CmdType::WR, {1, 0, 0, 0, 4}, wr_data, SZ_BITS(wr_data)},
        {6, CmdType::RD, {1, 0, 1, 0, 4}, rd_data, SZ_BITS(rd_data)},  // rank 1
        {10, CmdType::PRE, {1, 0, 1}},  // rank 1
        {15, CmdType::PRE, {1, 0, 0}},
        {20, CmdType::END_OF_SIMULATION}
    });

    SimulationStats stats = ddr->getStats();
    EXPECT_EQ(stats.bank.size(), spec->numberOfBanks * spec->numberOfRanks);
    EXPECT_EQ(stats.bank[bankIndex(1, 0)].cycles.act, 15);
    EXPECT_EQ(stats.bank[bankIndex(1, 1)].cycles.act, 8);
    EXPECT_EQ(stats.bank[bankIndex(1, 0)].cycles.pre, 5);
    EXPECT_EQ(stats.bank[bankIndex(1, 1)].cycles.pre, 12);

    EXPECT_EQ(stats.rank_total[0].cycles.act, 15);
    EXPECT_EQ(stats.rank_total[1].cycles.act, 8);
}

TEST_F(DDR5_MultirankTests, Pattern_2) {
    runCommands({ // TODO invalid state traversal
        {0, CmdType::ACT,   TargetCoordinate{0, 0, 0}}, // r0
        {5, CmdType::ACT,   TargetCoordinate{0, 0, 1}}, // r1
        {15, CmdType::RDA,  TargetCoordinate{0, 0, 0, 0, 4}, rd_data, SZ_BITS(rd_data)}, // r0
        {22, CmdType::ACT,  TargetCoordinate{3, 0, 0}},  // r0
        {35, CmdType::RD,   TargetCoordinate{0, 0, 1, 0, 0}, rd_data, SZ_BITS(rd_data)}, // r1
        {55, CmdType::RD,   TargetCoordinate{3, 0, 0, 0, 3}, rd_data, SZ_BITS(rd_data)}, // r0
        {60, CmdType::PREA, TargetCoordinate{0, 0, 0}}, // r0
        {65, CmdType::PREA, TargetCoordinate{0, 0, 1}}, // r1
        {75, CmdType::REFA, TargetCoordinate{0, 0, 0}}, // r0
        {80, CmdType::REFA, TargetCoordinate{0, 0, 1}}, // r1
        {130, CmdType::END_OF_SIMULATION},
    });

    // t = 0 ACT               -> Start activate // r0, b0
    // t = 5 ACT               -> Start activate // r1, b0
    // t = 15 RDA              -> delayed pre after 5 cycles // r0, b0
    // t = 20 RDA (implicit)   -> End activate // r0, b0
    // t = 22 ACT              -> Start activate // r0, b3
    // t = 35 RD               -> Read // r1, b0
    // t = 55 RD               -> Read // r0, b3
    // t = 60 PREA             -> End activate // r0, b0, b3
    // t = 65 PREA             -> End activate // r1, b0
    // t = 75 REFA             -> Start activate // r0, b0, b3
    // t = 80 REFA             -> Start activate // r1, b0
    // t = 100 REFA (implicit) -> End activate // r0, b0, b3
    // t = 105 REFA (implicit) -> End activate // r1, b0
    // t = 130 END_OF_SIMULATION

    SimulationStats stats = ddr->getStats();
    EXPECT_EQ(stats.bank.size(), spec->numberOfBanks * spec->numberOfRanks);
    EXPECT_EQ(stats.bank[bankIndex(0, 0)].cycles.act, 45);
    EXPECT_EQ(stats.bank[bankIndex(3, 0)].cycles.act, 63);
    EXPECT_EQ(stats.bank[bankIndex(0, 1)].cycles.act, 85);

    EXPECT_EQ(stats.bank[bankIndex(0, 0)].cycles.pre, 130 - 45);
    EXPECT_EQ(stats.bank[bankIndex(3, 0)].cycles.pre, 130 - 63);
    EXPECT_EQ(stats.bank[bankIndex(0, 1)].cycles.pre, 130 - 85);

    EXPECT_EQ(stats.rank_total[0].cycles.act, 83);
    EXPECT_EQ(stats.rank_total[1].cycles.act, 85);
}
