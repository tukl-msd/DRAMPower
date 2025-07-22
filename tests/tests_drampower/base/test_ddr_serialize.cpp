#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/ddr4/DDR4.h>
#include <DRAMPower/standards/ddr5/DDR5.h>
#include <DRAMPower/standards/lpddr4/LPDDR4.h>
#include <DRAMPower/standards/lpddr5/LPDDR5.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR4.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR5.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>

#include <DRAMPower/memspec/MemSpec.h>
#include <stdint.h>

#include <memory>

using namespace DRAMPower;

template <typename Standard, typename MemSpec>
class DramPowerTest_DDR_Serialize : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
        {   0, CmdType::ACT,  { 0, 0, 0 }},
        {   15, CmdType::PRE,  { 0, 0, 0 }},
        {   30, CmdType::REFA,  { 0, 0, 0 }},
        {   60, CmdType::END_OF_SIMULATION },
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
        ddr1 = std::make_unique<Standard>(*memSpec);
        ddr2 = std::make_unique<Standard>(*memSpec);
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
    assert(testPattern.size() == 4);
    // // Enable dbi
    ddr1->getExtensionManager().template withExtension<DRAMPower::extensions::DBI>([](DRAMPower::extensions::DBI& dbi) {
        dbi.enable(0, false);
    });
    // ACT
    ddr1->doCoreCommand(testPattern[0]);
    ddr1->doInterfaceCommand(testPattern[0]);
    // PRE
    ddr1->doCoreCommand(testPattern[1]);
    ddr1->doInterfaceCommand(testPattern[1]);
    // Serialize and deserialize
    auto streamout = std::ostringstream();
    ddr1->serialize(streamout);
    auto streamin = std::istringstream(streamout.str());
    ddr2->deserialize(streamin);
    // REFA
    ddr1->doCoreCommand(testPattern[2]);
    ddr1->doInterfaceCommand(testPattern[2]);
    ddr2->doCoreCommand(testPattern[2]);
    ddr2->doInterfaceCommand(testPattern[2]);
    // EOS
    ddr1->doCoreCommand(testPattern[3]);
    ddr1->doInterfaceCommand(testPattern[3]);
    ddr2->doCoreCommand(testPattern[3]);
    ddr2->doInterfaceCommand(testPattern[3]);

    auto stats1 = ddr1->getStats();
    auto stats2 = ddr2->getStats();

    // Compare the stats of both DDR4 instances are equal
    ASSERT_EQ(stats1, stats2);
}

TEST_F(DramPowerTest_DDR4_Serialize, Test0){
    compareStats(testPattern, ddr1, ddr2);
}

TEST_F(DramPowerTest_DDR5_Serialize, Test0){
    compareStats(testPattern, ddr1, ddr2);
}

TEST_F(DramPowerTest_LPDDR4_Serialize, Test0){
    compareStats(testPattern, ddr1, ddr2);
}

TEST_F(DramPowerTest_LPDDR5_Serialize, Test0){
    compareStats(testPattern, ddr1, ddr2);
}
