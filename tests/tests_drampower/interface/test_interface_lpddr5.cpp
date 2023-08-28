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

        // With BG != 0 for testing BG mode
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 1, 0, 2}},
            {5, CmdType::WR, {1, 1, 0, 0, 4}, wr_data, SZ_BITS(wr_data)},
            {10, CmdType::RD, {1, 1, 0, 0, 4}, rd_data, SZ_BITS(rd_data)},
            {17, CmdType::PRE, {1, 1, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
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

TEST_F(LPDDR5_WindowStats_Tests, Pattern_3_BG_Mode) {
    spec.bank_arch = MemSpecLPDDR5::BG;
    ddr = std::make_unique<LPDDR5>(spec);

    runCommands(test_patterns[3]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(stats.writeBus.ones, 16);
    EXPECT_EQ(stats.writeBus.zeroes, 368);
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 16);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 16);

    EXPECT_EQ(stats.readBus.ones, 1);
    EXPECT_EQ(stats.readBus.zeroes, 383);
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 1);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 1);

    EXPECT_EQ(stats.commandBus.ones, 22);
    EXPECT_EQ(stats.commandBus.zeroes, 153);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 16);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 16);

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

// Tests for power consumption (given a known SimulationStats)
class LPDDR5_Energy_Tests : public ::testing::Test {
   public:
    LPDDR5_Energy_Tests() {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "lpddr5.json");

        if (!f.is_open()) {
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }

        json data = json::parse(f);
        spec = MemSpecLPDDR5{data["memspec"]};

        t_CK = spec.memTimingSpec.tCK;
        t_WCK = spec.memTimingSpec.tWCK;
        voltage = spec.memPowerSpec[MemSpecLPDDR5::VoltageDomain::VDDQ].vDDX;

        // Change impedances to different values from each other
        spec.memImpedanceSpec.R_eq_cb = 2;
        spec.memImpedanceSpec.R_eq_ck = 3;
        spec.memImpedanceSpec.R_eq_dqs = 4;
        spec.memImpedanceSpec.R_eq_rb = 5;
        spec.memImpedanceSpec.R_eq_wb = 6;
        spec.memImpedanceSpec.R_eq_wck = 7;

        spec.memImpedanceSpec.C_total_cb = 2;
        spec.memImpedanceSpec.C_total_ck = 3;
        spec.memImpedanceSpec.C_total_dqs = 4;
        spec.memImpedanceSpec.C_total_rb = 5;
        spec.memImpedanceSpec.C_total_wb = 6;
        spec.memImpedanceSpec.C_total_wck = 7;

        io_calc = std::make_unique<InterfaceCalculation_LPDDR5>(spec);
    }

    MemSpecLPDDR5 spec;
    double t_CK;
    double t_WCK;
    double voltage;
    std::unique_ptr<InterfaceCalculation_LPDDR5> io_calc;
};

TEST_F(LPDDR5_Energy_Tests, Parameters) {
    ASSERT_TRUE(t_CK > 0.0);
    ASSERT_TRUE(voltage > 0.0);
    ASSERT_TRUE(t_WCK > 0.0);
}

TEST_F(LPDDR5_Energy_Tests, Clock_Energy) {
    SimulationStats stats;
    stats.clockStats.ones = 43;
    stats.clockStats.zeroes_to_ones = 47;
    stats.clockStats.zeroes = 53;
    stats.clockStats.ones_to_zeroes = 59;

    stats.wClockStats.ones = 61;
    stats.wClockStats.zeroes = 67;
    stats.wClockStats.zeroes_to_ones = 71;
    stats.wClockStats.ones_to_zeroes = 73;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    // Clock is provided by the controller not the device
    EXPECT_DOUBLE_EQ(result.dram.dynamicPower, 0.0);
    EXPECT_DOUBLE_EQ(result.dram.staticPower, 0.0);

    // Clock is differential so there is always going to be one signal that consumes power
    // Calculation is done considering number of ones but could also be zeroes, since clock is
    // a symmetrical signal
    double expected_static = stats.clockStats.ones * voltage * voltage * t_CK / spec.memImpedanceSpec.R_eq_ck;
    expected_static += stats.wClockStats.ones * voltage * voltage * t_WCK / spec.memImpedanceSpec.R_eq_wck;

    double expected_dynamic = stats.clockStats.zeroes_to_ones * spec.memImpedanceSpec.C_total_ck * voltage * voltage;
    expected_dynamic += stats.wClockStats.zeroes_to_ones * spec.memImpedanceSpec.C_total_wck * voltage * voltage;

    EXPECT_DOUBLE_EQ(result.controller.staticPower, expected_static);  // value itself doesn't matter, only that it matches the formula
    EXPECT_DOUBLE_EQ(result.controller.dynamicPower, expected_dynamic);
    EXPECT_TRUE(result.controller.staticPower > 0.0);
    EXPECT_TRUE(result.controller.dynamicPower > 0.0);
}

TEST_F(LPDDR5_Energy_Tests, DQS_Energy) {
    SimulationStats stats;
    stats.readDQSStats.ones = 200;
    stats.readDQSStats.zeroes = 400;
    stats.readDQSStats.zeroes_to_ones = 700;
    stats.readDQSStats.ones_to_zeroes = 1000;

    // Write DQS is not used in LPDDR5, instead there is WCK
    stats.writeDQSStats.ones = 300;
    stats.writeDQSStats.zeroes = 100;
    stats.writeDQSStats.ones_to_zeroes = 2000;
    stats.writeDQSStats.zeroes_to_ones = 999;

    // Term power if consumed by 1's
    // Controller -> write power
    // Dram -> read power
    double expected_static_controller = 0.0;
    double expected_static_dram =
        0.5 * stats.readDQSStats.ones * voltage * voltage * t_CK / spec.memImpedanceSpec.R_eq_dqs;

    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic_controller = 0;
    double expected_dynamic_dram = stats.readDQSStats.zeroes_to_ones *
                                   spec.memImpedanceSpec.C_total_dqs / 2.0 * voltage * voltage;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    EXPECT_DOUBLE_EQ(result.controller.staticPower, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicPower, expected_dynamic_controller);
    EXPECT_DOUBLE_EQ(result.dram.staticPower, expected_static_dram);
    EXPECT_DOUBLE_EQ(result.dram.dynamicPower, expected_dynamic_dram);

    EXPECT_TRUE(result.dram.staticPower > 0.0);
    EXPECT_TRUE(result.dram.dynamicPower > 0.0);
}

TEST_F(LPDDR5_Energy_Tests, DQ_Energy) {
    SimulationStats stats;
    stats.readBus.ones = 7;
    stats.readBus.zeroes = 11;
    stats.readBus.zeroes_to_ones = 19;
    stats.readBus.ones_to_zeroes = 39;

    stats.writeBus.ones = 43;
    stats.writeBus.zeroes = 59;
    stats.writeBus.zeroes_to_ones = 13;
    stats.writeBus.ones_to_zeroes = 17;

    // Term power if consumed by 1's on LPDDR5 (pulldown terminated)
    // Controller -> write power
    // Dram -> read power
    double expected_static_controller =
        0.5 * stats.writeBus.ones * voltage * voltage * t_CK / spec.memImpedanceSpec.R_eq_wb;
    double expected_static_dram =
        0.5 * stats.readBus.ones * voltage * voltage * t_CK / spec.memImpedanceSpec.R_eq_rb;

    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic_controller =
        stats.writeBus.zeroes_to_ones * spec.memImpedanceSpec.C_total_wb / 2.0 * voltage * voltage;
    double expected_dynamic_dram =
        stats.readBus.zeroes_to_ones * spec.memImpedanceSpec.C_total_rb / 2.0 * voltage * voltage;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    EXPECT_DOUBLE_EQ(result.controller.staticPower, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicPower, expected_dynamic_controller);
    EXPECT_DOUBLE_EQ(result.dram.staticPower, expected_static_dram);
    EXPECT_DOUBLE_EQ(result.dram.dynamicPower, expected_dynamic_dram);

    EXPECT_TRUE(result.controller.staticPower > 0.0);
    EXPECT_TRUE(result.controller.dynamicPower > 0.0);
    EXPECT_TRUE(result.dram.staticPower > 0.0);
    EXPECT_TRUE(result.dram.dynamicPower > 0.0);
}

TEST_F(LPDDR5_Energy_Tests, CA_Energy) {
    SimulationStats stats;
    stats.commandBus.ones = 11;
    stats.commandBus.zeroes = 29;
    stats.commandBus.zeroes_to_ones = 39;
    stats.commandBus.ones_to_zeroes = 49;

    double expected_static_controller = 0.5 * stats.commandBus.zeroes * voltage * voltage * t_CK / spec.memImpedanceSpec.R_eq_cb;
    double expected_dynamic_controller = stats.commandBus.zeroes_to_ones * spec.memImpedanceSpec.C_total_cb / 2.0 * voltage * voltage;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);

    // CA bus power is provided by the controller
    EXPECT_DOUBLE_EQ(result.dram.dynamicPower, 0.0);
    EXPECT_DOUBLE_EQ(result.dram.staticPower, 0.0);

    EXPECT_DOUBLE_EQ(result.controller.staticPower, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicPower, expected_dynamic_controller);

    EXPECT_TRUE(result.controller.staticPower > 0.0);
    EXPECT_TRUE(result.controller.dynamicPower > 0.0);
}
