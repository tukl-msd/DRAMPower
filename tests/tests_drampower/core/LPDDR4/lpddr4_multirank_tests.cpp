#include <gtest/gtest.h>

#include <fstream>
#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>
#include <variant>

#include "DRAMPower/command/Command.h"
#include "DRAMPower/standards/lpddr4/LPDDR4.h"
#include "DRAMPower/memspec/MemSpecLPDDR4.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::LPDDR4;
using DRAMPower::MemSpecLPDDR4;
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

class LPDDR4_MultirankTests : public ::testing::Test {
  public:
    LPDDR4_MultirankTests() {

        initSpec();
        ddr = std::make_unique<LPDDR4>(*spec);
    }

    void initSpec() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr4.json");
        spec = std::make_unique<DRAMPower::MemSpecLPDDR4>(DRAMPower::MemSpecLPDDR4::from_memspec(*data));

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

    std::unique_ptr<MemSpecLPDDR4> spec;
    std::unique_ptr<LPDDR4> ddr;
};

TEST_F(LPDDR4_MultirankTests, Pattern_1) {
    runCommands({
        {0, CmdType::ACT, {1, 0, 0}},  // rank 0
        {4, CmdType::ACT, {1, 0, 1}},  // rank 1
        {8, CmdType::WR, {1, 0, 0, 0, 4}, wr_data, SZ_BITS(wr_data)},   // rank 0
        {24, CmdType::RD, {1, 0, 1, 0, 4}, rd_data, SZ_BITS(rd_data)},  // rank 1
        {38, CmdType::PRE, {1, 0, 1}},  // rank 1
        {40, CmdType::PRE, {1, 0, 0}},  // rank 0
        {45, CmdType::END_OF_SIMULATION}
    });
    // Act count for rank 0: act = 40 - 0 = 40
    // Act count for rank 1: act = 38 - 4 = 34
    // Pre count for rank 0: pre = 45 - 40 = 5
    // Pre count for rank 1: pre = 45 - 34 = 11

    SimulationStats stats = ddr->getStats();
    EXPECT_EQ(stats.bank.size(), spec->numberOfBanks * spec->numberOfRanks);
    EXPECT_EQ(stats.bank[bankIndex(1, 0)].cycles.act, 40);
    EXPECT_EQ(stats.bank[bankIndex(1, 1)].cycles.act, 34);
    EXPECT_EQ(stats.bank[bankIndex(1, 0)].cycles.pre, 5);
    EXPECT_EQ(stats.bank[bankIndex(1, 1)].cycles.pre, 11);

    EXPECT_EQ(stats.rank_total[0].cycles.act, 40);
    EXPECT_EQ(stats.rank_total[1].cycles.act, 34);
}

TEST_F(LPDDR4_MultirankTests, Pattern_2) {
    runCommands({
        {0, CmdType::ACT,   TargetCoordinate{0, 0, 0}}, // r0
        {5, CmdType::ACT,   TargetCoordinate{0, 0, 1}}, // r1
        {22, CmdType::ACT,  TargetCoordinate{3, 0, 1}}, // r1
        {35, CmdType::RD,   TargetCoordinate{3, 0, 1, 0, 0}, rd_data, SZ_BITS(rd_data)}, // r1
        {55, CmdType::RD,   TargetCoordinate{0, 0, 0, 0, 3}, rd_data, SZ_BITS(rd_data)}, // r0
        {60, CmdType::PREA, TargetCoordinate{0, 0, 0}}, // r0
        {65, CmdType::PREA, TargetCoordinate{0, 0, 1}}, // r1
        {75, CmdType::REFA, TargetCoordinate{0, 0, 0}}, // r0
        {80, CmdType::REFA, TargetCoordinate{0, 0, 1}}, // r1
        {130, CmdType::END_OF_SIMULATION},
    });

    // t = 0 ACT               -> Start activate // r0, b0
    // t = 5 ACT               -> Start activate // r1, b0
    // t = 22 ACT              -> Start activate // r1, b3
    // t = 35 RD               -> Read // r1, b3
    // t = 55 RD               -> Read // r0, b0
    // t = 60 PREA             -> End activate // r0, b0
    // t = 65 PREA             -> End activate // r1, b0, b3
    // t = 75 REFA             -> Start activate // r0, b0
    // t = 80 REFA             -> Start activate // r1, b0, b3
    // t = 100 REFA (implicit) -> End activate // r0, b0
    // t = 105 REFA (implicit) -> End activate // r1, b0, b3
    // t = 130 END_OF_SIMULATION

    SimulationStats stats = ddr->getStats();
    EXPECT_EQ(stats.bank.size(), spec->numberOfBanks * spec->numberOfRanks);
    EXPECT_EQ(stats.bank[bankIndex(0, 0)].cycles.act, 85);
    EXPECT_EQ(stats.bank[bankIndex(0, 1)].cycles.act, 85);
    EXPECT_EQ(stats.bank[bankIndex(3, 1)].cycles.act, 68);

    EXPECT_EQ(stats.bank[bankIndex(0, 0)].cycles.pre, 130 - 85);
    EXPECT_EQ(stats.bank[bankIndex(0, 1)].cycles.pre, 130 - 85);
    EXPECT_EQ(stats.bank[bankIndex(3, 1)].cycles.pre,  130 - 68);

    EXPECT_EQ(stats.rank_total[0].cycles.act, 85);
    EXPECT_EQ(stats.rank_total[1].cycles.act, 85);
}
