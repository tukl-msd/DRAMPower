#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>

#include <memory>

#include <fstream>

using namespace DRAMPower;

class DramPowerTest_LPDDR5_0 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   0, CmdType::ACT,  { 0, 0, 0 }},
            {   15, CmdType::PRE,  { 0, 0, 0 }},
            { 15, CmdType::END_OF_SIMULATION },
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

TEST_F(DramPowerTest_LPDDR5_0, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto stats = ddr->getStats();

    // Check bank command count: ACT
    ASSERT_EQ(stats.bank[0].counter.act, 1);
    for(auto b = 1; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].counter.act, 0);


    // Check cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 15);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 0);

    // Check bank specific ACT cycle count
    ASSERT_EQ(stats.bank[0].cycles.act, 15);
    for(auto b = 1; b < ddr->memSpec.numberOfBanks; b++)
        ASSERT_EQ(stats.bank[b].cycles.act, 0);

    // Check bank specific PRE cycle count
    ASSERT_EQ(stats.bank[0].cycles.pre, 0);
    for(auto b = 1; b < ddr->memSpec.numberOfBanks; b++)
        ASSERT_EQ(stats.bank[b].cycles.pre, 15);
}

TEST_F(DramPowerTest_LPDDR5_0, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto energy = ddr->calcEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();


    ASSERT_EQ(std::round(total_energy.E_act), 196);
    ASSERT_EQ(std::round(total_energy.E_pre), 208);
    ASSERT_EQ(std::round(energy.E_bg_act_shared), 476);
    ASSERT_EQ(std::round(total_energy.E_bg_act), 485);
    ASSERT_EQ(std::round(total_energy.E_bg_pre), 0);
    ASSERT_EQ(std::round(total_energy.total()), 888);
}
