#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include <cmath>

#include "DRAMPower/standards/ddr5/DDR5.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/standards/ddr5/interface_calculation_DDR5.h"

#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"

using namespace DRAMPower;

#define SZ_BITS(x) (x)*8

class DDR5_TogglingRate_Tests : public ::testing::Test {
   public:
    DDR5_TogglingRate_Tests() {
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {3, CmdType::WR, {1, 0, 0, 0, 4}, nullptr, datasize_bits},
            {12, CmdType::RD, {1, 0, 0, 0, 4}, nullptr, datasize_bits},
            {21, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        initSpec();
        ddr = std::make_unique<DDR5>(*spec);
    }

    void initSpec() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "ddr5.json");
        spec = std::make_unique<DRAMPower::MemSpecDDR5>(DRAMPower::MemSpecDDR5::from_memspec(*data));
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
    uint_fast16_t datasize_bits = 32 * 8; // 32 bytes
};

TEST_F(DDR5_TogglingRate_Tests, Pattern_0_LH) {
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
    // 0: ACT, 3: WR, 12: RD, 21: PRE, 24: EOS
    // Read bus: idle: L
        // 0 to 12 idle
        // 12 to 24 toggle -> toggle cut off by EOS
        // idle: 12 zeroes, toggle: 12 (datarate clock)
        // idle: 24 zeroes, toggle: 24 (datarate bus)
    // Write bus: idle: H
        // 0 to 3 idle
        // 3 to 19 toggle
        // 19 to 24 idle
        // idle: 8 ones, toggle: 16 (datarate clock)
        // idle: 16 zeroes, toggle: 32 (datarate bus)
    uint64_t toggles_read = 24;
    uint64_t toggles_write = 32;

    uint64_t idleread_ones = 0;
    uint64_t idleread_zeroes = 24;
    uint64_t idlewrite_ones = 16;
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
    EXPECT_EQ(stats.togglingStats->read.ones, (idleread_ones +  static_cast<uint64_t>(std::floor(dutyCycleRead * toggles_read))) * spec->bitWidth); // 112
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->read.zeroes, (idleread_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleRead) * toggles_read))) * spec->bitWidth); // 264
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.ones_to_zeroes, std::floor((togglingRateRead / 2) * toggles_read) * spec->bitWidth); // 64
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateRead / 2) * toggles_read)) * spec->bitWidth); // 64
    
    // Write bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.ones, (idlewrite_ones +  static_cast<uint64_t>(std::floor(dutyCycleWrite * toggles_write))) * spec->bitWidth); // 224
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.zeroes, (idlewrite_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleWrite) * toggles_write))) * spec->bitWidth); // 152
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.ones_to_zeroes, std::floor((togglingRateWrite / 2) * toggles_write) * spec->bitWidth); // 32
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateWrite / 2) * toggles_write)) * spec->bitWidth); // 32

// Clock (see test_interface_ddr5)
    EXPECT_EQ(stats.clockStats.ones, 48);
    EXPECT_EQ(stats.clockStats.zeroes, 48);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 48);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 48);

// Command bus (see test_interface_ddr5)
    EXPECT_EQ(stats.commandBus.ones, 282);
    EXPECT_EQ(stats.commandBus.zeroes, 54);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 39);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 39);

// DQs (see test_interface_ddr5)
    uint_fast8_t NumDQsPairs = spec->bitWidth == 16 ? 2 : 1;
    int number_of_cycles = (datasize_bits / spec->bitWidth);
    uint_fast8_t scale = NumDQsPairs * 2; // Differential_Pairs * 2(pairs of 2)
    // f(t) = t / 2;
    int DQS_ones = scale * (number_of_cycles / 2); // scale * (cycles / 2)
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
}

TEST_F(DDR5_TogglingRate_Tests, Pattern_0_HZ) {
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
    // 0: ACT, 3: WR, 12: RD, 21: PRE, 24: EOS
    // Read bus: idle: H
        // 0 to 12 idle
        // 12 to 24 toggle -> toggle cut off by EOS
        // idle: 12 ones, toggle: 12 (datarate clock)
        // idle: 24 ones, toggle: 24 (datarate bus)
    // Write bus: idle: Z
        // 0 to 3 idle
        // 3 to 19 toggle
        // 19 to 24 idle
        // idle: 0 ones/zeroes, toggle: 16 (datarate clock)
        // idle: 0 ones/zeroes, toggle: 32 (datarate bus)
    uint64_t toggles_read = 24;
    uint64_t toggles_write = 32;

    uint64_t idleread_zeroes = 0;
    uint64_t idleread_ones = 24;
    uint64_t idlewrite_ones = 0;
    uint64_t idlewrite_zeroes = 0;


    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);

    // Toggling rate in stats
    EXPECT_TRUE(stats.togglingStats);

// Data bus
    // Read bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->read.ones, (idleread_ones +  static_cast<uint64_t>(std::floor(dutyCycleRead * toggles_read))) * spec->bitWidth); // 304
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->read.zeroes, (idleread_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleRead) * toggles_read))) * spec->bitWidth); // 72
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.ones_to_zeroes, std::floor((togglingRateRead / 2) * toggles_read) * spec->bitWidth); // 64
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateRead / 2) * toggles_read)) * spec->bitWidth); // 64
    
    // Write bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.ones, (idlewrite_ones +  static_cast<uint64_t>(std::floor(dutyCycleWrite * toggles_write))) * spec->bitWidth); // 96
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.zeroes, (idlewrite_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleWrite) * toggles_write))) * spec->bitWidth); // 152
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.ones_to_zeroes, std::floor((togglingRateWrite / 2) * toggles_write) * spec->bitWidth); // 32
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateWrite / 2) * toggles_write)) * spec->bitWidth); // 32
}

// Tests for power consumption (given a known SimulationStats)
class DDR5_TogglingRateEnergy_Tests : public ::testing::Test {
   public:
    DDR5_TogglingRateEnergy_Tests() {
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

TEST_F(DDR5_TogglingRateEnergy_Tests, DQ_Energy) {
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
        stats.togglingStats->write.zeroes * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_wb;
    double expected_static_dram =
        stats.togglingStats->read.zeroes * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_rb;

    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic_controller = stats.togglingStats->write.zeroes_to_ones *
                            0.5 * spec->memImpedanceSpec.C_total_wb * voltage * voltage;
    double expected_dynamic_dram = stats.togglingStats->read.zeroes_to_ones *
                            0.5 * spec->memImpedanceSpec.C_total_rb * voltage * voltage;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, expected_static_dram);
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, expected_dynamic_dram);
}
