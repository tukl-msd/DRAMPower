#include <gtest/gtest.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR4.h>

#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecDDR4.h"
#include "DRAMPower/standards/ddr4/DDR4.h"
#include "DRAMPower/util/extensions.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::DDR4;
using DRAMPower::MemSpecDDR4;
using DRAMPower::SimulationStats;

#define SZ_BITS(x) sizeof(x)*8

// burst length = 8 for x8 devices
static constexpr uint8_t wr_data[] = {
    0xF0, 0x0F, 0xE0, 0xF1,  0x00, 0xFF, 0xFF, 0xFF, // inverted to 0xF0,0x0F,0x1F,0xF1, 0xFF,0xFF,0xFF,0xFF
    // DBI Line:
    // H // before burst
    // H // burst 1 time = 4, virtual_time = 8 ok
    // H // burst 2 vt = 9 ok
    // L // burst 3 vt = 10 ok
    // H // burst 4 vt = 11 ok
    // L // burst 5 vt = 12 ok
    // H // burst 6 vt = 13 ok
    // H // burst 7 vt = 14 ok
    // H // burst 8 vt = 15 ok
    // H // after burst ok
    // 2 inversions
    // 0xFF to 0xF0; 4 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes // before burst
    // 0xF0 to 0x0F; 4 ones to zeroes, 4 zeroes to ones, 4 ones, 4 zeroes
    // 0x0F to 0x1F; 0 ones to zeroes, 1 zeroes to ones, 4 ones, 4 zeroes
    // 0x1F to 0xF1; 3 ones to zeroes, 3 zeroes to ones, 5 ones, 3 zeroes
    // 0xF1 to 0xFF; 0 ones to zeroes, 3 zeroes to ones, 5 ones, 3 zeroes
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes // after burst
    // ones to zeroes: 11, zeroes to ones: 11, zeroes 14
};

// burst length = 8 for x8 devices
static constexpr uint8_t rd_data[] = {
    0, 0, 0, 0,  0, 0, 255, 1, // inverted to 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFE
    // DBI Line:
    // H // before burst
    // L // burst 1 time = 11, virtual_time = 22 ok
    // L // burst 2 vt = 23 ok
    // L // burst 3 vt = 24 ok
    // L // burst 4 vt = 25 ok
    // L // burst 5 vt = 26 ok
    // L // burst 6 vt = 27 ok
    // H // burst 7 vt = 28 ok
    // L // burst 8 vt = 29 ok
    // H // after burst TODO
    // 7 inversions
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes // before burst
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFF to 0xFF; 0 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFF to 0xFE; 1 ones to zeroes, 0 zeroes to ones, 8 ones, 0 zeroes
    // 0xFE to 0xFF; 0 ones to zeroes, 1 zeroes to ones, 8 ones, 1 zeroes // after burst
    // ones to zeroes: 1, zeroes to ones: 1, zeroes 1
};

class DDR4_DBI_Tests : public ::testing::Test {
   public:
    DDR4_DBI_Tests() {
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 16}, wr_data, SZ_BITS(wr_data)},
            {11, CmdType::RD, {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)},
            {16, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        initSpec();
        ddr = std::make_unique<DDR4>(*spec);
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
TEST_F(DDR4_DBI_Tests, Pattern_0) {
    ddr->getExtensionManager().withExtension<DRAMPower::extensions::DBI>([](DRAMPower::extensions::DBI& dbi) {
        dbi.enable(0, true);
    });
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 370); // 2 (datarate) * 24 (time) * 8 (bus width) - 14 (zeroes)
    EXPECT_EQ(stats.writeBus.zeroes, 14);
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 11);  // 0 transitions 1 -> 0
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 11);  // back to 1

    EXPECT_EQ(stats.readBus.ones, 383);  // 2 (datarate) * 24 (time) * 8 (bus width) - 1 (zeroes) 
    EXPECT_EQ(stats.readBus.zeroes, 1);  // 1 (zeroes)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 1); // 1 transition 1 -> 0
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 1); // back to 1

    // DBI
    EXPECT_EQ(stats.readDBI.ones, 48 - 7);
    EXPECT_EQ(stats.readDBI.zeroes, 7);
    EXPECT_EQ(stats.readDBI.ones_to_zeroes, 2);
    EXPECT_EQ(stats.readDBI.zeroes_to_ones, 2);

    EXPECT_EQ(stats.writeDBI.ones, 48 - 2);
    EXPECT_EQ(stats.writeDBI.zeroes, 2);
    EXPECT_EQ(stats.writeDBI.ones_to_zeroes, 2);
    EXPECT_EQ(stats.writeDBI.zeroes_to_ones, 2);

}
