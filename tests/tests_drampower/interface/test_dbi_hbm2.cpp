#include <gtest/gtest.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/standards/hbm2/interface_calculation_HBM2.h>
#include <DRAMUtils/memspec/standards/MemSpecHBM2.h>

#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecHBM2.h"
#include "DRAMPower/standards/hbm2/HBM2.h"
#include "DRAMPower/util/extensions.h"


using DRAMPower::interface_energy_info_t;
using DRAMPower::InterfaceCalculation_HBM2;
using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::HBM2;
using DRAMPower::MemSpecHBM2;
using DRAMPower::SimulationStats;

#define SZ_BITS(x) sizeof(x)*8

// burst length = 4 for x64 devices
static constexpr uint8_t wr_data[] = {
    0xF0, 0x0F, 0xE0, 0xF1,  0x00, 0xFF, 0xFF, 0xFF,
    0xF0, 0x0F, 0xE0, 0xF1,  0x00, 0xFF, 0xFF, 0xFF,
    0xF0, 0x0F, 0xE0, 0xF1,  0x00, 0xFF, 0xFF, 0xFF,
    0xF0, 0x0F, 0xE0, 0xF1,  0x00, 0xFF, 0xFF, 0xFF,
};

// burst length = 4 for x64 devices
static constexpr uint8_t rd_data[] = {
    0, 0, 0, 0,  0, 0, 255, 1,
    0, 0, 0, 0,  0, 0, 255, 1,
    0, 0, 0, 0,  0, 0, 255, 1,
    0, 0, 0, 0,  0, 0, 255, 1,
};

/*
bits: 128 * 48 = 6144

Write:
Non inverted:
Beat -1: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111
Beat  0: 11110000 11110000 11100000 11110001 00000000 11111111 11111111 11111111 
Beat  1: 11110000 11110000 11100000 11110001 00000000 11111111 11111111 11111111
Beat  2: 11110000 11110000 11100000 11110001 00000000 11111111 11111111 11111111
Beat  3: 11110000 11110000 11100000 11110001 00000000 11111111 11111111 11111111
Beat  4: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111
Inverted:
Beat -1: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 HHHHHHHH
Beat  0: 11110000 11110000 00011111 11110001 11111111 11111111 11111111 11111111 HHLHLHHH
Beat  1: 11110000 11110000 00011111 11110001 11111111 11111111 11111111 11111111 HHLHLHHH
Beat  2: 11110000 11110000 00011111 11110001 11111111 11111111 11111111 11111111 HHLHLHHH
Beat  3: 11110000 11110000 00011111 11110001 11111111 11111111 11111111 11111111 HHLHLHHH
Beat  4: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 HHHHHHHH
bus: zeroes: 14 * 4 = 56, ones: 48 * 128 - 56 = 6088, zeroes_to_ones: 14, ones_to_zeroes: 14
dbi: zeros: 2 * 4 = 8, ones: 48 * 8 - 28 = 376, zeroes_to_ones: 2, ones_to_zeroes: 2
Read:
Non inverted:
Beat -1: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111
Beat  0: 00000000 00000000 00000000 00000000 00000000 00000000 11111111 00000001
Beat  1: 00000000 00000000 00000000 00000000 00000000 00000000 11111111 00000001
Beat  2: 00000000 00000000 00000000 00000000 00000000 00000000 11111111 00000001
Beat  3: 00000000 00000000 00000000 00000000 00000000 00000000 11111111 00000001
Beat  4: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111
Inverted:
Beat -1: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 HHHHHHHH
Beat  0: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111110 LLLLLLHL
Beat  1: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111110 LLLLLLHL
Beat  2: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111110 LLLLLLHL
Beat  3: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111110 LLLLLLHL
Beat  4: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 HHHHHHHH
bus: zeroes: 1 * 4, ones: 48 * 128 - 4 = 6140, zeroes_to_ones: 1, ones_to_zeroes: 1
dbi: zeros: 7 * 4 = 28, ones: 48 * 8 - 28 = 356, zeroes_to_ones: 7, ones_to_zeroes: 7
*/

class HBM2_DBI_Tests : public ::testing::Test {
   public:
    HBM2_DBI_Tests() {
        test_patterns.push_back({
            {0,  CmdType::ACT, {1, 0, 0, 2, 0,  0, 0}},
            {4,  CmdType::WR,  {1, 0, 0, 0, 16, 0, 0}, wr_data, SZ_BITS(wr_data)},
            {11, CmdType::RD,  {1, 0, 0, 0, 16, 0, 0}, rd_data, SZ_BITS(rd_data)},
            // Non seamless read
            {14, CmdType::RD,  {1, 0, 0, 0, 16, 0, 0}, rd_data, SZ_BITS(rd_data)},
            {20, CmdType::PRE, {1, 0, 0, 2, 0,  0, 0}},
            {24, CmdType::END_OF_SIMULATION},
        });
        test_patterns.push_back({
            {0,  CmdType::ACT, {1, 0, 0, 2}},
            {4,  CmdType::WR,  {1, 0, 0, 0, 16}, wr_data, SZ_BITS(wr_data)},
            {11, CmdType::RD,  {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)},
            // Seamless read
            {13, CmdType::RD,  {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)}, // Seamless read
            {20, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        initSpec();
        ddr = std::make_unique<HBM2>(*spec);
    }

    void initSpec() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "hbm2.json");
        spec = std::make_unique<DRAMPower::MemSpecHBM2>(DRAMPower::MemSpecHBM2::from_memspec(*data));
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCoreCommand(command);
            ddr->doInterfaceCommand(command);
        }
    }

    std::vector<std::vector<Command>> test_patterns;
    std::unique_ptr<MemSpecHBM2> spec;
    std::unique_ptr<HBM2> ddr;
};

// Test patterns for stats (counter)
TEST_F(HBM2_DBI_Tests, Pattern_0) {
    ddr->getExtensionManager().withExtension<DRAMPower::extensions::DBI>([](DRAMPower::extensions::DBI& dbi) {
        dbi.enable(0, true);
    });
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 6144 - 56);
    EXPECT_EQ(stats.writeBus.zeroes, 56);
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 14);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 14);

    EXPECT_EQ(stats.readBus.ones, 6144 - 2 * 4);
    EXPECT_EQ(stats.readBus.zeroes, 2 * 4);
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 2 * 1);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 2 * 1);

    // DBI
    EXPECT_EQ(stats.readDBI.ones, 384 - 2 * 28);
    EXPECT_EQ(stats.readDBI.zeroes, 2 * 28);
    EXPECT_EQ(stats.readDBI.ones_to_zeroes, 2 * 7);
    EXPECT_EQ(stats.readDBI.zeroes_to_ones, 2 * 7);

    EXPECT_EQ(stats.writeDBI.ones, 384 - 8);
    EXPECT_EQ(stats.writeDBI.zeroes, 8);
    EXPECT_EQ(stats.writeDBI.ones_to_zeroes, 2);
    EXPECT_EQ(stats.writeDBI.zeroes_to_ones, 2);
}

TEST_F(HBM2_DBI_Tests, Pattern_1) {
    ddr->getExtensionManager().withExtension<DRAMPower::extensions::DBI>([](DRAMPower::extensions::DBI& dbi) {
        dbi.enable(0, true);
    });
    runCommands(test_patterns[1]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 6144 - 56);
    EXPECT_EQ(stats.writeBus.zeroes, 56);
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 14);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 14);

    EXPECT_EQ(stats.readBus.ones, 6144 - 8);
    EXPECT_EQ(stats.readBus.zeroes, 8);
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 1);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 1);

    // DBI
    EXPECT_EQ(stats.readDBI.ones, 384 - 2 * 28);
    EXPECT_EQ(stats.readDBI.zeroes, 2 * 28);
    EXPECT_EQ(stats.readDBI.ones_to_zeroes, 7);
    EXPECT_EQ(stats.readDBI.zeroes_to_ones, 7);

    EXPECT_EQ(stats.writeDBI.ones, 384 - 8);
    EXPECT_EQ(stats.writeDBI.zeroes, 8);
    EXPECT_EQ(stats.writeDBI.ones_to_zeroes, 2);
    EXPECT_EQ(stats.writeDBI.zeroes_to_ones, 2);
}

class HBM2_DBI_Energy_Tests : public ::testing::Test {
   public:
    HBM2_DBI_Energy_Tests() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "hbm2.json");
        spec = std::make_unique<DRAMPower::MemSpecHBM2>(DRAMPower::MemSpecHBM2::from_memspec(*data));

        t_CK = spec->memTimingSpec.tCK;
        voltage = spec->vddq;

        // Change impedances to different values from each other
        spec->memImpedanceSpec.rdbi_termination = true;
        spec->memImpedanceSpec.wdbi_termination = true;

        spec->memImpedanceSpec.rdbi_R_eq = 1;
        spec->memImpedanceSpec.wdbi_R_eq = 2;

        spec->memImpedanceSpec.rdbi_dyn_E = 3;
        spec->memImpedanceSpec.wdbi_dyn_E = 4;

        io_calc = std::make_unique<InterfaceCalculation_HBM2>(*spec);
    }

    std::unique_ptr<MemSpecHBM2> spec;
    double t_CK;
    double voltage;
    std::unique_ptr<InterfaceCalculation_HBM2> io_calc;
};

TEST_F(HBM2_DBI_Energy_Tests, Energy) {
    SimulationStats stats;
    stats.readDBI.ones = 1;
    stats.readDBI.zeroes = 2;
    stats.readDBI.ones_to_zeroes = 3;
    stats.readDBI.zeroes_to_ones = 4;

    // Controller -> write power
    // Dram -> read power
    // data rate dbi is 2 -> t_per_bit = 0.5 * t_CK
    double expected_static_controller =
        stats.writeDBI.zeroes * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.wdbi_R_eq;
    double expected_static_dram =
        stats.readDBI.zeroes * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.rdbi_R_eq;

    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic_controller = stats.writeDBI.zeroes_to_ones *
                            spec->memImpedanceSpec.wdbi_dyn_E;
    double expected_dynamic_dram = stats.readDBI.zeroes_to_ones *
                            spec->memImpedanceSpec.rdbi_dyn_E;


    // DBI
    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, expected_static_dram);
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, expected_dynamic_dram);
}
