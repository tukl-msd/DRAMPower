#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/ddr5/DDR5.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR5.h>
#include <variant>

#include <fstream>


using namespace DRAMPower;

class DramPowerTest_DDR5_10 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   0, CmdType::ACT,  { 0, 0, 0 }},
            {   15, CmdType::RD,  { 0, 0, 0 }},
            {   20, CmdType::ACT,  { 11, 0, 0 }},
            {   55, CmdType::PREA,  { 0, 0, 0 }},
            {   70, CmdType::REFSB,  { 0, 0, 0 }},
            {   75, CmdType::REFSB,  { 11, 0, 0 }},
            { 100, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::DDR5> ddr;

    virtual void SetUp()
    {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "ddr5.json");
        auto memSpec = DRAMPower::MemSpecDDR5::from_memspec(*data);

        ddr = std::make_unique<DDR5>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_DDR5_10, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto stats = ddr->getStats();


    // Check cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 80);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 20);
    ASSERT_EQ(stats.rank_total[0].cycles.selfRefresh, 0);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownAct, 0);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownPre, 0);

    // Check bank specific ACT cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0){
            ASSERT_EQ(stats.bank[b].cycles.act, 75);
        }else if (b == 3 || b == 8){
            ASSERT_EQ(stats.bank[b].cycles.act, 20);
        }else if (b == 11){
            ASSERT_EQ(stats.bank[b].cycles.act, 55);
        }else{
            ASSERT_EQ(stats.bank[b].cycles.act, 0);
        }
    }

    // Check bank specific PRE cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0){
            ASSERT_EQ(stats.bank[b].cycles.pre, 25);
        }else if (b == 3 || b == 8){
            ASSERT_EQ(stats.bank[b].cycles.pre, 80);
        }else if (b == 11){
            ASSERT_EQ(stats.bank[b].cycles.pre, 45);
        }else{
            ASSERT_EQ(stats.bank[b].cycles.pre, 100);
        }
    }

    // Check bank specific PDNA cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownAct, 0);

    // Check bank specific PDNP cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownPre, 0);

    // Check bank specific SREF cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.selfRefresh, 0);
}

TEST_F(DramPowerTest_DDR5_10, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto energy = ddr->calcCoreEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();


    ASSERT_EQ(std::round(total_energy.E_act*1e12), 359);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 415);
    ASSERT_EQ(std::round(total_energy.E_RD*1e12), 436);
    ASSERT_EQ(std::round(energy.E_sref*1e12), 0);
    ASSERT_EQ(std::round(energy.E_PDNA*1e12), 0);
    ASSERT_EQ(std::round(energy.E_PDNP*1e12), 0);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 2733);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 2705);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 623);
    ASSERT_EQ(std::round(total_energy.E_ref_SB*1e12), 2696);
    ASSERT_EQ(std::round((total_energy.total() + energy.E_sref + energy.E_PDNA + energy.E_PDNP)*1e12), 7262);
}
