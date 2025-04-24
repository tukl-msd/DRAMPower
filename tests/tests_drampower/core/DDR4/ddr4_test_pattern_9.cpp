#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/ddr4/DDR4.h>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecDDR4.h>
#include <variant>
#include <stdint.h>

#include <memory>

#include <fstream>
#include <string>


using namespace DRAMPower;

class DramPowerTest_DDR4_9 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
        // Timestamp,   Cmd,  { Channel, Bank, BG, Rank }
            { 5,   CmdType::ACT,  { 0, 0, 0, 0 }},
            { 15,  CmdType::ACT,  { 0, 5, 0, 0 }},
            { 20,  CmdType::RD,   { 0, 0, 0, 0 }},
            { 30,  CmdType::PDEA, { 0, 0, 0, 0 }},
            { 50,  CmdType::PDXA, { 0, 0, 0, 0 }},
            { 55,  CmdType::RD,   { 0, 5, 0, 0 }},
            { 60,  CmdType::RD,   { 0, 0, 0, 0 }},
            { 70,  CmdType::PREA, { 0, 0, 0, 0 }},
            { 80,  CmdType::PDEP, { 0, 0, 0, 0 }},
            { 95,  CmdType::PDXP, { 0, 0, 0, 0 }},
            { 100, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::DDR4> ddr;

    virtual void SetUp()
    {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "ddr4.json");
        auto memSpec = DRAMPower::MemSpecDDR4::from_memspec(*data);

        ddr = std::make_unique<DDR4>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_DDR4_9, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto stats = ddr->getStats();


    // Check cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 45);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 20);
    ASSERT_EQ(stats.rank_total[0].cycles.selfRefresh, 0);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownAct, 20);
    ASSERT_EQ(stats.rank_total[0].cycles.powerDownPre, 15);

    // Check bank specific ACT cycle count
    for(uint64_t b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0){
            ASSERT_EQ(stats .bank[b].cycles.act, 45);
        }else if (b == 5){
            ASSERT_EQ(stats .bank[b].cycles.act, 35);
        }else{
            ASSERT_EQ(stats .bank[b].cycles.act, 0);
        }
    }

    // Check bank specific PRE cycle count
    for(uint64_t b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0){
            ASSERT_EQ(stats.bank[b].cycles.pre, 20);
        }else if (b == 5){
            ASSERT_EQ(stats.bank[b].cycles.pre, 30);
        }else{
            ASSERT_EQ(stats.bank[b].cycles.pre, 65);
        }
    }

    // Check bank specific PDNA cycle count
    for(uint64_t b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownAct, 20);

    // Check bank specific PDNP cycle count
    for(uint64_t b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownPre, 15);

    // Check bank specific SREF cycle count
    for(uint64_t b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.selfRefresh, 0);
}

TEST_F(DramPowerTest_DDR4_9, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto energy = ddr->calcCoreEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();


    ASSERT_EQ(std::round(total_energy.E_act*1e12), 359);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 415);
    ASSERT_EQ(std::round(total_energy.E_RD*1e12), 1307);
    ASSERT_EQ(std::round(energy.E_sref*1e12), 0);
    ASSERT_EQ(std::round(energy.E_PDNA*1e12), 415);
    ASSERT_EQ(std::round(energy.E_PDNP*1e12), 235);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 1535);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 1521);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 623);
    ASSERT_EQ(std::round((total_energy.total() + energy.E_sref + energy.E_PDNA + energy.E_PDNP)*1e12), 4890);
}
