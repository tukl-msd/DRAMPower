#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"
#include "DRAMPower/simconfig/simconfig.h"
#include "DRAMUtils/config/toggling_rate.h"

#include <DRAMPower/standards/ddr4/DDR4.h>
#include <DRAMPower/standards/ddr5/DDR5.h>
#include <DRAMPower/standards/lpddr4/LPDDR4.h>
#include <DRAMPower/standards/lpddr5/LPDDR5.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR4.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR5.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>

#include <DRAMPower/memspec/MemSpec.h>
#include <optional>
#include <stdint.h>

#include <memory>

using namespace DRAMPower;

template <typename Standard, typename MemSpec>
class DramPowerTest_DDR_Serialize : public ::testing::Test {
protected:
    using Pattern_t = std::vector<Command>;
    using PatternList_t = std::vector<Pattern_t>;

    static constexpr uint8_t wr_data[] = {
        0x00, 0xFF, 0x01, 0x10, 0x00, 0xFF, 0x00, 0xFF
    };
    static constexpr uint8_t rd_data[] = {
        0x00, 0xFF, 0x01, 0x10, 0x00, 0xFF, 0x00, 0xFF
    };

    // Test pattern
    PatternList_t testPattern = {
        {
            {   0, CmdType::ACT,  { 0, 0, 0 }},
            Command{15, CmdType::WR, TargetCoordinate{
                0,
                0,
                0,
                0,
                0,
            }, wr_data, sizeof(wr_data) * 8}, // TODO: for sz_bits 0 not working
            Command{25, CmdType::RD, TargetCoordinate{
                0,
                0,
                0,
                0,
                0,
            }, rd_data, sizeof(wr_data) * 8},
            {   35, CmdType::PRE,  { 0, 0, 0 }},
            {   45, CmdType::REFA,  { 0, 0, 0 }},
            {   80, CmdType::END_OF_SIMULATION },
        },
        {
            {   0, CmdType::ACT,  { 0, 0, 0 }},
            Command{15, CmdType::WR, TargetCoordinate{
                0,
                0,
                0,
                0,
                0,
            }, nullptr},
            Command{25, CmdType::RD, TargetCoordinate{
                0,
                0,
                0,
                0,
                0,
            }, nullptr},
            {   35, CmdType::PRE,  { 0, 0, 0 }},
            {   45, CmdType::REFA,  { 0, 0, 0 }},
            {   80, CmdType::END_OF_SIMULATION },
        }
    };


    // Test variables
    std::unique_ptr<Standard> ddr1;
    std::unique_ptr<Standard> ddr2;
    std::unique_ptr<MemSpec> memSpec;

    virtual void getPath(std::filesystem::path& path) const = 0;

    virtual void SetUp()
    {
        std::filesystem::path path;
        getPath(path);
        auto data = DRAMUtils::parse_memspec_from_file(path);
        memSpec = std::make_unique<MemSpec>(MemSpec::from_memspec(*data));
    }

    void createStandard(std::optional<DRAMUtils::Config::ToggleRateDefinition> trd) {
        if (!trd.has_value()) {
            ddr1 = std::make_unique<Standard>(*memSpec);
            ddr2 = std::make_unique<Standard>(*memSpec);
        } else {
            ddr1 = std::make_unique<Standard>(*memSpec, config::SimConfig{trd.value()});
            ddr2 = std::make_unique<Standard>(*memSpec, config::SimConfig{trd.value()});
        }
    }

    virtual void TearDown()
    {
    }
};

class DramPowerTest_DDR4_Serialize : public DramPowerTest_DDR_Serialize<DDR4, MemSpecDDR4> {
protected:
    void getPath(std::filesystem::path& path) const override {
        path = std::filesystem::path(TEST_RESOURCE_DIR) / "ddr4.json";
    }
};
class DramPowerTest_DDR5_Serialize : public DramPowerTest_DDR_Serialize<DDR5, MemSpecDDR5> {
protected:
    void getPath(std::filesystem::path& path) const override {
        path = std::filesystem::path(TEST_RESOURCE_DIR) / "ddr5.json";
    }
};
class DramPowerTest_LPDDR4_Serialize : public DramPowerTest_DDR_Serialize<LPDDR4, MemSpecLPDDR4> {
protected:
    void getPath(std::filesystem::path& path) const override {
        path = std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr4.json";
    }
};
class DramPowerTest_LPDDR5_Serialize : public DramPowerTest_DDR_Serialize<LPDDR5, MemSpecLPDDR5> {
protected:
    void getPath(std::filesystem::path& path) const override {
        path = std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr5.json";
    }
};

template <typename Standard>
void compareStats(const std::vector<Command>& testPattern, std::unique_ptr<Standard>& ddr1, std::unique_ptr<Standard>& ddr2) {
    assert(testPattern.size() == 6);
    // Enable dbi if possible
    ddr1->getExtensionManager().template withExtension<DRAMPower::extensions::DBI>([](DRAMPower::extensions::DBI& dbi) {
        dbi.enable(0, false);
    });
    // ACT
    ddr1->doCoreCommand(testPattern[0]);
    ddr1->doInterfaceCommand(testPattern[0]);
    // WR
    ddr1->doCoreCommand(testPattern[1]);
    ddr1->doInterfaceCommand(testPattern[1]);
    // Serialize and deserialize
    auto streamout = std::ostringstream();
    ddr1->serialize(streamout);
    auto streamin = std::istringstream(streamout.str());
    ddr2->deserialize(streamin);
    // RD
    ddr1->doCoreCommand(testPattern[2]);
    ddr1->doInterfaceCommand(testPattern[2]);
    ddr2->doCoreCommand(testPattern[2]);
    ddr2->doInterfaceCommand(testPattern[2]);
    // PRE
    ddr1->doCoreCommand(testPattern[3]);
    ddr1->doInterfaceCommand(testPattern[3]);
    ddr2->doCoreCommand(testPattern[3]);
    ddr2->doInterfaceCommand(testPattern[3]);
    // REFA
    ddr1->doCoreCommand(testPattern[4]);
    ddr1->doInterfaceCommand(testPattern[4]);
    ddr2->doCoreCommand(testPattern[4]);
    ddr2->doInterfaceCommand(testPattern[4]);
    // EOS
    ddr1->doCoreCommand(testPattern[5]);
    ddr1->doInterfaceCommand(testPattern[5]);
    ddr2->doCoreCommand(testPattern[5]);
    ddr2->doInterfaceCommand(testPattern[5]);

    auto stats1 = ddr1->getStats();
    auto stats2 = ddr2->getStats();

    // Compare the stats of both DDR4 instances are equal
    ASSERT_EQ(stats1, stats2);
}

TEST_F(DramPowerTest_DDR4_Serialize, Test0){
    createStandard(std::nullopt);
    compareStats(testPattern.at(0), ddr1, ddr2);
}

TEST_F(DramPowerTest_DDR5_Serialize, Test0){
    createStandard(std::nullopt);
    compareStats(testPattern.at(0), ddr1, ddr2);
}

TEST_F(DramPowerTest_LPDDR4_Serialize, Test0){
    createStandard(std::nullopt);
    compareStats(testPattern.at(0), ddr1, ddr2);
}

TEST_F(DramPowerTest_LPDDR5_Serialize, Test0){
    createStandard(std::nullopt);
    compareStats(testPattern.at(0), ddr1, ddr2);
}

TEST_F(DramPowerTest_DDR4_Serialize, Test1){
    createStandard(DRAMUtils::Config::ToggleRateDefinition {
        0.6,
        0.4,
        0.3,
        0.2,
        TogglingRateIdlePattern::L,
        TogglingRateIdlePattern::L,
    });
    compareStats(testPattern.at(1), ddr1, ddr2);
}

TEST_F(DramPowerTest_DDR5_Serialize, Test1){
    createStandard(DRAMUtils::Config::ToggleRateDefinition {
        0.6,
        0.4,
        0.3,
        0.2,
        TogglingRateIdlePattern::L,
        TogglingRateIdlePattern::L,
    });
    compareStats(testPattern.at(1), ddr1, ddr2);
}

TEST_F(DramPowerTest_LPDDR4_Serialize, Test1){
    createStandard(DRAMUtils::Config::ToggleRateDefinition {
        0.6,
        0.4,
        0.3,
        0.2,
        TogglingRateIdlePattern::L,
        TogglingRateIdlePattern::L,
    });
    compareStats(testPattern.at(1), ddr1, ddr2);
}

TEST_F(DramPowerTest_LPDDR5_Serialize, Test1){
    createStandard(DRAMUtils::Config::ToggleRateDefinition {
        0.6,
        0.4,
        0.3,
        0.2,
        TogglingRateIdlePattern::L,
        TogglingRateIdlePattern::L,
    });
    compareStats(testPattern.at(1), ddr1, ddr2);
}
