#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/ddr5/DDR5.h>

#include <memory>

#include <fstream>


using namespace DRAMPower;

class DramPowerTest_DDR5_11 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   0, CmdType::ACT,  { 0, 0, 0 }},
            {   15, CmdType::RD,  { 0, 0, 0 }},
            {   20, CmdType::ACT,  { 11, 0, 0 }},
            {   25, CmdType::ACT,  { 3, 0, 0 }},
            {   40, CmdType::WR,  { 11, 0, 0 }},
            {   45, CmdType::RD,  { 3, 0, 0 }},
            {   55, CmdType::PRESB,  { 0, 0, 0 }},
            {   65, CmdType::PRESB,  { 11, 0, 0 }},
            { 100, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::DDR5> ddr;

    virtual void SetUp()
    {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "ddr5.json");

        if(!f.is_open()){
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }
        json data = json::parse(f);
        MemSpecDDR5 memSpec(data["memspec"]);

        ddr = std::make_unique<DDR5>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_DDR5_11, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto stats = ddr->getStats();


    // Check cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 65);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 35);
    ASSERT_EQ(stats.rank_total[0].cycles.selfRefresh, 0);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownAct, 0);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownPre, 0);

    // Check bank specific ACT cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0){
            ASSERT_EQ(stats.bank[b].cycles.act, 55);
        }else if (b == 3){
            ASSERT_EQ(stats.bank[b].cycles.act, 40);
        }else if (b == 11){
            ASSERT_EQ(stats.bank[b].cycles.act, 45);
        }else{
            ASSERT_EQ(stats.bank[b].cycles.act, 0);
        }
    }

    // Check bank specific PRE cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0){
            ASSERT_EQ(stats.bank[b].cycles.pre, 45);
        }else if (b == 3){
            ASSERT_EQ(stats.bank[b].cycles.pre, 60);
        }else if (b == 11){
            ASSERT_EQ(stats.bank[b].cycles.pre, 55);
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

TEST_F(DramPowerTest_DDR5_11, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto energy = ddr->calcEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();


    ASSERT_EQ(std::round(total_energy.E_act), 538);
    ASSERT_EQ(std::round(total_energy.E_pre), 623);
    ASSERT_EQ(std::round(total_energy.E_RD), 871);
    ASSERT_EQ(std::round(total_energy.E_WR), 353);
    ASSERT_EQ(std::round(energy.E_sref), 0);
    ASSERT_EQ(std::round(energy.E_PDNA), 0);
    ASSERT_EQ(std::round(energy.E_PDNP), 0);
    ASSERT_EQ(std::round(total_energy.E_bg_act), 2221);
    ASSERT_EQ(std::round(energy.E_bg_act_shared), 2198);
    ASSERT_EQ(std::round(total_energy.E_bg_pre), 1090);
    ASSERT_EQ(std::round(total_energy.E_ref_SB), 0);
    ASSERT_EQ(std::round(total_energy.total() + energy.E_sref + energy.E_PDNA + energy.E_PDNP), 5697);
}
