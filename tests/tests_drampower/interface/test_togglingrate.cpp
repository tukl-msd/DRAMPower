#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include <cmath>

#include "DRAMPower/standards/ddr4/DDR4.h"
#include "DRAMPower/memspec/MemSpecDDR4.h"

#include "DRAMPower/standards/ddr5/DDR5.h"
#include "DRAMPower/memspec/MemSpecDDR5.h"
#include "DRAMPower/standards/lpddr4/LPDDR4.h"
#include "DRAMPower/memspec/MemSpecLPDDR4.h"
#include "DRAMPower/standards/lpddr5/LPDDR5.h"
#include "DRAMPower/memspec/MemSpecLPDDR5.h"


#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/standards/ddr4/interface_calculation_DDR4.h"

using namespace DRAMPower;

#define SZ_BITS(x) (x)*8

class DDR4_TogglingRate_Tests : public ::testing::Test {
   public:
    DDR4_TogglingRate_Tests() {
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 16}, nullptr, 64},
            {11, CmdType::RD, {1, 0, 0, 0, 16}, nullptr, 64},
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

TEST_F(DDR4_TogglingRate_Tests, Pattern_0_LH) {
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
    // SZ_BITS: 64, width: 8 -> Burstlength: 8
    // 0: ACT, 4: WR, 11: RD, 16: PRE, 24: EOS
    // Read bus: idle: L
        // 0 to 11 idle
        // 11 to 19 toggle
        // 19 to 24 idle
        // idle: 16 zeroes, toggle: 8
    // Write bus: idle: H
        // 0 to 4 idle
        // 4 to 12 toggle
        // 12 to 24 idle
        // idle: 16 ones, toggle: 8
    uint64_t idleread_ones = 0;
    uint64_t idleread_zeroes = 16;
    uint64_t idlewrite_ones = 16;
    uint64_t idlewrite_zeroes = 0;


    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2); // TODO use in energy calculation

    // Toggling rate in stats
    EXPECT_TRUE(stats.togglingStats);

// Data bus
    // Read bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->read.ones, (idleread_ones +  static_cast<uint64_t>(std::floor(dutyCycleRead * 8))) * 8); // 32
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->read.zeroes, (idleread_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleRead) * 8))) * 8); // 152
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.ones_to_zeroes, std::floor((togglingRateRead / 2) * 8) * 8); // 16
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateRead / 2) * 8)) *  8); // 16
    
    // Write bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.ones, (idlewrite_ones +  static_cast<uint64_t>(std::floor(dutyCycleWrite * 8))) * 8); // 152
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.zeroes, (idlewrite_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleWrite) * 8))) * 8); // 32
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.ones_to_zeroes, std::floor((togglingRateWrite / 2) * 8) * 8); // 8
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateWrite / 2) * 8)) *  8); // 8

// Clock (see test_interface_ddr4)
    EXPECT_EQ(stats.clockStats.ones, 48);
    EXPECT_EQ(stats.clockStats.zeroes, 48);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 48);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 48);

// Command bus (see test_interface_ddr4)
    EXPECT_EQ(stats.commandBus.ones, 591);
    EXPECT_EQ(stats.commandBus.zeroes, 57);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 57);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 57);

// DQs (see test_interface_ddr4)
    // number of cycles per write/read
    // BL = (BL_BITS / width)
    // number of cycles = BL / data rate
    int number_of_cycles = (64 / 8) / spec->dataRate;
    int DQS_ones = number_of_cycles * spec->dataRate;
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

TEST_F(DDR4_TogglingRate_Tests, Pattern_0_HZ) {
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
    // SZ_BITS: 64, width: 8 -> Burstlength: 8
    // 0: ACT, 4: WR, 11: RD, 16: PRE, 24: EOS
    // Read bus: idle: H
        // 0 to 11 idle
        // 11 to 19 toggle
        // 19 to 24 idle
        // idle: 16 ones, toggle: 8
    // Write bus: idle: Z
        // 0 to 4 idle
        // 4 to 12 toggle
        // 12 to 24 idle
        // idle: 0 ones/zeroes, toggle: 8
    uint64_t idleread_zeroes = 0;
    uint64_t idleread_ones = 16;
    uint64_t idlewrite_ones = 0;
    uint64_t idlewrite_zeroes = 0;


    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2); // TODO use in energy calculation

    // Toggling rate in stats
    EXPECT_TRUE(stats.togglingStats);

// Data bus
    // Read bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->read.ones, (idleread_ones +  static_cast<uint64_t>(std::floor(dutyCycleRead * 8))) * 8); // 32
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->read.zeroes, (idleread_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleRead) * 8))) * 8); // 152
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.ones_to_zeroes, std::floor((togglingRateRead / 2) * 8) * 8); // 16
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->read.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateRead / 2) * 8)) *  8); // 16
    
    // Write bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.ones, (idlewrite_ones +  static_cast<uint64_t>(std::floor(dutyCycleWrite * 8))) * 8); // 152
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats->write.zeroes, (idlewrite_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleWrite) * 8))) * 8); // 32
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.ones_to_zeroes, std::floor((togglingRateWrite / 2) * 8) * 8); // 8
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats->write.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateWrite / 2) * 8)) *  8); // 8

// Clock (see test_interface_ddr4)
    EXPECT_EQ(stats.clockStats.ones, 48);
    EXPECT_EQ(stats.clockStats.zeroes, 48);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 48);
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 48);

// Command bus (see test_interface_ddr4)
    EXPECT_EQ(stats.commandBus.ones, 591);
    EXPECT_EQ(stats.commandBus.zeroes, 57);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 57);
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 57);

// DQs (see test_interface_ddr4)
    // number of cycles per write/read
    // BL = (BL_BITS / width)
    // number of cycles = BL / data rate
    int number_of_cycles = (64 / 8) / spec->dataRate;
    int DQS_ones = number_of_cycles * spec->dataRate;
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

// Tests for power consumption (given a known SimulationStats)
class DDR4_TogglingRateEnergy_Tests : public ::testing::Test {
   public:
    DDR4_TogglingRateEnergy_Tests() {
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

        io_calc = std::make_unique<InterfaceCalculation_DDR4>(*spec);
    }

    std::unique_ptr<MemSpecDDR4> spec;
    double t_CK;
    double voltage;
    std::unique_ptr<InterfaceCalculation_DDR4> io_calc;
};

TEST_F(DDR4_TogglingRateEnergy_Tests, DQ_Energy) {
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
    // zeroes and ones of the data bus are the zeroes and ones per pattern (data rate is not modeled in the bus)
    // data rate data bus is 2 -> t_per_bit = 0.5 * t_CK
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
