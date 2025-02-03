#include <gtest/gtest.h>

#include <memory>
#include <fstream>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR5.h>
#include <variant>

#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/standards/ddr5/DDR5.h"
#include "DRAMPower/standards/ddr5/interface_calculation_DDR5.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::DDR5;
using DRAMPower::interface_energy_info_t;
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
            {3, CmdType::WR, {1, 0, 0, 0, 4}, wr_data, SZ_BITS(wr_data)},
            {12, CmdType::RD, {1, 0, 0, 0, 4}, rd_data, SZ_BITS(rd_data)},
            {21, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        test_patterns.push_back({
            {0, CmdType::ACT, {2, 0, 0, 372}},
            {3, CmdType::WR, {2, 0, 0, 372, 27}, wr_data, SZ_BITS(wr_data)},
            {12, CmdType::PRE, {2, 0, 0, 372}},
            {15, CmdType::SREFEN},
            {42, CmdType::END_OF_SIMULATION}
        });

        test_patterns.push_back({
            {0, CmdType::ACT, {2, 0, 0, 372}},
            {3, CmdType::RD, {2, 0, 0, 372, 27}, rd_data, SZ_BITS(rd_data)},
            {12, CmdType::WRA, {2, 0, 0, 372, 27}, wr_data, SZ_BITS(wr_data)},
            {28, CmdType::SREFEN},
            {55, CmdType::END_OF_SIMULATION}
        });

        initSpec();
        ddr = std::make_unique<DDR5>(*spec);
    }

    void initSpec() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "ddr5.json");
        spec = std::make_unique<DRAMPower::MemSpecDDR5>(DRAMPower::MemSpecDDR5::from_memspec(*data));

        spec->bitWidth = 16;
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCoreCommand(command);
            ddr->doInterfaceCommand(command);
        }
    }

    std::vector<std::vector<Command>> test_patterns;
    std::unique_ptr<MemSpecDDR5> spec;
    std::unique_ptr<DDR5> ddr;
};

// Tests for the window stats (ones to zeros, ones etc)
// Write and Read bus are trivial
// Command bus needs to be calculated from command patterns
TEST_F(DDR5_WindowStats_Tests, Pattern_0) {
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(stats.writeBus.ones, 528); // 2 (datarate) * 16 (bus width) * 24 (time) - 240 (zeroes)
    EXPECT_EQ(stats.writeBus.zeroes, 240);  // 14 (bursts) * 16 (bus width) + 2 (bursts) * 8 (zeroes in data burst)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 24);  // 16 (first burst) + 8 (data ones to zeroes in bursts)
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 24);  // 8 (data ones to zeroes in bursts) + 8 (last burst data) + 8 (end last burst)

    EXPECT_EQ(stats.readBus.ones, 513); // 2 (datarate) * 16 (bus width) * 24 (time) - 255 (zeroes)
    EXPECT_EQ(stats.readBus.zeroes, 255);  // 15 (bursts) * 16 (bus width) + 15 (zeroes in data burst)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 17); // 16 (first burst) + 1 (data ones to zeroes in bursts)
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 17); // 1 (data ones to zeroes in bursts) + 16 (end last burst)

    // Clock
    EXPECT_EQ(stats.clockStats.ones, 48);
    EXPECT_EQ(stats.clockStats.zeroes, 48);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 48);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 48);

    // Notes
    // Pattern.h: first 4 bits of column (C0-C3) are set to 0 (for reads and writes) // TODO correct???
    EXPECT_EQ(stats.commandBus.ones, 282);
    EXPECT_EQ(stats.commandBus.zeroes, 54);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 39);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 39);

    // In this example read data and write data are the same size, so stats should be the same
    EXPECT_EQ(SZ_BITS(wr_data), SZ_BITS(rd_data));
    EXPECT_EQ(ddr->readBus.get_width(), spec->bitWidth);
    EXPECT_EQ(ddr->writeBus.get_width(), spec->bitWidth);
    uint_fast8_t NumDQsPairs = spec->bitWidth == 16 ? 2 : 1;
    uint64_t number_of_cycles = (SZ_BITS(wr_data) / spec->bitWidth);
    uint_fast8_t scale = NumDQsPairs * 2; // Differential_Pairs * 2(pairs of 2)
    // f(t) = t / 2;
    uint64_t DQS_ones = scale * (number_of_cycles / 2); // scale * (cycles / 2)
    uint64_t DQS_zeros = DQS_ones;
    uint64_t DQS_zeros_to_ones = DQS_ones;
    uint64_t DQS_ones_to_zeros = DQS_zeros;
    EXPECT_EQ(stats.writeDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.writeDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.writeDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.writeDQSStats.zeroes_to_ones, DQS_ones_to_zeros);

    // Read strobe should be the same (only because wr_data is same as rd_data in this test)
    EXPECT_EQ(stats.readDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.readDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes_to_ones, DQS_ones_to_zeros);
}

TEST_F(DDR5_WindowStats_Tests, Pattern_1) {
    runCommands(test_patterns[1]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(stats.writeBus.ones, 1104); // 2 (datarate) * 16 (bus width) * 42 (time) - 240 (zeroes)
    EXPECT_EQ(stats.writeBus.zeroes, 240); // 14 (bursts) * 16 (bus width) + 2 (bursts) * 8 (zeroes in data burst)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 24); // 16 (first burst) + 8 (data ones to zeroes in bursts)
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 24); // 8 (data ones to zeroes in bursts) + 8 (last burst data) + 8 (end last burst)

    EXPECT_EQ(stats.readBus.ones, 1344); // 2 (datarate) * 16 (bus width) * 42 (time)
    EXPECT_EQ(stats.readBus.zeroes, 0);
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 0);

    EXPECT_EQ(stats.commandBus.ones, 551);
    EXPECT_EQ(stats.commandBus.zeroes, 37);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 28);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 28);
}

TEST_F(DDR5_WindowStats_Tests, Pattern_2) {
    runCommands(test_patterns[2]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(stats.writeBus.ones, 1520); // 2 (datarate) * 16 (bus width) * 55 (time) - 240 (zeroes)
    EXPECT_EQ(stats.writeBus.zeroes, 240);  // 14 (bursts) * 16 (bus width) + 2 (bursts) * 8 (zeroes in data burst)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 24);  // 16 (first burst) + 8 (data ones to zeroes in bursts)
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 24);  // 8 (data ones to zeroes in bursts) + 8 (last burst data) + 8 (end last burst)

    EXPECT_EQ(stats.readBus.ones, 1505); // 2 (datarate) * 16 (bus width) * 55 (time) - 255 (zeroes)
    EXPECT_EQ(stats.readBus.zeroes, 255);  // 15 (bursts) * 16 (bus width) + 15 (zeroes in data burst)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 17); // 16 (first burst) + 1 (data ones to zeroes in bursts)
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 17); // 1 (data ones to zeroes in bursts) + 16 (end last burst)

    EXPECT_EQ(stats.commandBus.ones, 724);
    EXPECT_EQ(stats.commandBus.zeroes, 46);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 33);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 33);
}

// Tests for power consumption (given a known SimulationStats)
class DDR5_Energy_Tests : public ::testing::Test {
   public:
    DDR5_Energy_Tests() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "ddr5.json");
        spec = std::make_unique<DRAMPower::MemSpecDDR5>(DRAMPower::MemSpecDDR5::from_memspec(*data));

        t_CK = spec->memTimingSpec.tCK;
        voltage = spec->vddq;

        // Change impedances to different values from each other
        spec->memImpedanceSpec.R_eq_cb = 2;
        spec->memImpedanceSpec.R_eq_ck = 3;
        spec->memImpedanceSpec.R_eq_dqs = 4;
        spec->memImpedanceSpec.R_eq_rb = 5;
        spec->memImpedanceSpec.R_eq_wb = 6;

        spec->memImpedanceSpec.C_total_cb = 2;
        spec->memImpedanceSpec.C_total_ck = 3;
        spec->memImpedanceSpec.C_total_dqs = 4;
        spec->memImpedanceSpec.C_total_rb = 5;
        spec->memImpedanceSpec.C_total_wb = 6;

        io_calc = std::make_unique<InterfaceCalculation_DDR5>(*spec);
    }

    std::unique_ptr<MemSpecDDR5> spec;
    double t_CK;
    double voltage;
    std::unique_ptr<InterfaceCalculation_DDR5> io_calc;
};

TEST_F(DDR5_Energy_Tests, Parameters) {
    ASSERT_TRUE(t_CK > 0.0);
    ASSERT_TRUE(voltage > 0.0);
}

TEST_F(DDR5_Energy_Tests, Clock_Energy) {
    // Note: stats of both clock lines
    SimulationStats stats;
    stats.clockStats.ones = 200;
    stats.clockStats.zeroes_to_ones = 200;

    stats.clockStats.zeroes = 400;  // different number to validate termination
    stats.clockStats.ones_to_zeroes = 400;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    // Clock is provided by the controller not the device
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, 0.0);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, 0.0);

    // DDR5 clock power consumed on 1's
    double expected_static = stats.clockStats.zeroes * voltage * voltage * 0.5 * t_CK / spec->memImpedanceSpec.R_eq_ck;
    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic = stats.clockStats.zeroes_to_ones * 0.5 * spec->memImpedanceSpec.C_total_ck * voltage * voltage;

    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static);  // value itself doesn't matter, only that it matches the formula
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic);
}

TEST_F(DDR5_Energy_Tests, DQS_Energy) {
    SimulationStats stats;
    stats.readDQSStats.ones = 200;
    stats.readDQSStats.zeroes = 400;
    stats.readDQSStats.zeroes_to_ones = 700;
    stats.readDQSStats.ones_to_zeroes = 1000;

    stats.writeDQSStats.ones = 300;
    stats.writeDQSStats.zeroes = 100;
    stats.writeDQSStats.ones_to_zeroes = 2000;
    stats.writeDQSStats.zeroes_to_ones = 999;

    // Term power if consumed by 1's
    // Controller -> write power
    // Dram -> read power
    double expected_static_controller =
        0.5 * stats.writeDQSStats.zeroes * voltage * voltage * t_CK / spec->memImpedanceSpec.R_eq_dqs;
    double expected_static_dram =
        0.5 * stats.readDQSStats.zeroes * voltage * voltage * t_CK / spec->memImpedanceSpec.R_eq_dqs;

    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic_controller = stats.writeDQSStats.zeroes_to_ones *
                                         spec->memImpedanceSpec.C_total_dqs / 2.0 * voltage *
                                         voltage;
    double expected_dynamic_dram = stats.readDQSStats.zeroes_to_ones *
                                   spec->memImpedanceSpec.C_total_dqs / 2.0 * voltage * voltage;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, expected_static_dram);
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, expected_dynamic_dram);
}

TEST_F(DDR5_Energy_Tests, DQ_Energy) {
    SimulationStats stats;
    stats.readBus.ones = 7;
    stats.readBus.zeroes = 11;
    stats.readBus.zeroes_to_ones = 19;
    stats.readBus.ones_to_zeroes = 39;

    stats.writeBus.ones = 43;
    stats.writeBus.zeroes = 59;
    stats.writeBus.zeroes_to_ones = 13;
    stats.writeBus.ones_to_zeroes = 17;

    // Term power if consumed by 0's on DDR 5 (pullup terminated)
    // Controller -> write power
    // Dram -> read power
    double expected_static_controller =
        stats.writeBus.zeroes * voltage * voltage * 0.5 * t_CK / spec->memImpedanceSpec.R_eq_wb;
    double expected_static_dram =
        stats.readBus.zeroes * voltage * voltage * 0.5 * t_CK / spec->memImpedanceSpec.R_eq_rb;

    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic_controller =
        stats.writeBus.zeroes_to_ones * spec->memImpedanceSpec.C_total_wb / 2.0 * voltage * voltage;
    double expected_dynamic_dram =
        stats.readBus.zeroes_to_ones * spec->memImpedanceSpec.C_total_rb / 2.0 * voltage * voltage;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, expected_static_dram);
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, expected_dynamic_dram);
}

TEST_F(DDR5_Energy_Tests, CA_Energy) {
    SimulationStats stats;
    stats.commandBus.ones = 11;
    stats.commandBus.zeroes = 29;
    stats.commandBus.zeroes_to_ones = 39;
    stats.commandBus.ones_to_zeroes = 49;

    double expected_static_controller = stats.commandBus.zeroes * voltage * voltage * t_CK / spec->memImpedanceSpec.R_eq_cb;
    double expected_dynamic_controller = stats.commandBus.zeroes_to_ones * spec->memImpedanceSpec.C_total_cb / 2.0 * voltage * voltage;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);

    // CA bus power is provided by the controller
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, 0.0);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, 0.0);

    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
}
