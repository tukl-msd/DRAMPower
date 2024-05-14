#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>

#include <memory>

#include <fstream>


using namespace DRAMPower;

class DramPowerTest_LPDDR5_4 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   0, CmdType::ACT,  { 0, 0, 0 }},
            {   15, CmdType::RD,  { 0, 0, 0 }},
            {   20, CmdType::ACT,  { 3, 0, 0 }},
            {   35, CmdType::RD,  { 3, 0, 0 }},
            {   40, CmdType::RD,  { 0, 0, 0 }},
            {   50, CmdType::PREA,  { 0, 0, 0 }},
            { 70, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::LPDDR5> ddr;

    virtual void SetUp()
    {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "lpddr5.json");

        if(!f.is_open()){
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }
        json data = json::parse(f);
        MemSpecLPDDR5 memSpec(data["memspec"]);

        ddr = std::make_unique<LPDDR5>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR5_4, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto stats = ddr->getStats();

    // Check bank command count: ACT
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0 || b == 3)
            ASSERT_EQ(stats.bank[b].counter.act, 1);
        else
            ASSERT_EQ(stats.bank[b].counter.act, 0);
    }

    // Check bank command count: RD
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if (b == 0)
            ASSERT_EQ(stats.bank[b].counter.reads, 2);
        else if(b == 3)
            ASSERT_EQ(stats.bank[b].counter.reads, 1);
        else
            ASSERT_EQ(stats.bank[b].counter.reads, 0);
    };

    // Check bank command count: PRE
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0 || b == 3)
            ASSERT_EQ(stats.bank[b].counter.pre, 1);
        else
            ASSERT_EQ(stats.bank[b].counter.pre, 0);
    }

    // Check cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 50);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 20);

    // Check bank specific ACT cycle count;
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if (b == 0)
            ASSERT_EQ(stats.bank[b].cycles.act, 50);
        else if(b == 3)
            ASSERT_EQ(stats.bank[b].cycles.act, 30);
        else
            ASSERT_EQ(stats.bank[b].cycles.act, 0);
    }

    // Check bank specific PRE cycle count
    for(auto b = 1; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0)
            ASSERT_EQ(stats.bank[b].cycles.pre, 20);
        else if (b == 3)
            ASSERT_EQ(stats.bank[b].cycles.pre, 40);
        else
            ASSERT_EQ(stats.bank[b].cycles.pre, 70);
    }
}

TEST_F(DramPowerTest_LPDDR5_4, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto energy = ddr->calcEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();

    ASSERT_EQ(std::round(total_energy.E_act*1e12), 392);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 415);
    ASSERT_EQ(std::round(total_energy.E_RD*1e12), 678);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 1633);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 1587);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 623);
    ASSERT_EQ(std::round(total_energy.total()*1e12), 3742);
}
