#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>
#include <variant>
#include <stdint.h>

#include <fstream>


using namespace DRAMPower;

class DramPowerTest_LPDDR5_8 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   0, CmdType::PDEA,  { 0, 0, 0 }},
            {   30, CmdType::PDXA,  { 0, 0, 0 }},
            {   45, CmdType::PDEP,  { 0, 0, 0 }},
            {   70, CmdType::PDXP,  { 0, 0, 0 }},
            { 85, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::MemSpecLPDDR5> memSpec;
    std::unique_ptr<DRAMPower::LPDDR5> ddr;

    virtual void SetUp()
    {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr5.json");
        memSpec = std::make_unique<DRAMPower::MemSpecLPDDR5>(DRAMPower::MemSpecLPDDR5::from_memspec(*data));

        ddr = std::make_unique<LPDDR5>(*memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR5_8, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto stats = ddr->getStats();


    // Check cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 0);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 30);
    ASSERT_EQ(stats.rank_total[0].cycles.selfRefresh, 0);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownAct, 30);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownPre, 25);

    // Check bank specific ACT cycle count
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.act, 0);

    // Check bank specific PRE cycle count
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.pre, 30);

    // Check bank specific PDNA cycle count
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownAct, 30);

    // Check bank specific PDNP cycle count
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownPre, 25);

    // Check bank specific SREF cycle count
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.selfRefresh, 0);
}

TEST_F(DramPowerTest_LPDDR5_8, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto energy = ddr->calcCoreEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();


    ASSERT_EQ(std::round(total_energy.E_act*1e12), 0);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 0);
    ASSERT_EQ(std::round(energy.E_sref*1e12), 0);
    ASSERT_EQ(std::round(energy.E_PDNA*1e12), 623);
    ASSERT_EQ(std::round(energy.E_PDNP*1e12), 392);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 0);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 0);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 935);
    ASSERT_EQ(std::round((total_energy.total() + energy.E_sref + energy.E_PDNA + energy.E_PDNP)*1e12), 1950);
}
