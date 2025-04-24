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

class DramPowerTest_LPDDR4_10 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
        // Timestamp,   Cmd,  { Channel, Bank, BG, Rank }
            { 0,   CmdType::ACT,  { 0, 0, 0, 0 }},
            { 5,   CmdType::ACT,  { 0, 2, 0, 0 }},
            { 15,  CmdType::ACT,  { 0, 10, 0, 0 }},
            { 30,  CmdType::PREA, { 0, 0, 0, 0 }},
            { 40,  CmdType::REFB, { 0, 9, 0, 0 }},
            { 50,  CmdType::REFB, { 0, 5, 0, 0 }},
            { 100, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::LPDDR4> ddr;

    virtual void SetUp()
    {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr4.json");
        auto memSpec = DRAMPower::MemSpecLPDDR4::from_memspec(*data);

        ddr = std::make_unique<LPDDR4>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR4_10, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto stats = ddr->getStats();


    // Check cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 60);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 40);
    ASSERT_EQ(stats.rank_total[0].cycles.selfRefresh, 0);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownAct, 0);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownPre, 0);

    // Check bank specific ACT cycle count
    for(uint64_t b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0){
            ASSERT_EQ(stats .bank[b].cycles.act, 30);
        }else if (b == 2){
            ASSERT_EQ(stats .bank[b].cycles.act, 25);
        }else if(b == 10){
            ASSERT_EQ(stats .bank[b].cycles.act, 15);
        }else if (b == 5 || b == 9){
            ASSERT_EQ(stats .bank[b].cycles.act, 20);
        }else{
            ASSERT_EQ(stats .bank[b].cycles.act, 0);
        }
    }

    // Check bank specific PRE cycle count
    for(uint64_t b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0){
            ASSERT_EQ(stats.bank[b].cycles.pre, 70);
        }else if (b == 2){
            ASSERT_EQ(stats.bank[b].cycles.pre, 75);
        }else if (b== 10){
            ASSERT_EQ(stats.bank[b].cycles.pre, 85);
        }else if (b == 5 || b == 9){
            ASSERT_EQ(stats.bank[b].cycles.pre, 80);
        }else{
            ASSERT_EQ(stats.bank[b].cycles.pre, 100);
        }
    }

    // Check bank specific PDNA cycle count
    for(uint64_t b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownAct, 0);

    // Check bank specific PDNP cycle count
    for(uint64_t b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownPre, 0);

    // Check bank specific SREF cycle count
    for(uint64_t b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.selfRefresh, 0);
}

TEST_F(DramPowerTest_LPDDR4_10, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto energy = ddr->calcCoreEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();


    ASSERT_EQ(std::round(total_energy.E_act*1e12), 588);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 623);
    ASSERT_EQ(std::round(total_energy.E_RD*1e12), 0);
    ASSERT_EQ(std::round(total_energy.E_ref_2B*1e12), 0);
    ASSERT_EQ(std::round(total_energy.E_ref_PB*1e12), 140);
    ASSERT_EQ(std::round(energy.E_sref*1e12), 0);
    ASSERT_EQ(std::round(energy.E_PDNA*1e12), 0);
    ASSERT_EQ(std::round(energy.E_PDNP*1e12), 0);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 1967);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 1904);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 1246);
    ASSERT_EQ(std::round((total_energy.total() + energy.E_sref + energy.E_PDNA + energy.E_PDNP)*1e12), 4565);
}
