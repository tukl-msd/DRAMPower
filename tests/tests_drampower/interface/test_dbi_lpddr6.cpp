#include <gtest/gtest.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMPower/standards/lpddr6/interface_calculation_LPDDR6.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR6.h>

#include "DRAMPower/memspec/MemSpecLPDDR6.h"
#include "DRAMPower/standards/lpddr6/LPDDR6.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Interface.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/bus_types.h"


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
    LPDDR6Interface::DataFormatter formatter{};
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
}
