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
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "ddr5.json");

        if(!f.is_open()){
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }
        json data = json::parse(f);
        DRAMPower::MemSpecContainer memspeccontainer = data;
        
        spec = std::make_unique<MemSpecDDR5>(std::get<DRAMUtils::Config::MemSpecDDR5>(memspeccontainer.memspec.getVariant()));
        spec->numberOfRanks = 2;
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCommand(command);
            ddr->handleInterfaceCommand(command);
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
    runCommands({
        {0, CmdType::ACT, {0, 0, 0}},
        {5, CmdType::ACT, {0, 0, 1}},  // r1
        {15, CmdType::RDA, {0, 0, 0, 0, 4}, rd_data, SZ_BITS(rd_data)},
        {20, CmdType::ACT, {3, 0, 1}},  // r1
        {35, CmdType::RD, {3, 0, 1, 0, 0}, rd_data, SZ_BITS(rd_data)},  // r1
        {40, CmdType::RD, {0, 0, 0, 0, 3}, rd_data, SZ_BITS(rd_data)},
        {50, CmdType::PREA, {0, 0, 0}},
        {55, CmdType::PREA, {0, 0, 1}},  // r1
        {65, CmdType::REFA, {0, 0, 0}},
        {70, CmdType::REFA, {0, 0, 1}},  // r1
        {100, CmdType::END_OF_SIMULATION},
    });

    SimulationStats stats = ddr->getStats();
    EXPECT_EQ(stats.bank.size(), spec->numberOfBanks * spec->numberOfRanks);
    EXPECT_EQ(stats.bank[bankIndex(0, 0)].cycles.act, 45);
    EXPECT_EQ(stats.bank[bankIndex(0, 1)].cycles.act, 75);
    EXPECT_EQ(stats.bank[bankIndex(3, 1)].cycles.act, 60);

    EXPECT_EQ(stats.bank[bankIndex(0, 0)].cycles.pre, 55);
    EXPECT_EQ(stats.bank[bankIndex(0, 1)].cycles.pre, 25);
    EXPECT_EQ(stats.bank[bankIndex(3, 1)].cycles.pre, 40);

    EXPECT_EQ(stats.rank_total[0].cycles.act, 45);
    EXPECT_EQ(stats.rank_total[1].cycles.act, 75);
}
