#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr4/LPDDR4.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>
#include <variant>
#include <stdint.h>

#include <fstream>


using namespace DRAMPower;

class DramPowerTest_LPDDR4_6 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   0, CmdType::ACT,  { 0, 0, 0 }},
            {   15, CmdType::RDA,  { 0, 0, 0 }},
            {   20, CmdType::ACT,  { 3, 0, 0 }},
            {   35, CmdType::RD,  { 3, 0, 0 }},
            {   50, CmdType::PREA,  { 0, 0, 0 }},
            {   65, CmdType::REFA,  { 0, 0, 0 }},
            { 100, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::MemSpecLPDDR4> memSpec;
    std::unique_ptr<DRAMPower::LPDDR4> ddr;

    virtual void SetUp()
    {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr4.json");
        memSpec = std::make_unique<DRAMPower::MemSpecLPDDR4>(DRAMPower::MemSpecLPDDR4::from_memspec(*data));

        ddr = std::make_unique<LPDDR4>(*memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR4_6, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto stats = ddr->getStats();

    // Check bank command count: ACT
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++){
        if(b == 0 || b == 3)
            ASSERT_EQ(stats.bank[b].counter.act, 1);
        else
            ASSERT_EQ(stats.bank[b].counter.act, 0);
    }

    // Check bank command count: RD
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++){
        if (b == 3)
            ASSERT_EQ(stats.bank[b].counter.reads, 1);
        else
            ASSERT_EQ(stats.bank[b].counter.reads, 0);
    }

    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++){
        if (b == 0)
            ASSERT_EQ(stats.bank[b].counter.readAuto, 1);
        else
            ASSERT_EQ(stats.bank[b].counter.readAuto, 0);
    }

    // Check bank command count: PRE
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++){
        if(b == 0 || b == 3)
            ASSERT_EQ(stats.bank[b].counter.pre, 1);
        else
            ASSERT_EQ(stats.bank[b].counter.pre, 0);
    }

    // Check bank command count: REFA
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++){
        ASSERT_EQ(stats.bank[b].counter.refAllBank, 1);
    }

    // Check cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 75);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 25);

    // Check bank specific ACT cycle count;
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++){
        if (b == 0)
            ASSERT_EQ(stats.bank[b].cycles.act, 50);
        else if(b == 3)
            ASSERT_EQ(stats.bank[b].cycles.act, 55);
        else
            ASSERT_EQ(stats.bank[b].cycles.act, 25);
    }

    // Check bank specific PRE cycle count
    for(uint64_t b = 1; b < memSpec->numberOfBanks; b++){
        if(b == 0)
            ASSERT_EQ(stats.bank[b].cycles.pre, 50);
        else if (b == 3)
            ASSERT_EQ(stats.bank[b].cycles.pre, 45);
        else
            ASSERT_EQ(stats.bank[b].cycles.pre, 75);
    }
}

TEST_F(DramPowerTest_LPDDR4_6, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto energy = ddr->calcCoreEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();

    ASSERT_EQ(std::round(total_energy.E_act*1e12), 392);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 415);
    ASSERT_EQ(std::round(total_energy.E_RD*1e12), 452);
    ASSERT_EQ(std::round(total_energy.E_RDA*1e12), 452);
    ASSERT_EQ(std::round(total_energy.E_ref_AB*1e12), 1699);
    ASSERT_EQ(std::round(total_energy.E_ref_SB*1e12), 0);
    ASSERT_EQ(std::round(total_energy.E_ref_2B*1e12), 0);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 2642);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 2380);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 779);
    ASSERT_EQ(std::round(total_energy.total()*1e12), 6833);
}
