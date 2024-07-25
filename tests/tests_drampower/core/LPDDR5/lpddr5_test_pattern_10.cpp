#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>
#include <variant>

#include <fstream>


using namespace DRAMPower;

class DramPowerTest_LPDDR5_10 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   10, CmdType::ACT,  { 0, 0, 0 }},
            {   30, CmdType::PRE,  { 0, 0, 0 }},
            {   45, CmdType::SREFEN,  { 0, 0, 0 }},
            {   70, CmdType::DSMEN,  { 0, 0, 0 }},
            {   85, CmdType::DSMEX,  { 0, 0, 0 }},
            {   95, CmdType::SREFEX,  { 0, 0, 0 }},
            { 100, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::LPDDR5> ddr;

    virtual void SetUp()
    {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr5.json");
        auto memSpec = DRAMPower::MemSpecLPDDR5::from_memspec(*data);

        ddr = std::make_unique<LPDDR5>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR5_10, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto stats = ddr->getStats();


    // Check cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 45);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 30);
    ASSERT_EQ(stats.rank_total[0].cycles.selfRefresh, 10);
    ASSERT_EQ(stats.rank_total[0].cycles.deepSleepMode, 15);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownAct, 0);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownPre, 0);

    // Check bank specific ACT cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0){
            ASSERT_EQ(stats.bank[b].cycles.act, 45);
        }else{
            ASSERT_EQ(stats.bank[b].cycles.act, 25);
        }
    }

    // Check bank specific PRE cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0){
            ASSERT_EQ(stats.bank[b].cycles.pre, 30);
        }else{
            ASSERT_EQ(stats.bank[b].cycles.pre, 50);
        }
    }

    // Check bank specific PDNA cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownAct, 0);

    // Check bank specific PDNP cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownPre, 0);

    // Check bank specific SREF cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.selfRefresh, 10);

    // Check bank specific DSM cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.deepSleepMode, 15);
}

TEST_F(DramPowerTest_LPDDR5_10, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto energy = ddr->calcCoreEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();


    ASSERT_EQ(std::round(total_energy.E_act*1e12), 196);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 208);
    ASSERT_EQ(std::round(total_energy.E_RD*1e12), 0);
    ASSERT_EQ(std::round(energy.E_sref*1e12), 187);
    ASSERT_EQ(std::round(energy.E_PDNA*1e12), 0);
    ASSERT_EQ(std::round(energy.E_PDNP*1e12), 0);
    ASSERT_EQ(std::round(energy.E_dsm*1e12), 183);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 1670);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 1428);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 935);
    ASSERT_EQ(std::round((total_energy.total() + energy.E_dsm +energy.E_sref + energy.E_PDNA + energy.E_PDNP)*1e12), 5078);
}
