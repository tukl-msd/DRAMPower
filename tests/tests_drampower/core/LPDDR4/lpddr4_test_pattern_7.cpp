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

class DramPowerTest_LPDDR4_7 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   0, CmdType::SREFEN,  { 0, 0, 0 }},
            {   40, CmdType::SREFEX,  { 0, 0, 0 }},
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

TEST_F(DramPowerTest_LPDDR4_7, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto stats = ddr->getStats();

    // Check bank command count: ACT
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].counter.act, 0);

    // Check bank command count: REFA
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].counter.refAllBank, 1);


    // Check cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 25);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 60);
    ASSERT_EQ(stats.rank_total[0].cycles.selfRefresh, 15);

    // Check bank specific ACT cycle count
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.act, 25);

    // Check bank specific PRE cycle count
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.pre, 60);

    // Check bank specific SREF cycle count
    for(uint64_t b = 0; b < memSpec->numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.selfRefresh, 15);

}

TEST_F(DramPowerTest_LPDDR4_7, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto energy = ddr->calcCoreEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();

    ASSERT_EQ(std::round(total_energy.E_act*1e12), 0);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 0);
    ASSERT_EQ(std::round(total_energy.E_ref_AB*1e12), 1699);
    ASSERT_EQ(std::round(energy.E_sref*1e12), 280);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 1024);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 793);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 1869);
    ASSERT_EQ(std::round((total_energy.total() + energy.E_sref)*1e12), 4873);
}
