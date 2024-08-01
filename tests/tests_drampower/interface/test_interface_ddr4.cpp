#include <gtest/gtest.h>

#include <memory>
#include <fstream>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR4.h>
#include <variant>

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

        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 16}, wr_data, SZ_BITS(wr_data)},
            {10, CmdType::WR, {1, 0, 0, 0, 16}, wr_data, SZ_BITS(wr_data)},
            {15, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::RD, {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)},
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
TEST_F(DDR4_WindowStats_Tests, Pattern_0) {
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);

    // Clock
    EXPECT_EQ(stats.clockStats.ones, 48);
    EXPECT_EQ(stats.clockStats.zeroes, 48);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 48);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 48);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 328); // 2 (datarate) * 24 (time) * 8 (bus width) - 56 (zeroes) 
    EXPECT_EQ(stats.writeBus.zeroes, 56);  // 7 (length) * 8 (bus width)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 8);  // 8 transitions 1 -> 0
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 8);  // back to 1

    EXPECT_EQ(stats.readBus.ones, 321);  // 2 (datarate) * 24 (time) * 8 (bus width) - 63 (zeroes) 
    EXPECT_EQ(stats.readBus.zeroes, 63);  // 7 (length zero bursts) * 8 (bus width) + 7 (zeroes in last burst)
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 8);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 8);

    EXPECT_EQ(stats.commandBus.ones, 591);
    EXPECT_EQ(stats.commandBus.zeroes, 57);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 57);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 57);

    // DQs bus
    // For write and read the number of clock cycles the strobes stay on is
    // ("size in bits" / bus_size) / bus_rate
    // read and write have the same length
    // number of cycles per write/read
    int number_of_cycles = (SZ_BITS(wr_data) / 8) / spec->dataRate;

    // In this example read data and write data are the same size, so stats should be the same
    // DQs modelled as single line
    int DQS_ones = number_of_cycles * spec->dataRate;
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

    EXPECT_EQ(spec->dataRate, 2);

    // Clock
    EXPECT_EQ(stats.clockStats.ones, 48);
    EXPECT_EQ(stats.clockStats.zeroes, 48);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 48);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 48);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 272);  // 2 (datarate) * 24 (time) * 8 (bus width) - 112 (zeroes)
    EXPECT_EQ(stats.writeBus.zeroes, 112);  // 7 (zeroe bursts) * 8 (width) * 2 (2 writes)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 16);  // 0 -> 255 * 2 = 16 transitions,
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 16);  // back to 0

    EXPECT_EQ(stats.readBus.ones, 384); // 2 (datarate) * 24 (time) * 8 (bus width)
    EXPECT_EQ(stats.readBus.zeroes, 0);
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 0);

    EXPECT_EQ(stats.commandBus.ones, 593);
    EXPECT_EQ(stats.commandBus.zeroes, 55);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 55);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 55);

    // DQs bus
    // For write and read the number of clock cycles the strobes stay on is
    // (("number of writes/reads" * "size in bits") / bus_size) / bus_rate
    int number_of_cycles = ((2 * SZ_BITS(wr_data)) / 8) / spec->dataRate;

    int DQS_ones = number_of_cycles * spec->dataRate;
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
    EXPECT_EQ(stats.clockStats.ones, 48);
    EXPECT_EQ(stats.clockStats.zeroes, 48);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 48);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 48);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 384); // 2 (datarate) * 24 (time) * 8 (bus width)
    EXPECT_EQ(stats.writeBus.zeroes, 0);
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 0);

    EXPECT_EQ(stats.readBus.ones, 258); // 2 (datarate) * 24 (time) * 8 (bus width) - 126 (zeroes)
    EXPECT_EQ(stats.readBus.zeroes, 126); // 2 (reads) * [ 7 (length zero bursts) * 8 (bus width) + 7 (zeroes in last burst)]
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 9); // 8 (1->0 begin burst) + 1 (1->0 burst to burst data)
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 9); // 8 (0->1 end burst (last burst data included)) + 1 (0->1 burst to burst data)

    EXPECT_EQ(stats.commandBus.ones, 589);
    EXPECT_EQ(stats.commandBus.zeroes, 59);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 59);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 59);

    // DQs bus
    // For write and read the number of clock cycles the strobes stay on is
    // (("number of reads/writes" * "size in bits") / bus_size) / bus_rate
    int number_of_cycles = ((2 * SZ_BITS(rd_data)) / 8) / spec->dataRate;

    // Only read
    int DQS_ones = number_of_cycles * spec->dataRate;
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

TEST_F(DDR4_WindowStats_Tests, Pattern_3) {
    runCommands(test_patterns[3]);

    SimulationStats stats = ddr->getStats();

    // Clock
    EXPECT_EQ(stats.clockStats.ones, 48);
    EXPECT_EQ(stats.clockStats.zeroes, 48);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 48);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 48);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 272); // 2 (datarate) * 24 (time) * 8 (bus width) - 112 (zeroes)
    EXPECT_EQ(stats.writeBus.zeroes, 112);  // 7 (length) * 8 (bus width)
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 16); // 8 (begin first burst) + 8 (end first burst, begin second burst)
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 16); // 8 (end first burst) + 8 (end second burst)

    EXPECT_EQ(stats.readBus.ones, 384); // 2 (datarate) * 24 (time) * 8 (bus width)
    EXPECT_EQ(stats.readBus.zeroes, 0);
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 0);

    EXPECT_EQ(stats.commandBus.ones, 593);
    EXPECT_EQ(stats.commandBus.zeroes, 55);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 55);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 55);

    // DQs bus
    // For write and read the number of clock cycles the strobes stay on is
    // (("number of writes/reads" * "size in bits") / bus_size) / bus_rate
    int number_of_cycles = ((2 * SZ_BITS(wr_data)) / 8) / spec->dataRate;

    // Only writes
    int DQS_ones = number_of_cycles * spec->dataRate;
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
    EXPECT_EQ(prepos.writeSeamless, 0);
    EXPECT_EQ(prepos.readMerged, 0);
    EXPECT_EQ(prepos.readMergedTime, 0);
    EXPECT_EQ(prepos.writeMerged, 0);
    EXPECT_EQ(prepos.writeMergedTime, 0);
}

TEST_F(DDR4_WindowStats_Tests, Pattern_4) {
    runCommands(test_patterns[4]);

    SimulationStats stats = ddr->getStats();

    // Clock
    EXPECT_EQ(stats.clockStats.ones, 48);
    EXPECT_EQ(stats.clockStats.zeroes, 48);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 48);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 48);

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 384); // 2 (datarate) * 24 (time) * 8 (bus width)
    EXPECT_EQ(stats.writeBus.zeroes, 0);
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 0);

    EXPECT_EQ(stats.readBus.ones, 258); // 2 (datarate) * 24 (time) * 8 (bus width) - 126 (zeroes)
    EXPECT_EQ(stats.readBus.zeroes, 126); // 2 (reads) * [ 7 (length zero bursts) * 8 (bus width) + 7 (zeroes in last burst)]
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 16); // 8 (first burst begin) + 8 (second burst begin)
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 16); // 8 (end first burst) + 8 (end second burst)

    EXPECT_EQ(stats.commandBus.ones, 589);
    EXPECT_EQ(stats.commandBus.zeroes, 59);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 59);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 59);

    // DQs bus
    // For write and read the number of clock cycles the strobes stay on is
    // (("number of writes/reads" * "size in bits") / bus_size) / bus_rate
    int number_of_cycles = ((2 * SZ_BITS(wr_data)) / 8) / spec->dataRate;

    // Only reads
    int DQS_ones = number_of_cycles * spec->dataRate;
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
    EXPECT_EQ(prepos.readSeamless, 0);
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
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "ddr4.json");
        spec = std::make_unique<DRAMPower::MemSpecDDR4>(DRAMPower::MemSpecDDR4::from_memspec(*data));

        t_CK = spec->memTimingSpec.tCK;
        voltage = spec->memPowerSpec[MemSpecDDR4::VoltageDomain::VDD].vXX;

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

        // PrePostamble is a possible DDR4 pattern
        // Preamble 2tCK, Postamble 0.5tCK
        spec->prePostamble.read_ones = 2.5;
        spec->prePostamble.read_zeroes = 2.5;
        spec->prePostamble.read_zeroes_to_ones = 2;
        spec->prePostamble.read_ones_to_zeroes = 2;

        // Preamble 1tCK, Postamble 0.5tCK
        spec->prePostamble.write_ones = 1.5;
        spec->prePostamble.write_zeroes = 1.5;
        spec->prePostamble.write_zeroes_to_ones = 2;
        spec->prePostamble.write_ones_to_zeroes = 2;

        spec->prePostamble.readMinTccd = 3;
        spec->prePostamble.writeMinTccd = 2;

        io_calc = std::make_unique<InterfaceCalculation_DDR4>(*spec);
    }

    std::unique_ptr<MemSpecDDR4> spec;
    double t_CK;
    double voltage;
    std::unique_ptr<InterfaceCalculation_DDR4> io_calc;
};

TEST_F(DDR4_Energy_Tests, Parameters) {
    ASSERT_TRUE(t_CK > 0.0);
    ASSERT_TRUE(voltage > 0.0);
}

// Test pattern for energy consumption
TEST_F(DDR4_Energy_Tests, Clock_Energy) {
    SimulationStats stats = {0};
    stats.clockStats.ones = 200;
    stats.clockStats.zeroes_to_ones = 200;

    stats.clockStats.zeroes = 400;  // different number to validate termination
    stats.clockStats.ones_to_zeroes = 400;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    // Clock is provided by the controller not the device
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, 0.0);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, 0.0);

    // Note
    // The clock stats include both lines of the differential pair

    // DDR4 clock power consumed on 0's
    double expected_static = stats.clockStats.zeroes * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_ck;
    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic = stats.clockStats.zeroes_to_ones * 0.5 * spec->memImpedanceSpec.C_total_ck * voltage * voltage;

    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static);  // value itself doesn't matter, only that it matches the formula
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic);
}

TEST_F(DDR4_Energy_Tests, DQS_Energy) {
    SimulationStats stats;
    stats.readDQSStats.ones = 200;
    stats.readDQSStats.zeroes = 400;
    stats.readDQSStats.zeroes_to_ones = 700;
    stats.readDQSStats.ones_to_zeroes = 1000;

    stats.writeDQSStats.ones = 300;
    stats.writeDQSStats.zeroes = 100;
    stats.writeDQSStats.ones_to_zeroes = 2000;
    stats.writeDQSStats.zeroes_to_ones = 999;

    // Note
    // The DQS stats include both lines of the differential pair

    // Controller -> write power
    // Dram -> read power
    // Note dqs is modeled as clock. The clock class incorporates the data rate
    double expected_static_controller = stats.writeDQSStats.zeroes *
                        voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_dqs;
    double expected_static_dram = stats.readDQSStats.zeroes *
                        voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_dqs;

    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic_controller = stats.writeDQSStats.zeroes_to_ones *
                                         0.5 * spec->memImpedanceSpec.C_total_dqs * voltage * voltage;
    double expected_dynamic_dram = stats.readDQSStats.zeroes_to_ones *
                                   0.5 * spec->memImpedanceSpec.C_total_dqs * voltage * voltage;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, expected_static_dram);
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, expected_dynamic_dram);
}

TEST_F(DDR4_Energy_Tests, DQ_Energy) {
    SimulationStats stats;
    stats.readBus.ones = 7;
    stats.readBus.zeroes = 11;
    stats.readBus.zeroes_to_ones = 19;
    stats.readBus.ones_to_zeroes = 39;

    stats.writeBus.ones = 43;
    stats.writeBus.zeroes = 59;
    stats.writeBus.zeroes_to_ones = 13;
    stats.writeBus.ones_to_zeroes = 17;

    // Controller -> write power
    // Dram -> read power
    // zeroes and ones of the data bus are the zeroes and ones per pattern (data rate is not modeled in the bus)
    // data rate data bus is 2 -> t_per_bit = 0.5 * t_CK
    double expected_static_controller =
        stats.writeBus.zeroes * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_wb;
    double expected_static_dram =
        stats.readBus.zeroes * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_rb;

    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic_controller = stats.writeBus.zeroes_to_ones *
                            0.5 * spec->memImpedanceSpec.C_total_wb * voltage * voltage;
    double expected_dynamic_dram = stats.readBus.zeroes_to_ones *
                            0.5 * spec->memImpedanceSpec.C_total_rb * voltage * voltage;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, expected_static_dram);
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, expected_dynamic_dram);
}

TEST_F(DDR4_Energy_Tests, CA_Energy) {
    SimulationStats stats;
    stats.commandBus.ones = 11;
    stats.commandBus.zeroes = 29;
    stats.commandBus.zeroes_to_ones = 39;
    stats.commandBus.ones_to_zeroes = 49;

    double expected_static_controller = stats.commandBus.zeroes * 
                            voltage * voltage * t_CK / spec->memImpedanceSpec.R_eq_cb;
    double expected_dynamic_controller = stats.commandBus.zeroes_to_ones *
                            0.5 * spec->memImpedanceSpec.C_total_cb * voltage * voltage;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);

    // CA bus power is provided by the controller
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, 0.0);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, 0.0);

    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
}

TEST_F(DDR4_Energy_Tests, PrePostamble_Energy) {
    SimulationStats stats;
    stats.readDQSStats.ones = 200;
    stats.readDQSStats.zeroes = 400;
    stats.readDQSStats.zeroes_to_ones = 700;
    stats.readDQSStats.ones_to_zeroes = 1000;

    stats.writeDQSStats.ones = 300;
    stats.writeDQSStats.zeroes = 100;
    stats.writeDQSStats.ones_to_zeroes = 2000;
    stats.writeDQSStats.zeroes_to_ones = 999;

    // DDR4 doesn't support merged preambles or postambles
    stats.rank_total.resize(1);
    stats.rank_total[0].prepos.readMerged = 0;
    stats.rank_total[0].prepos.readMergedTime = 0;
    stats.rank_total[0].prepos.writeMerged = 0;
    stats.rank_total[0].prepos.writeMergedTime = 0;

    // Required reads + readAuto > readSeamless
    // Required writes + writeAuto > writeSeamless
    stats.rank_total[0].prepos.readSeamless = 4;
    stats.rank_total[0].counter.reads = 4;
    stats.rank_total[0].counter.readAuto = 10;
    stats.rank_total[0].prepos.writeSeamless = 5;
    stats.rank_total[0].counter.writes = 6;
    stats.rank_total[0].counter.writeAuto = 11;

    double writecount = stats.rank_total[0].counter.writes + stats.rank_total[0].counter.writeAuto;
    double readcount = stats.rank_total[0].counter.reads + stats.rank_total[0].counter.readAuto;

    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic_controller = stats.writeDQSStats.zeroes_to_ones *
                                         0.5 * spec->memImpedanceSpec.C_total_dqs * voltage * voltage;
    double expected_dynamic_dram = stats.readDQSStats.zeroes_to_ones *
                                   0.5 * spec->memImpedanceSpec.C_total_dqs * voltage * voltage;
       
    // Controller -> write power
    // Dram -> read power
    // Note dqs is modeled as clock. The clock class incorporates the data rate
    double expected_static_controller = stats.writeDQSStats.zeroes *
                        voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_dqs;
    double expected_static_dram = stats.readDQSStats.zeroes *
                        voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_dqs;

    // Note DQS already tested in DDR4_Energy_Tests.DQS_Energy

    // Add seamless preambles and postambles power
    // Note read_zeroes incorporates the data rate
    // Note write_zeroes incorporates the data rate
    expected_static_controller += spec->prePostamble.write_zeroes * (writecount - stats.rank_total[0].prepos.writeSeamless) *
                            voltage * voltage * t_CK / spec->memImpedanceSpec.R_eq_dqs;
    expected_static_dram += spec->prePostamble.read_zeroes * (readcount - stats.rank_total[0].prepos.readSeamless) *
                          voltage * voltage * t_CK / spec->memImpedanceSpec.R_eq_dqs;

    expected_dynamic_controller += spec->prePostamble.write_zeroes_to_ones * (writecount - stats.rank_total[0].prepos.writeSeamless) *
                            0.5 * spec->memImpedanceSpec.C_total_dqs * voltage * voltage;
    expected_dynamic_dram += spec->prePostamble.read_zeroes_to_ones * (readcount - stats.rank_total[0].prepos.readSeamless) *
                            0.5 * spec->memImpedanceSpec.C_total_dqs * voltage * voltage;


    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, expected_static_dram);
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, expected_dynamic_dram);
}