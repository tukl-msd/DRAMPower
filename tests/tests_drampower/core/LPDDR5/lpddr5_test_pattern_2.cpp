#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR5.h>
#include <variant>

#include <fstream>


using namespace DRAMPower;

class DramPowerTest_LPDDR5_2 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   0, CmdType::ACT,  { 0, 0, 0 }},
            {   15, CmdType::RD,  { 0, 0, 0 }},
            {   35, CmdType::PRE,  { 0, 0, 0 }},
            { 50, CmdType::END_OF_SIMULATION },
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
        DRAMPower::MemSpecContainer memspeccontainer = data;
        MemSpecLPDDR5 memSpec(std::get<DRAMUtils::Config::MemSpecLPDDR5>(memspeccontainer.memspec.getVariant()));

        ddr = std::make_unique<LPDDR5>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR5_2, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto stats = ddr->getStats();

    // Check bank command count: ACT
    ASSERT_EQ(stats.bank[0].counter.act, 1);
    for(auto b = 1; b < ddr->memSpec.numberOfBanks; b++)
        ASSERT_EQ(stats.bank[b].counter.act, 0);

    // Check bank command count: RD
    ASSERT_EQ(stats.bank[0].counter.reads, 1);
    for(auto b = 1; b < ddr->memSpec.numberOfBanks; b++)
        ASSERT_EQ(stats.bank[b].counter.reads, 0);

    // Check bank command count: PRE
    ASSERT_EQ(stats.bank[0].counter.pre, 1);
    for(auto b = 1; b < ddr->memSpec.numberOfBanks; b++)
        ASSERT_EQ(stats.bank[b].counter.pre, 0);

    // Check cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 35);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 15);

    // Check bank specific ACT cycle count
    ASSERT_EQ(stats.bank[0].cycles.act, 35);
    for(auto b = 1; b < ddr->memSpec.numberOfBanks; b++)
        ASSERT_EQ(stats.bank[b].cycles.act, 0);

    // Check bank specific PRE cycle count
    ASSERT_EQ(stats.bank[0].cycles.pre, 15);
    for(auto b = 1; b < ddr->memSpec.numberOfBanks; b++)
        ASSERT_EQ(stats.bank[b].cycles.pre, 50);
}

TEST_F(DramPowerTest_LPDDR5_2, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto energy = ddr->calcCoreEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();

    ASSERT_EQ(std::round(total_energy.E_act*1e12), 196);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 208);
    ASSERT_EQ(std::round(total_energy.E_RD*1e12), 226);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 1131);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 1111);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 467);
    ASSERT_EQ(std::round(total_energy.total()*1e12), 2228);
}
