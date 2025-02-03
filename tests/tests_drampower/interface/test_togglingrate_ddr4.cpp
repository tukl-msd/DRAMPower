#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include <cmath>

#include <DRAMUtils/config/toggling_rate.h>

#include "DRAMPower/standards/ddr4/DDR4.h"
#include "DRAMPower/memspec/MemSpecDDR4.h"
#include "DRAMPower/standards/ddr4/interface_calculation_DDR4.h"

#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"

using namespace DRAMPower;
using namespace DRAMUtils::Config;

#define SZ_BITS(x) sizeof(x)*8

// burst length = 16 for x8 devices
static constexpr uint8_t wr_data[] = {
    0, 0, 0, 0,  0, 0, 0, 255,  0, 0, 0, 0,  0, 0, 0, 0,
};

// burst length = 16 for x8 devices
static constexpr uint8_t rd_data[] = {
    255, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
};

class DDR4_TogglingRate_Tests : public ::testing::Test {
   public:
    DDR4_TogglingRate_Tests() {
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 16}, nullptr, datasize_bits},
            {11, CmdType::RD, {1, 0, 0, 0, 16}, nullptr, datasize_bits},
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
    uint64_t datasize_bits = 8 * 8; // 8 bytes
};

TEST_F(DDR4_TogglingRate_Tests, Pattern_0_LH) {
    // Setup toggling rate
    double togglingRateRead = 0.7;
    double togglingRateWrite = 0.3;
    double dutyCycleRead = 0.6;
    double dutyCycleWrite = 0.4;
    TogglingRateIdlePattern idlePatternRead = TogglingRateIdlePattern::L;
    TogglingRateIdlePattern idlePatternWrite = TogglingRateIdlePattern::H;
    ddr->setToggleRate(0, ToggleRateDefinition {
        togglingRateRead, // togglingRateRead
        togglingRateWrite, // togglingRateWrite
        dutyCycleRead, // dutyCycleRead
        dutyCycleWrite, // dutyCycleWrite
        idlePatternRead, // idlePatternRead
        idlePatternWrite  // idlePatternWrite
    });
    // Run commands
    runCommands(test_patterns.at(0));
    // SZ_BITS: 64, width: 8 -> Burstlength: 8 (datarate bus)
    // 0: ACT, 4: WR, 11: RD, 16: PRE, 24: EOS
    // Read bus: idle: L
        // 0 to 11 idle
        // 11 to 15 toggle
        // 15 to 24 idle
        // idle: 20 zeroes, toggle: 4 (datarate clock)
        // idle: 40 zeroes, toggle: 8 (datarate bus)
    // Write bus: idle: H
        // 0 to 4 idle
        // 4 to 8 toggle
        // 8 to 24 idle
        // idle: 20 ones, toggle: 4 (datarate clock)
        // idle: 40 ones, toggle: 8 (datarate bus)
    uint64_t toggles_read = 8;
    uint64_t toggles_write = 8;

    uint64_t idleread_ones = 0;
    uint64_t idleread_zeroes = 40;
    uint64_t idlewrite_ones = 40;
    uint64_t idlewrite_zeroes = 0;


    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);
    EXPECT_EQ(ddr->readBus.get_width(), spec->bitWidth);
    EXPECT_EQ(ddr->writeBus.get_width(), spec->bitWidth);

// Data bus
    // Read bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.read.ones, (idleread_ones +  static_cast<uint64_t>(std::floor(dutyCycleRead * toggles_read))) * spec->bitWidth); // 32
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.read.zeroes, (idleread_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleRead) * toggles_read))) * spec->bitWidth); // 344
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.read.ones_to_zeroes, std::floor((togglingRateRead / 2) * toggles_read) * spec->bitWidth); // 16
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.read.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateRead / 2) * toggles_read)) *  spec->bitWidth); // 16
    
    // Write bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.write.ones, (idlewrite_ones +  static_cast<uint64_t>(std::floor(dutyCycleWrite * toggles_write))) * spec->bitWidth); // 344
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.write.zeroes, (idlewrite_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleWrite) * toggles_write))) * spec->bitWidth); // 32
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.write.ones_to_zeroes, std::floor((togglingRateWrite / 2) * toggles_write) * spec->bitWidth); // 8
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.write.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateWrite / 2) * toggles_write)) * spec->bitWidth); // 8

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
    ddr->setToggleRate(0, ToggleRateDefinition {
        togglingRateRead, // togglingRateRead
        togglingRateWrite, // togglingRateWrite
        dutyCycleRead, // dutyCycleRead
        dutyCycleWrite, // dutyCycleWrite
        idlePatternRead, // idlePatternRead
        idlePatternWrite  // idlePatternWrite
    });
    // Run commands
    runCommands(test_patterns[0]);
    // SZ_BITS: 64, width: 8 -> Burstlength: 8 (datarate bus)
    // 0: ACT, 4: WR, 11: RD, 16: PRE, 24: EOS
    // Read bus: idle: H
        // 0 to 11 idle
        // 11 to 15 toggle
        // 15 to 24 idle
        // idle: 20 ones, toggle: 4 (datarate clock)
        // idle: 40 ones, toggle: 8 (datarate bus)
    // Write bus: idle: Z
        // 0 to 4 idle
        // 4 to 8 toggle
        // 8 to 24 idle
        // idle: 0 ones/zeroes, toggle: 4 (datarate clock)
        // idle: 0 ones/zeroes, toggle: 8 (datarate bus)
    uint64_t toggles_read = 8;
    uint64_t toggles_write = 8;

    uint64_t idleread_zeroes = 0;
    uint64_t idleread_ones = 40;
    uint64_t idlewrite_ones = 0;
    uint64_t idlewrite_zeroes = 0;


    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);
    EXPECT_EQ(ddr->readBus.get_width(), spec->bitWidth);
    EXPECT_EQ(ddr->writeBus.get_width(), spec->bitWidth);

// Data bus
    // Read bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.read.ones, (idleread_ones +  static_cast<uint64_t>(std::floor(dutyCycleRead * toggles_read))) * spec->bitWidth); // 352
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.read.zeroes, (idleread_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleRead) * toggles_read))) * spec->bitWidth); // 24
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.read.ones_to_zeroes, std::floor((togglingRateRead / 2) * toggles_read) * spec->bitWidth); // 16
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.read.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateRead / 2) * toggles_read)) *  spec->bitWidth); // 16
    
    // Write bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.write.ones, (idlewrite_ones +  static_cast<uint64_t>(std::floor(dutyCycleWrite * toggles_write))) * spec->bitWidth); // 24
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.write.zeroes, (idlewrite_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleWrite) * toggles_write))) * spec->bitWidth); // 32
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.write.ones_to_zeroes, std::floor((togglingRateWrite / 2) * toggles_write) * spec->bitWidth); // 8
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.write.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateWrite / 2) * toggles_write)) * spec->bitWidth); // 8
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
    stats.togglingStats.read.ones = 7;
    stats.togglingStats.read.zeroes = 11;
    stats.togglingStats.read.zeroes_to_ones = 19;
    stats.togglingStats.read.ones_to_zeroes = 39;

    stats.togglingStats.write.ones = 43;
    stats.togglingStats.write.zeroes = 59;
    stats.togglingStats.write.zeroes_to_ones = 13;
    stats.togglingStats.write.ones_to_zeroes = 17;

    // Controller -> write power
    // Dram -> read power
    double expected_static_controller =
        stats.togglingStats.write.zeroes * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_wb;
    double expected_static_dram =
        stats.togglingStats.read.zeroes * voltage * voltage * (0.5 * t_CK) / spec->memImpedanceSpec.R_eq_rb;

    // Dynamic power is consumed on 0 -> 1 transition
    double expected_dynamic_controller = stats.togglingStats.write.zeroes_to_ones *
                            0.5 * spec->memImpedanceSpec.C_total_wb * voltage * voltage;
    double expected_dynamic_dram = stats.togglingStats.read.zeroes_to_ones *
                            0.5 * spec->memImpedanceSpec.C_total_rb * voltage * voltage;

    interface_energy_info_t result = io_calc->calculateEnergy(stats);
    EXPECT_DOUBLE_EQ(result.controller.staticEnergy, expected_static_controller);
    EXPECT_DOUBLE_EQ(result.controller.dynamicEnergy, expected_dynamic_controller);
    EXPECT_DOUBLE_EQ(result.dram.staticEnergy, expected_static_dram);
    EXPECT_DOUBLE_EQ(result.dram.dynamicEnergy, expected_dynamic_dram);
}

TEST_F(DDR4_TogglingRate_Tests, Pattern_1) {
    // Setup toggling rate
    double togglingRateRead = 0.7;
    double togglingRateWrite = 0.3;
    double dutyCycleRead = 0.6;
    double dutyCycleWrite = 0.4;
    TogglingRateIdlePattern idlePatternRead = TogglingRateIdlePattern::L;
    TogglingRateIdlePattern idlePatternWrite = TogglingRateIdlePattern::H;
    // Run commands
    ddr->setToggleRate(0, std::nullopt);
    ddr->doCoreInterfaceCommand({0, CmdType::ACT, {1, 0, 0, 2}});
    ddr->doCoreInterfaceCommand({5, CmdType::WR, {1, 0, 0, 0, 16}, wr_data, SZ_BITS(wr_data)});
    ddr->doCoreInterfaceCommand({14, CmdType::RD, {1, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)});
    // Enable toggling rate at beginning of read
    // The toggling rate should be enabled at t=22
    ddr->setToggleRate(14, ToggleRateDefinition {
        togglingRateRead, // togglingRateRead
        togglingRateWrite, // togglingRateWrite
        dutyCycleRead, // dutyCycleRead
        dutyCycleWrite, // dutyCycleWrite
        idlePatternRead, // idlePatternRead
        idlePatternWrite  // idlePatternWrite
    });
    ddr->doCoreInterfaceCommand({23, CmdType::PRE, {1, 0, 0, 2}});
    ddr->doCoreInterfaceCommand({30, CmdType::ACT, {1, 0, 0, 2}});
    ddr->doCoreInterfaceCommand({35, CmdType::WR, {1, 0, 0, 0, 16}, nullptr, 16*8}); // burst length = 16
    ddr->doCoreInterfaceCommand({44, CmdType::RD, {1, 0, 0, 0, 16}, nullptr, 16*8}); // burst length = 16
    // Disable toggling rate during read
    // The toggling rate should be disabled at t=52
    ddr->setToggleRate(46, std::nullopt);
    ddr->doCoreInterfaceCommand({53, CmdType::PRE, {1, 0, 0, 2}});
    ddr->doCoreInterfaceCommand({56, CmdType::END_OF_SIMULATION});



    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);
    EXPECT_EQ(ddr->readBus.get_width(), spec->bitWidth);
    EXPECT_EQ(ddr->writeBus.get_width(), spec->bitWidth);

// Toggling rate
    uint64_t toggles_read = 16;
    uint64_t idleread_ones = 0;
    uint64_t idleread_zeroes = 44; // TogglingRateIdlePattern::L

    uint64_t toggles_write = 16;
    uint64_t idlewrite_ones = 44; // TogglingRateIdlePattern::H
    uint64_t idlewrite_zeroes = 0;

    // Read bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.read.ones, (idleread_ones +  static_cast<uint64_t>(std::floor(dutyCycleRead * toggles_read))) * spec->bitWidth); // 112
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.read.zeroes, (idleread_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleRead) * toggles_read))) * spec->bitWidth); // 296
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.read.ones_to_zeroes, std::floor((togglingRateRead / 2) * toggles_read) * spec->bitWidth); // 64
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.read.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateRead / 2) * toggles_read)) * spec->bitWidth); // 64
    
    // Write bus
    // ones: {idle + floor[duty_cycle * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.write.ones, (idlewrite_ones +  static_cast<uint64_t>(std::floor(dutyCycleWrite * toggles_write))) * spec->bitWidth); // 256
    // zeroes: {idle + floor[(1 - duty_cycle) * toggling_count]} * width
    EXPECT_EQ(stats.togglingStats.write.zeroes, (idlewrite_zeroes +  static_cast<uint64_t>(std::floor((1 - dutyCycleWrite) * toggles_write))) * spec->bitWidth); // 152
    // onestozeroes: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.write.ones_to_zeroes, std::floor((togglingRateWrite / 2) * toggles_write) * spec->bitWidth); // 32
    // zeroestoones: floor[(toggle_rate / 2) * toggling_count] * width
    EXPECT_EQ(stats.togglingStats.write.zeroes_to_ones,  static_cast<uint64_t>(std::floor((togglingRateWrite / 2) * toggles_write)) * spec->bitWidth); // 32

// Data bus
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 8);
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 8);
    EXPECT_EQ(stats.readBus.zeroes, 120);
    EXPECT_EQ(stats.readBus.ones, 296);

    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 16);
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 16);
    EXPECT_EQ(stats.writeBus.zeroes, 120);
    EXPECT_EQ(stats.writeBus.ones, 296);

// Clock (see test_interface_ddr4)
    EXPECT_EQ(stats.clockStats.zeroes_to_ones, 112);
    EXPECT_EQ(stats.clockStats.ones_to_zeroes, 112);
    EXPECT_EQ(stats.clockStats.zeroes, 112);
    EXPECT_EQ(stats.clockStats.ones, 112);

// Command bus (see test_interface_ddr4)
    EXPECT_EQ(stats.commandBus.zeroes_to_ones, 114);
    EXPECT_EQ(stats.commandBus.ones_to_zeroes, 114);
    EXPECT_EQ(stats.commandBus.zeroes, 114);
    EXPECT_EQ(stats.commandBus.ones, 1398);

// DQs (see test_interface_ddr4)
    EXPECT_EQ(stats.writeDQSStats.zeroes_to_ones, 32);
    EXPECT_EQ(stats.writeDQSStats.ones_to_zeroes, 32);
    EXPECT_EQ(stats.writeDQSStats.zeroes, 32);
    EXPECT_EQ(stats.writeDQSStats.ones, 32);
    EXPECT_EQ(stats.readDQSStats.zeroes_to_ones, 32);
    EXPECT_EQ(stats.readDQSStats.ones_to_zeroes, 32);
    EXPECT_EQ(stats.readDQSStats.zeroes, 32);
    EXPECT_EQ(stats.readDQSStats.ones, 32);

// PrePostamble
    auto prepos = stats.rank_total[0].prepos;
    EXPECT_EQ(prepos.readSeamless, 0);
    EXPECT_EQ(prepos.writeSeamless, 0);
    EXPECT_EQ(prepos.readMerged, 0);
    EXPECT_EQ(prepos.readMergedTime, 0);
    EXPECT_EQ(prepos.writeMerged, 0);
    EXPECT_EQ(prepos.writeMergedTime, 0);
}
