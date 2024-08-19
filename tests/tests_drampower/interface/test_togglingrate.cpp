#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include <cmath>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR4.h>

#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecDDR4.h"
#include "DRAMPower/standards/ddr4/DDR4.h"
#include "DRAMPower/standards/ddr4/interface_calculation_DDR4.h"

using DRAMPower::CmdType;
using DRAMPower::Command;
using DRAMPower::DDR4;
using DRAMPower::MemSpecDDR4;
using DRAMPower::ToggleRateDefinition;
using DRAMPower::TogglingRateIdlePattern;
using DRAMPower::SimulationStats;

#define SZ_BITS(x) sizeof(x)*8

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

        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 16}, nullptr, 64},
            {8, CmdType::WR, {1, 0, 0, 0, 16}, nullptr, 64},
            {16, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::RD, {1, 0, 0, 0, 16}, nullptr, 64},
            {8, CmdType::RD, {1, 0, 0, 0, 16}, nullptr, 64},
            {16, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 16}, nullptr, 64},
            {10, CmdType::WR, {1, 0, 0, 0, 16}, nullptr, 64},
            {15, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::RD, {1, 0, 0, 0, 16}, nullptr, 64},
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

TEST_F(DDR4_TogglingRate_Tests, Pattern_0) {
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
    runCommands(test_patterns[0]);
    // SZ_BITS: 64, width: 8 -> Burstlength: 8
    // 0: ACT, 4: WR, 11: RD, 16: PRE, 24: EOS
    // Write bus: idle: H
        // 0 to 4 idle
        // 4 to 12 toggle
        // 12 to 24 idle
        // idle: 16 ones, toggle: 8
    // Read bus: idle: L
        // 0 to 11 idle
        // 11 to 19 toggle
        // 19 to 24 idle
        // idle: 16 zeroes, toggle: 8
    uint64_t idleread_ones = 0;
    uint64_t idleread_zeroes = 16;
    uint64_t idlewrite_ones = 16;
    uint64_t idlewrite_zeroes = 0;


    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2); // TODO use in energy calculation

    // Toggling rate in stats
    EXPECT_TRUE(stats.togglingStats);

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
}
