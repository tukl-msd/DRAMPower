#include <gtest/gtest.h>

#include <fstream>
#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR6.h>
#include <variant>

#include "DRAMPower/command/Command.h"
#include "DRAMPower/standards/lpddr6/LPDDR6.h"
#include "DRAMPower/memspec/MemSpecLPDDR6.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::LPDDR6;
using DRAMPower::MemSpecLPDDR6;
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

class LPDDR6_MultirankTests : public ::testing::Test {
  public:
    LPDDR6_MultirankTests() {

        initSpec();
        ddr = std::make_unique<LPDDR6>(*spec);
    }

    void initSpec() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr6.json");
        spec = std::make_unique<DRAMPower::MemSpecLPDDR6>(DRAMPower::MemSpecLPDDR6::from_memspec(*data));
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

    std::unique_ptr<MemSpecLPDDR6> spec;
    std::unique_ptr<LPDDR6> ddr;
};

TEST_F(LPDDR6_MultirankTests, Pattern_1) {
    runCommands({
        {0, CmdType::ACT, {1, 0, 0}},
        {4, CmdType::ACT, {1, 0, 1}},  // rank 1
        {8, CmdType::WR, {1, 0, 0, 0, 4}, wr_data, SZ_BITS(wr_data)},
        {12, CmdType::RD, {1, 0, 1, 0, 4}, rd_data, SZ_BITS(rd_data)},  // rank 1
        {16, CmdType::PRE, {1, 0, 1}},  // rank 1
        {21, CmdType::PRE, {1, 0, 0}},
        {26, CmdType::END_OF_SIMULATION}
    });

    SimulationStats stats = ddr->getStats();
    EXPECT_EQ(stats.bank.size(), spec->numberOfBanks * spec->numberOfRanks);
    // TODO verify
    EXPECT_EQ(stats.bank[bankIndex(1, 0)].cycles.act, 21);
    EXPECT_EQ(stats.bank[bankIndex(1, 1)].cycles.act, 12);
    EXPECT_EQ(stats.bank[bankIndex(1, 0)].cycles.pre, 5);
    EXPECT_EQ(stats.bank[bankIndex(1, 1)].cycles.pre, 14);

    EXPECT_EQ(stats.rank_total[0].cycles.act, 21);
    EXPECT_EQ(stats.rank_total[1].cycles.act, 12);
}

TEST_F(LPDDR6_MultirankTests, Pattern_2) {
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

    SimulationStats stats = ddr->getStats();
    EXPECT_EQ(stats.bank.size(), spec->numberOfBanks * spec->numberOfRanks);
    EXPECT_EQ(stats.bank[bankIndex(0, 0)].cycles.act, 85);
    EXPECT_EQ(stats.bank[bankIndex(0, 1)].cycles.act, 85);
    EXPECT_EQ(stats.bank[bankIndex(3, 1)].cycles.act, 68);

    EXPECT_EQ(stats.bank[bankIndex(0, 0)].cycles.pre, 130 - 85);
    EXPECT_EQ(stats.bank[bankIndex(0, 1)].cycles.pre,  130 - 85);
    EXPECT_EQ(stats.bank[bankIndex(3, 1)].cycles.pre,  130 - 68);

    EXPECT_EQ(stats.rank_total[0].cycles.act, 85);
    EXPECT_EQ(stats.rank_total[1].cycles.act, 85);
}
