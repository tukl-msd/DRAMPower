#include <gtest/gtest.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/standards/lpddr6/interface_calculation_LPDDR6.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR6.h>

#include "DRAMPower/data/stats.h"
#include "DRAMPower/memspec/MemSpecLPDDR6.h"
#include "DRAMPower/standards/lpddr6/LPDDR6.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Interface.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/bus_types.h"
#include "DRAMPower/util/extensions.h"


using namespace DRAMPower;

// Burst length 48
static constexpr std::array<uint8_t, 64> data {
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

class LPDDR6_DBI_Tests : public ::testing::Test {
   public:
    LPDDR6_DBI_Tests() {
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 16}, data.data(), data.size() * 8},
            {11, CmdType::RD, {1, 0, 0, 0, 16}, data.data(),  data.size() * 8},
            {16, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });
        test_patterns.push_back({
            {0, CmdType::ACT, {1, 0, 0, 2}},
            {4, CmdType::WR, {1, 0, 0, 0, 16}, data.data(), data.size() * 8},
            {11, CmdType::RD, {1, 0, 0, 0, 16}, data.data(), data.size() * 8},
            {15, CmdType::RD, {1, 0, 0, 0, 16}, data.data(), data.size() * 8}, // Seamless read
            {20, CmdType::PRE, {1, 0, 0, 2}},
            {24, CmdType::END_OF_SIMULATION},
        });

        initSpec();
        ddr = std::make_unique<LPDDR6>(*spec);
    }

    void initSpec() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr6.json");
        spec = std::make_unique<MemSpecLPDDR6>(MemSpecLPDDR6::from_memspec(*data));
    }

    void runCommands(const std::vector<Command> &commands) {
        for (const Command &command : commands) {
            ddr->doCoreCommand(command);
            ddr->doInterfaceCommand(command);
        }
    }

    std::vector<std::vector<Command>> test_patterns;
    std::unique_ptr<MemSpecLPDDR6> spec;
    std::unique_ptr<LPDDR6> ddr;
};

TEST_F(LPDDR6_DBI_Tests, FormatData_0) {
    util::Bus<12> bus(12, 1, util::BusIdlePatternSpec::L, true);
    LPDDR6Interface::DBIFormatter formatter{};
    std::vector<uint8_t> expectedOutput {
        0xBD, 0xDF, 0xFB, 0xBD, 0xDF, 0xFB,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x3F, 0xCF, 0xFF, 0x3F, 0xCF, 0xFF,
        0xFF, 0xFC, 0xCF, 0xFF, 0xFC, 0xCF,
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,

        0xBD, 0xDF, 0xFB, 0xBD, 0xDF, 0xFB,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x3F, 0xCF, 0xFF, 0x3F, 0xCF, 0xFF,
        0xFF, 0xFC, 0xCF, 0xFF, 0xFC, 0xCF,
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    };
    std::vector<bool> inversions {
        true, true, true, true,
        false, false, false, false,
        false, false, false, false,
        true, true, true, true,

        true, true, true, true,
        false, false, false, false,
        false, false, false, false,
        true, true, true, true,
    };
    formatter.m_metaData = {0b10101010'01010101, 0b10101010'01010101};

    auto[output, output_size] = formatter.formatData(data.data(), data.size() * 8, inversions);
    EXPECT_EQ(output_size, 576);
    EXPECT_EQ(output_size, expectedOutput.size() * 8);
    bus.load(0, output, output_size);
    util::bus_stats_t stats = bus.get_stats(48);
    for (std::size_t i = 0; i < output_size / 8; ++i) {
        EXPECT_EQ(output[i], expectedOutput[i]);
    }

    // TODO verify bus stats
}

TEST_F(LPDDR6_DBI_Tests, Pattern_0) {
    ddr->getExtensionManager().withExtension<extensions::DBI>([](extensions::DBI& dbi) {
        dbi.enable(0, true);
    });
    runCommands(test_patterns[0]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);

    // TODO

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 0);
    EXPECT_EQ(stats.writeBus.zeroes, 0);
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 0);

    EXPECT_EQ(stats.readBus.ones, 0);
    EXPECT_EQ(stats.readBus.zeroes, 0);
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 0);

    // DBI
    EXPECT_EQ(stats.readDBI.ones, 0);
    EXPECT_EQ(stats.readDBI.zeroes, 0);
    EXPECT_EQ(stats.readDBI.ones_to_zeroes, 0);
    EXPECT_EQ(stats.readDBI.zeroes_to_ones, 0);

    EXPECT_EQ(stats.writeDBI.ones, 0);
    EXPECT_EQ(stats.writeDBI.zeroes, 0);
    EXPECT_EQ(stats.writeDBI.ones_to_zeroes, 0);
    EXPECT_EQ(stats.writeDBI.zeroes_to_ones, 0);

}

TEST_F(LPDDR6_DBI_Tests, Pattern_1) {
    ddr->getExtensionManager().withExtension<extensions::DBI>([](extensions::DBI& dbi) {
        dbi.enable(0, true);
    });
    runCommands(test_patterns[1]);

    SimulationStats stats = ddr->getStats();

    EXPECT_EQ(spec->dataRate, 2);

    // TODO

    // Data bus
    EXPECT_EQ(stats.writeBus.ones, 0);
    EXPECT_EQ(stats.writeBus.zeroes, 0);
    EXPECT_EQ(stats.writeBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.writeBus.zeroes_to_ones, 0);

    EXPECT_EQ(stats.readBus.ones, 0);
    EXPECT_EQ(stats.readBus.zeroes, 0);
    EXPECT_EQ(stats.readBus.ones_to_zeroes, 0);
    EXPECT_EQ(stats.readBus.zeroes_to_ones, 0);

    // DBI
    EXPECT_EQ(stats.readDBI.ones, 0);
    EXPECT_EQ(stats.readDBI.zeroes, 0);
    EXPECT_EQ(stats.readDBI.ones_to_zeroes, 0);
    EXPECT_EQ(stats.readDBI.zeroes_to_ones, 0);

    EXPECT_EQ(stats.writeDBI.ones, 0);
    EXPECT_EQ(stats.writeDBI.zeroes, 0);
    EXPECT_EQ(stats.writeDBI.ones_to_zeroes, 0);
    EXPECT_EQ(stats.writeDBI.zeroes_to_ones, 0);
}

class LPDDR6_DBI_Energy_Tests : public ::testing::Test {
   public:
    LPDDR6_DBI_Energy_Tests() {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr6.json");
        spec = std::make_unique<MemSpecLPDDR6>(MemSpecLPDDR6::from_memspec(*data));

        t_CK = spec->memTimingSpec.tCK;
        voltage = spec->vddq;

        // Change impedances to different values from each other
        spec->memImpedanceSpec.rdbi_R_eq = 1;
        spec->memImpedanceSpec.wdbi_R_eq = 2;

        spec->memImpedanceSpec.rdbi_dyn_E = 3;
        spec->memImpedanceSpec.wdbi_dyn_E = 4;

        io_calc = std::make_unique<InterfaceCalculation_LPDDR6>(*spec);
    }

    std::unique_ptr<MemSpecLPDDR6> spec;
    double t_CK;
    double voltage;
    std::unique_ptr<InterfaceCalculation_LPDDR6> io_calc;
};

TEST_F(LPDDR6_DBI_Energy_Tests, Energy) {
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
