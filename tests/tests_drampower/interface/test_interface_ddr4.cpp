#include <gtest/gtest.h>

#include <memory>
#include <fstream>

#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecDDR4.h"
#include "DRAMPower/standards/ddr4/DDR4.h"
#include "DRAMPower/standards/ddr4/interface_calculation_DDR4.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::DDR4;
using DRAMPower::interface_energy_info_t;
using DRAMPower::InterfaceCalculation_DDR4;
using DRAMPower::MemSpecDDR4;
using DRAMPower::SimulationStats;

#define SZ_BITS(x) sizeof(x)*8

// burst length = 8 for x8 devices
static constexpr uint8_t wr_data[] = {
    0, 0, 0, 0,  0, 0, 0, 255,
};

// burst length = 8 for x8 devices
static constexpr uint8_t rd_data[] = {
    0, 0, 0, 0,  0, 0, 0, 1,
};

class DDR4_WindowStats_Tests : public ::testing::Test {
   public:
    DDR4_WindowStats_Tests() {
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
            {8, CmdType::WR, {1, 0, 0, 0, 16}, wr_data, SZ_BITS(wr_data)},
            {16, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::RD, {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)},
            {8, CmdType::RD, {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)},
            {16, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        initSpec();
        ddr = std::make_unique<DDR4>(spec);
    }

    void initSpec() {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "ddr4.json");

        if(!f.is_open()){
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }

        json data = json::parse(f);
        spec = MemSpecDDR4{data["memspec"]};
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCommand(command);
            ddr->handleInterfaceCommand(command);
        }
    }

    std::vector<std::vector<Command>> test_patterns;
    MemSpecDDR4 spec;
    std::unique_ptr<DDR4> ddr;
};

// Tests for the window stats (ones to zeros, ones etc)
// Tests for command bus, data bus and DQs 
TEST_F(DDR4_WindowStats_Tests, Pattern_0) {
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    // Clock
    EXPECT_EQ(stats.clockStats.ones, 24);
    EXPECT_EQ(stats.clockStats.zeroes, 24);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 24);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 24);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 8);
    EXPECT_EQ(stats.writeBus.zeroes, 184);  // 24 (time) * 8 (bus width) - 8 (ones)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 8);  // 0 -> 255 = 8 transitions,
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 8);  // back to 0

    EXPECT_EQ(stats.readBus.ones, 1);
    EXPECT_EQ(stats.readBus.zeroes, 191);  // 24 (time) * 8 (bus width) - 1 (ones)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 1);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 1);

    EXPECT_EQ(stats.commandBus.ones, 14);
    EXPECT_EQ(stats.commandBus.zeroes, 661);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 14);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 14);

    // DQs bus
    // For write and read the number of clock cycles the strobes stay on is
    // ("size in bits" / bus_size) / bus_rate
    int number_of_cycles = (SZ_BITS(wr_data) / 8) / spec.dataRate;

    // In this example read data and write data are the same size, so stats should be the same
    // DQs modelled as single line
    int DQS_ones = number_of_cycles;
    int DQS_zeros = DQS_ones;
    int DQS_zeros_to_ones = DQS_ones;
    int DQS_ones_to_zeros = DQS_zeros;
    EXPECT_EQ(stats.writeDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.writeDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.writeDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.writeDQSStats.zeroes_to_ones, DQS_ones_to_zeros);

    // Read strobe should be the same (only because wr_data is same as rd_data in this test)
    EXPECT_EQ(stats.readDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.readDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes_to_ones, DQS_ones_to_zeros);

    // PrePostamble
    // No seamless preambles or postambles
    auto prepos = stats.rank_total[0].prepos;
    EXPECT_EQ(prepos.readSeamless, 0);
    EXPECT_EQ(prepos.writeSeamless, 0);
    EXPECT_EQ(prepos.readMerged, 0);
    EXPECT_EQ(prepos.readMergedTime, 0);
    EXPECT_EQ(prepos.writeMerged, 0);
    EXPECT_EQ(prepos.writeMergedTime, 0);
}

TEST_F(DDR4_WindowStats_Tests, Pattern_1) {
    runCommands(test_patterns[1]);

    SimulationStats stats = ddr->getStats();

    // Clock
    EXPECT_EQ(stats.clockStats.ones, 24);
    EXPECT_EQ(stats.clockStats.zeroes, 24);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 24);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 24);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 16);
    EXPECT_EQ(stats.writeBus.zeroes, 176);  // 24 (time) * 8 (bus width) - 16 (ones)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 16);  // 0 -> 255 * 2 = 16 transitions,
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 16);  // back to 0

    EXPECT_EQ(stats.readBus.ones, 0);
    EXPECT_EQ(stats.readBus.zeroes, 192);  // 24 (time) * 8 (bus width)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 0);

    EXPECT_EQ(stats.commandBus.ones, 13);
    EXPECT_EQ(stats.commandBus.zeroes, 662);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 13);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 13);

    // DQs bus
    // For write and read the number of clock cycles the strobes stay on is
    // (("number of writes" * "size in bits") / bus_size) / bus_rate
    int number_of_cycles = ((2 * SZ_BITS(wr_data)) / 8) / spec.dataRate;

    // In this example read data and write data are the same size, so stats should be the same
    int DQS_ones = number_of_cycles;
    int DQS_zeros = DQS_ones;
    int DQS_zeros_to_ones = DQS_ones;
    int DQS_ones_to_zeros = DQS_zeros;
    EXPECT_EQ(stats.writeDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.writeDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.writeDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.writeDQSStats.zeroes_to_ones, DQS_ones_to_zeros);

    // Read strobe should be zero (no reads in this test)
    EXPECT_EQ(stats.readDQSStats.ones, 0);
    EXPECT_EQ(stats.readDQSStats.zeroes, 0);
    EXPECT_EQ(stats.readDQSStats.ones_to_zeroes, 0);
    EXPECT_EQ(stats.readDQSStats.zeroes_to_ones, 0);

    // PrePostamble
    auto prepos = stats.rank_total[0].prepos;
    EXPECT_EQ(prepos.readSeamless, 0);
    EXPECT_EQ(prepos.writeSeamless, 1);
    EXPECT_EQ(prepos.readMerged, 0);
    EXPECT_EQ(prepos.readMergedTime, 0);
    EXPECT_EQ(prepos.writeMerged, 0);
    EXPECT_EQ(prepos.writeMergedTime, 0);
}

TEST_F(DDR4_WindowStats_Tests, Pattern_2) {
    runCommands(test_patterns[2]);

    SimulationStats stats = ddr->getStats();

    // Clock
    EXPECT_EQ(stats.clockStats.ones, 24);
    EXPECT_EQ(stats.clockStats.zeroes, 24);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 24);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 24);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 0);
    EXPECT_EQ(stats.writeBus.zeroes, 192);  // 24 (time) * 8 (bus width) - 16 (ones)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 0);

    EXPECT_EQ(stats.readBus.ones, 2);
    EXPECT_EQ(stats.readBus.zeroes, 190);  // 24 (time) * 8 (bus width) - 2 (ones)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 2);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 2);

    EXPECT_EQ(stats.commandBus.ones, 15);
    EXPECT_EQ(stats.commandBus.zeroes, 660);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 15);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 15);

    // DQs bus
    // For write and read the number of clock cycles the strobes stay on is
    // (("number of reads" * "size in bits") / bus_size) / bus_rate
    int number_of_cycles = ((2 * SZ_BITS(rd_data)) / 8) / spec.dataRate;

    // In this example read data and write data are the same size, so stats should be the same
    int DQS_ones = number_of_cycles;
    int DQS_zeros = DQS_ones;
    int DQS_zeros_to_ones = DQS_ones;
    int DQS_ones_to_zeros = DQS_zeros;
    EXPECT_EQ(stats.readDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.readDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes_to_ones, DQS_ones_to_zeros);

    // Write strobe should be zero (no writes in this test)
    EXPECT_EQ(stats.writeDQSStats.ones, 0);
    EXPECT_EQ(stats.writeDQSStats.zeroes, 0);
    EXPECT_EQ(stats.writeDQSStats.ones_to_zeroes, 0);
    EXPECT_EQ(stats.writeDQSStats.zeroes_to_ones, 0);

    // PrePostamble
    auto prepos = stats.rank_total[0].prepos;
    EXPECT_EQ(prepos.readSeamless, 1);
    EXPECT_EQ(prepos.writeSeamless, 0);
    EXPECT_EQ(prepos.readMerged, 0);
    EXPECT_EQ(prepos.readMergedTime, 0);
    EXPECT_EQ(prepos.writeMerged, 0);
    EXPECT_EQ(prepos.writeMergedTime, 0);
}

// Tests for power consumption (given a known SimulationStats)
class DDR4_Energy_Tests : public ::testing::Test {
   public:
    DDR4_Energy_Tests() {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "ddr4.json");

        if (!f.is_open()) {
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }

        json data = json::parse(f);
        spec = MemSpecDDR4{data["memspec"]};

        t_CK = spec.memTimingSpec.tCK;
        voltage = spec.memPowerSpec[MemSpecDDR4::VoltageDomain::VDD].vXX;

        // Change impedances to different values from each other
        spec.memImpedanceSpec.R_eq_cb = 2;
        spec.memImpedanceSpec.R_eq_ck = 3;
        spec.memImpedanceSpec.R_eq_dqs = 4;
        spec.memImpedanceSpec.R_eq_rb = 5;
        spec.memImpedanceSpec.R_eq_wb = 6;

        spec.memImpedanceSpec.C_total_cb = 2;
        spec.memImpedanceSpec.C_total_ck = 3;
        spec.memImpedanceSpec.C_total_dqs = 4;
        spec.memImpedanceSpec.C_total_rb = 5;
        spec.memImpedanceSpec.C_total_wb = 6;

        io_calc = std::make_unique<InterfaceCalculation_DDR4>(spec);
    }

    MemSpecDDR4 spec;
    double t_CK;
    double voltage;
    std::unique_ptr<InterfaceCalculation_DDR4> io_calc;
};

TEST_F(DDR4_Energy_Tests, Parameters) {
    ASSERT_TRUE(t_CK > 0.0);
    ASSERT_TRUE(voltage > 0.0);
}

TEST_F(DDR4_Energy_Tests, Clock_Energy) {
    SimulationStats stats = {0};
    stats.clockStats.ones = 200;
    stats.clockStats.zeroes_to_ones = 200;

    stats.clockStats.zeroes = 400;  // different number to validate termination
    stats.clockStats.ones_to_zeroes = 400;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    // Clock is provided by the controller not the device
    EXPECT_DOUBLE_EQ(result.dram.dynamicPower, 0.0);
    EXPECT_DOUBLE_EQ(result.dram.staticPower, 0.0);

    // DDR5 clock power consumed on 1's
    double expected_static = (2 * stats.clockStats.ones) * voltage * voltage * (0.5 * t_CK) / spec.memImpedanceSpec.R_eq_ck;
    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic = (2 * stats.clockStats.zeroes_to_ones) * 0.5 * spec.memImpedanceSpec.C_total_ck * voltage * voltage;

    EXPECT_DOUBLE_EQ(result.controller.staticPower, expected_static);  // value itself doesn't matter, only that it matches the formula
    EXPECT_DOUBLE_EQ(result.controller.dynamicPower, expected_dynamic);
}