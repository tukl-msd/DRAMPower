#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/hbm2/HBM2.h>
#include <DRAMPower/memspec/MemSpec.h>

#include <DRAMUtils/memspec/standards/MemSpecHBM2.h>

#include <stdint.h>


using namespace DRAMPower;

class DramPowerTest_HBM2_0 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            { 0, CmdType::ACT,  { 0, 0, 0 }},
            { 15, CmdType::PRE,  { 0, 0, 0 }},
            { 16, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::HBM2> ddr;
    std::unique_ptr<DRAMPower::MemSpecHBM2> memSpec;

    virtual void SetUp()
    {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "hbm2.json");
        memSpec = std::make_unique<DRAMPower::MemSpecHBM2>(DRAMPower::MemSpecHBM2::from_memspec(*data));
        ddr = std::make_unique<HBM2>(*memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_HBM2_0, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
        ddr->doInterfaceCommand(command);
    }

    auto stats = ddr->getStats();

    // Check bank command count: ACT
    const auto n_banks = memSpec->numberOfPseudoChannels * memSpec->numberOfDevices * memSpec->numberOfBanks * memSpec->numberOfStacks;
    ASSERT_EQ(stats.bank.size(), n_banks);
    ASSERT_EQ(stats.bank.at(0).counter.act, 1);
    for(uint64_t b = 1; b < memSpec->numberOfBanks * memSpec->numberOfStacks; b++)  ASSERT_EQ(stats.bank.at(b).counter.act, 0);


    // Check cycles count
    ASSERT_EQ(stats.rank_total.at(0).cycles.act, 15);
    ASSERT_EQ(stats.rank_total.at(0).cycles.pre, 1);

    // Check bank specific ACT cycle count
    ASSERT_EQ(stats.bank.at(0).cycles.act, 15);
    for(uint64_t b = 1; b < memSpec->numberOfBanks; b++)
        ASSERT_EQ(stats.bank.at(b).cycles.act, 0);

    // Check bank specific PRE cycle count
    ASSERT_EQ(stats.bank.at(0).cycles.pre, 1);
    for(uint64_t b = 1; b < memSpec->numberOfBanks * memSpec->numberOfStacks; b++)
        ASSERT_EQ(stats.bank.at(b).cycles.pre, 16);
}

TEST_F(DramPowerTest_HBM2_0, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto energy = ddr->calcCoreEnergy(testPattern.back().timestamp);
    auto total_energy = energy.aggregated_bank_energy();


    ASSERT_EQ(std::round(total_energy.E_act*1e12), -153);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 21);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 1006);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 457);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 518);
    ASSERT_EQ(std::round(total_energy.total()*1e12), 1391);
}
