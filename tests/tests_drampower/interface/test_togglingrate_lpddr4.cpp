#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include <cmath>

#include "DRAMPower/standards/lpddr4/LPDDR4.h"
#include "DRAMPower/memspec/MemSpecLPDDR4.h"
#include "DRAMPower/standards/lpddr4/interface_calculation_LPDDR4.h"

#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"

using namespace DRAMPower;

#define SZ_BITS(x) (x)*8

class LPDDR4_TogglingRate_Tests : public ::testing::Test {
   public:
    LPDDR4_TogglingRate_Tests() {
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {5, CmdType::WR, {1, 0, 0, 0, 16}, nullptr, datasize_bits},
            {14, CmdType::RD, {1, 0, 0, 0, 16}, nullptr, datasize_bits},
            {23, CmdType::PRE, {1, 0, 0, 2}},
            {26, CmdType::END_OF_SIMULATION},
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
    uint_fast16_t datasize_bits = 32 * 8; // 32 bytes
};

TEST_F(LPDDR4_TogglingRate_Tests, Pattern_0_LH) {
    // Setup toggling rate
    double togglingRateRead = 0.7;
    double togglingRateWrite = 0.3;
    double dutyCycleRead = 0.6;
    double dutyCycleWrite = 0.4;
    TogglingRateIdlePattern idlePatternRead = TogglingRateIdlePattern::L;
    TogglingRateIdlePattern idlePatternWrite = TogglingRateIdlePattern::H;
    ddr->setToggleRate(ToggleRateDefinition {
        togglingRateRead, // togglingRateRead
        togglingRateWrite, // togglingRateWrite
        dutyCycleRead, // dutyCycleRead
        dutyCycleWrite, // dutyCycleWrite
        idlePatternRead, // idlePatternRead
        idlePatternWrite  // idlePatternWrite
    });
    // Run commands
    runCommands(test_patterns.at(0));
    // SZ_BITS: 256, width: 8 -> Burstlength: 32 (datarate bus)
    // 0: ACT, 5: WR, 14: RD, 23: PRE, 26: EOS
    // Read bus: idle: L
        // 0 to 14 idle
        // 14 to 26 toggle -> toggle cut off by EOS
        // idle: 14 zeroes, toggle: 12 (datarate clock)
        // idle: 28 zeroes, toggle: 24 (datarate bus)
    // Write bus: idle: H
        // 0 to 5 idle
        // 5 to 21 toggle
        // 21 to 26 idle
        // idle: 10 ones, toggle: 16 (datarate clock)
        // idle: 20 ones, toggle: 32 (datarate bus)
    uint64_t toggles_read = 24;
    uint64_t toggles_write = 32;

    uint64_t idleread_ones = 0;
    uint64_t idleread_zeroes = 28;
    uint64_t idlewrite_ones = 20;
    uint64_t idlewrite_zeroes = 0;


    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);
    EXPECT_EQ(ddr->readBus.get_width(), spec->bitWidth);
    EXPECT_EQ(ddr->writeBus.get_width(), spec->bitWidth);

    // Toggling rate in stats
    EXPECT_TRUE(stats.togglingStats);

// Data bus
    // Read bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->read.ones, (idleread_ones +  static_cast<uint64_t>(std::floor(dutyCycleRead * toggles_read))) * spec->bitWidth); // 56
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->read.zeroes, (idleread_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleRead) * toggles_read))) * spec->bitWidth); // 128
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.ones_to_zeroes, std::floor((togglingRateRead / 2) * toggles_read) * spec->bitWidth); // 32
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateRead / 2) * toggles_read)) * spec->bitWidth); // 32
    
    // Write bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.ones, (idlewrite_ones +  static_cast<uint64_t>(std::floor(dutyCycleWrite * toggles_write))) * spec->bitWidth); // 88
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.zeroes, (idlewrite_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleWrite) * toggles_write))) * spec->bitWidth); // 96
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.ones_to_zeroes, std::floor((togglingRateWrite / 2) * toggles_write) * spec->bitWidth); // 24
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateWrite / 2) * toggles_write)) * spec->bitWidth); // 24

// Clock (see test_interface_lpddr4)
    EXPECT_EQ(stats.clockStats.ones, 52);
    EXPECT_EQ(stats.clockStats.zeroes, 52);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 52);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 52);

// Command bus (see test_interface_lpddr4)
    ASSERT_EQ(stats.commandBus.ones, 15);
    ASSERT_EQ(stats.commandBus.zeroes, 141);
    ASSERT_EQ(stats.commandBus.ones_to_zeroes, 14);
    ASSERT_EQ(stats.commandBus.zeroes_to_ones, 14);

// DQs (see test_interface_lpddr4)
    int number_of_cycles = (datasize_bits / spec->bitWidth);
    uint_fast8_t scale = 1 * 2; // Differential_Pairs * 2(pairs of 2)
    // f(t) = t / 2;
    int DQS_ones = scale * (number_of_cycles / 2); // scale * (cycles / 2
    int DQS_zeros = DQS_ones;
    int DQS_zeros_to_ones = DQS_ones;
    int DQS_ones_to_zeros = DQS_zeros;
    EXPECT_EQ(stats.writeDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.writeDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.writeDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.writeDQSStats.zeroes_to_ones, DQS_ones_to_zeros);
    EXPECT_EQ(stats.readDQSStats.ones, DQS_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes, DQS_zeros);
    EXPECT_EQ(stats.readDQSStats.ones_to_zeroes, DQS_zeros_to_ones);
    EXPECT_EQ(stats.readDQSStats.zeroes_to_ones, DQS_ones_to_zeros);

// PrePostamble
    auto prepos = stats.rank_total[0].prepos;
    EXPECT_EQ(prepos.readSeamless, 0);
    EXPECT_EQ(prepos.writeSeamless, 0);
    EXPECT_EQ(prepos.readMerged, 0);
    EXPECT_EQ(prepos.readMergedTime, 0);
    EXPECT_EQ(prepos.writeMerged, 0);
    EXPECT_EQ(prepos.writeMergedTime, 0);
}

TEST_F(LPDDR4_TogglingRate_Tests, Pattern_0_HZ) {
    // Setup toggling rate
    double togglingRateRead = 0.7;
    double togglingRateWrite = 0.3;
    double dutyCycleRead = 0.6;
    double dutyCycleWrite = 0.4;
    TogglingRateIdlePattern idlePatternRead = TogglingRateIdlePattern::H;
    TogglingRateIdlePattern idlePatternWrite = TogglingRateIdlePattern::Z;
    ddr->setToggleRate(ToggleRateDefinition {
        togglingRateRead, // togglingRateRead
        togglingRateWrite, // togglingRateWrite
        dutyCycleRead, // dutyCycleRead
        dutyCycleWrite, // dutyCycleWrite
        idlePatternRead, // idlePatternRead
        idlePatternWrite  // idlePatternWrite
    });
    // Run commands
    runCommands(test_patterns[0]);
    // SZ_BITS: 256, width: 8 -> Burstlength: 32 (datarate bus)
    // 0: ACT, 5: WR, 14: RD, 23: PRE, 26: EOS
    // Read bus: idle: H
        // 0 to 14 idle
        // 14 to 26 toggle -> toggle cut off by EOS
        // idle: 14 ones, toggle: 12 (datarate clock)
        // idle: 28 ones, toggle: 24 (datarate bus)
    // Write bus: idle: H
        // 0 to 5 idle
        // 5 to 21 toggle
        // 21 to 26 idle
        // idle: 0 ones/zeroes, toggle: 16 (datarate clock)
        // idle: 0 ones/zeroes, toggle: 32 (datarate bus)
    uint64_t toggles_read = 24;
    uint64_t toggles_write = 32;

    uint64_t idleread_ones = 28;
    uint64_t idleread_zeroes = 0;
    uint64_t idlewrite_ones = 0;
    uint64_t idlewrite_zeroes = 0;

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);

    // Toggling rate in stats
    EXPECT_TRUE(stats.togglingStats);

// Data bus
    // Read bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->read.ones, (idleread_ones +  static_cast<uint64_t>(std::floor(dutyCycleRead * toggles_read))) * spec->bitWidth); // 56
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->read.zeroes, (idleread_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleRead) * toggles_read))) * spec->bitWidth); // 128
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.ones_to_zeroes, std::floor((togglingRateRead / 2) * toggles_read) * spec->bitWidth); // 32
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateRead / 2) * toggles_read)) * spec->bitWidth); // 32
    
    // Write bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.ones, (idlewrite_ones +  static_cast<uint64_t>(std::floor(dutyCycleWrite * toggles_write))) * spec->bitWidth); // 88
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.zeroes, (idlewrite_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleWrite) * toggles_write))) * spec->bitWidth); // 96
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.ones_to_zeroes, std::floor((togglingRateWrite / 2) * toggles_write) * spec->bitWidth); // 24
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateWrite / 2) * toggles_write)) * spec->bitWidth); // 24
}

// Tests for power consumption (given a known SimulationStats)
class LPDDR4_TogglingRateEnergy_Tests : public ::testing::Test {
   public:
    LPDDR4_TogglingRateEnergy_Tests() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr4.json");
        spec = std::make_unique<DRAMPower::MemSpecLPDDR4>(DRAMPower::MemSpecLPDDR4::from_memspec(*data));

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

        io_calc = std::make_unique<InterfacePowerCalculation_LPPDR4>(*spec);
    }

    std::unique_ptr<MemSpecLPDDR4> spec;
    double t_CK;
    double voltage;
    std::unique_ptr<InterfacePowerCalculation_LPPDR4> io_calc;
};

TEST_F(LPDDR4_TogglingRateEnergy_Tests, DQ_Energy) {
    SimulationStats stats;
    stats.togglingStats = TogglingStats();
    stats.togglingStats->read.ones = 7;
    stats.togglingStats->read.zeroes = 11;
    stats.togglingStats->read.zeroes_to_ones = 19;
    stats.togglingStats->read.ones_to_zeroes = 39;

    stats.togglingStats->write.ones = 43;
    stats.togglingStats->write.zeroes = 59;
    stats.togglingStats->write.zeroes_to_ones = 13;
    stats.togglingStats->write.ones_to_zeroes = 17;

    // Controller -> write power
    // Dram -> read power
    double expected_static_controller =
        stats.togglingStats->write.ones * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_wb;
    double expected_static_dram =
        stats.togglingStats->read.ones * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_rb;

    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic_controller = stats.togglingStats->write.zeroes_to_ones *
                            0.5 * spec->memImpedanceSpec.C_total_wb * voltage * voltage;
    double expected_dynamic_dram = stats.togglingStats->read.zeroes_to_ones *
                            0.5 * spec->memImpedanceSpec.C_total_rb * voltage * voltage;

    interface_energy_info_t result = io_calc->calcEnergy(stats);
    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, expected_static_dram);
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, expected_dynamic_dram);
}