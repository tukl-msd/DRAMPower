#include <gtest/gtest.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/standards/lpddr4/interface_calculation_LPDDR4.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>

#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecLPDDR4.h"
#include "DRAMPower/standards/lpddr4/LPDDR4.h"
#include "DRAMPower/util/extensions.h"


using DRAMPower::interface_energy_info_t;
using DRAMPower::InterfaceCalculation_LPDDR4;
using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::LPDDR4;
using DRAMPower::MemSpecLPDDR4;
using DRAMPower::SimulationStats;

#define SZ_BITS(x) sizeof(x)*8

// burst length = 8 for x8 devices
static constexpr uint8_t wr_data[] = {
    0xF0, 0x0F, 0xE0, 0xF1,  0x00, 0xFF, 0xFF, 0xFF, // inverted to 0xF0,0x0F,0xE0,0x0E, 0x00,0x00,0x00,0x00
    // DBI Line:
    // L // before burst
    // L // burst 1 time = 4, virtual_time = 8 ok
    // L // burst 2 vt = 9 ok
    // L // burst 3 vt = 10 ok
    // H // burst 4 vt = 11 ok
    // L // burst 5 vt = 12 ok
    // H // burst 6 vt = 13 ok
    // H // burst 7 vt = 14 ok
    // H // burst 8 vt = 15 ok
    // L // after burst ok
    // 4 inversions
    // 0x00 to 0xF0; 0 ones to zeroes, 4 zeroes to ones, 0 ones, 8 zeroes // before burst
    // 0xF0 to 0x0F; 4 ones to zeroes, 4 zeroes to ones, 4 ones, 4 zeroes
    // 0x0F to 0xE0; 4 ones to zeroes, 3 zeroes to ones, 4 ones, 4 zeroes
    // 0xE0 to 0x0E; 3 ones to zeroes, 3 zeroes to ones, 3 ones, 5 zeroes
    // 0x0E to 0x00; 3 ones to zeroes, 0 zeroes to ones, 3 ones, 5 zeroes
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes // after burst
    // ones to zeroes: 14, zeroes to ones: 14, ones: 14
};

// burst length = 8 for x8 devices
static constexpr uint8_t rd_data[] = {
    0, 0, 0, 0,  0, 0, 255, 1, // inverted to 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x01
    // DBI Line:
    // L // before burst for test_patterns[0] or test_patterns[1] burst 1
    // L // before burst for test_patterns[1] burst 2
    // L // burst 1 time = 11, virtual_time = 22 ok
    // L // burst 2 vt = 23 ok
    // L // burst 3 vt = 24 ok
    // L // burst 4 vt = 25 ok
    // L // burst 5 vt = 26 ok
    // L // burst 6 vt = 27 ok
    // H // burst 7 vt = 28 ok
    // L // burst 8 vt = 29 ok
    // L // after burst for test_patterns[0]
    // L // after burst for test_patterns[1] burst 2
    // 1 inversions
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes // before burst
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x00 to 0x00; 0 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x00 to 0x01; 1 ones to zeroes, 0 zeroes to ones, 0 ones, 8 zeroes
    // 0x01 to 0x00; 0 ones to zeroes, 1 zeroes to ones, 1 ones, 1 zeroes // after burst
    // ones to zeroes: 1, zeroes to ones: 1, ones 1
};

class LPDDR4_DBI_Tests : public ::testing::Test {
   public:
    LPDDR4_DBI_Tests() {
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 16}, wr_data, SZ_BITS(wr_data)},
            {11, CmdType::RD, {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)},
            {16, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 16}, wr_data, SZ_BITS(wr_data)},
            {11, CmdType::RD, {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)},
            {15, CmdType::RD, {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)}, // Seamless read
            {20, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        initSpec();
        ddr = std::make_unique<LPDDR4>(*spec);
    }

    void initSpec() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr4.json");
        spec = std::make_unique<DRAMPower::MemSpecLPDDR4>(DRAMPower::MemSpecLPDDR4::from_memspec(*data));
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCoreCommand(command);
            ddr->doInterfaceCommand(command);
        }
    }

    std::vector<std::vector<Command>> test_patterns;
    std::unique_ptr<MemSpecLPDDR4> spec;
    std::unique_ptr<LPDDR4> ddr;
};

// Test patterns for stats (counter)
TEST_F(LPDDR4_DBI_Tests, Pattern_0) {
    ddr->getExtensionManager().withExtension<DRAMPower::extensions::DBI>([](DRAMPower::extensions::DBI& dbi) {
        dbi.enable(0, true);
    });
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 14);
    EXPECT_EQ(stats.writeBus.zeroes, 370); // 2 (datarate) * 24 (time) * 8 (bus width) - 14 (ones)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 14);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 14);

    EXPECT_EQ(stats.readBus.ones, 1);
    EXPECT_EQ(stats.readBus.zeroes, 383); // 2 (datarate) * 24 (time) * 8 (bus width) - 1 (ones)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 1);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 1);

    // DBI
    EXPECT_EQ(stats.readDBI.ones, 1);
    EXPECT_EQ(stats.readDBI.zeroes, 48-1);
    EXPECT_EQ(stats.readDBI.ones_to_zeroes, 1);
    EXPECT_EQ(stats.readDBI.zeroes_to_ones, 1);

    EXPECT_EQ(stats.writeDBI.ones, 4);
    EXPECT_EQ(stats.writeDBI.zeroes, 48-4);
    EXPECT_EQ(stats.writeDBI.ones_to_zeroes, 2);
    EXPECT_EQ(stats.writeDBI.zeroes_to_ones, 2);

}

TEST_F(LPDDR4_DBI_Tests, Pattern_1) {
    ddr->getExtensionManager().withExtension<DRAMPower::extensions::DBI>([](DRAMPower::extensions::DBI& dbi) {
        dbi.enable(0, true);
    });
    runCommands(test_patterns[1]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 14);
    EXPECT_EQ(stats.writeBus.zeroes, 370); // 2 (datarate) * 24 (time) * 8 (bus width) - 14 (ones)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 14);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 14);

    EXPECT_EQ(stats.readBus.ones, 2);
    EXPECT_EQ(stats.readBus.zeroes, 382);  // 2 (datarate) * 24 (time) * 8 (bus width) - 2 (ones) 
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 2);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 2);

    // DBI
    EXPECT_EQ(stats.readDBI.ones, 2);
    EXPECT_EQ(stats.readDBI.zeroes, 48-2);
    EXPECT_EQ(stats.readDBI.ones_to_zeroes, 2);
    EXPECT_EQ(stats.readDBI.zeroes_to_ones, 2);

    EXPECT_EQ(stats.writeDBI.ones, 4);
    EXPECT_EQ(stats.writeDBI.zeroes, 48 - 4);
    EXPECT_EQ(stats.writeDBI.ones_to_zeroes, 2);
    EXPECT_EQ(stats.writeDBI.zeroes_to_ones, 2);
}
class LPDDR4_DBI_Energy_Tests : public ::testing::Test {
   public:
    LPDDR4_DBI_Energy_Tests() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr4.json");
        spec = std::make_unique<DRAMPower::MemSpecLPDDR4>(DRAMPower::MemSpecLPDDR4::from_memspec(*data));

        t_CK = spec->memTimingSpec.tCK;
        voltage = spec->vddq;

        // Change impedances to different values from each other
        spec->memImpedanceSpec.rdbi_R_eq = 1;
        spec->memImpedanceSpec.wdbi_R_eq = 2;

        spec->memImpedanceSpec.rdbi_dyn_E = 3;
        spec->memImpedanceSpec.wdbi_dyn_E = 4;

        io_calc = std::make_unique<InterfaceCalculation_LPDDR4>(*spec);
    }

    std::unique_ptr<MemSpecLPDDR4> spec;
    double t_CK;
    double voltage;
    std::unique_ptr<InterfaceCalculation_LPDDR4> io_calc;
};

TEST_F(LPDDR4_DBI_Energy_Tests, Energy) {
    SimulationStats stats;
    stats.readDBI.ones = 1;
    stats.readDBI.zeroes = 2;
    stats.readDBI.ones_to_zeroes = 3;
    stats.readDBI.zeroes_to_ones = 4;

    // Controller -> write power
    // Dram -> read power
    // data rate dbi is 2 -> t_per_bit = 0.5 * t_CK
    double expected_static_controller =
        stats.writeDBI.ones * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.wdbi_R_eq;
    double expected_static_dram =
        stats.readDBI.ones * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.rdbi_R_eq;

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
