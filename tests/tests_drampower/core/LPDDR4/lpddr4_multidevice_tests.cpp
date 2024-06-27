/* Note
This test is based on lpddr4_test_pattern_4.cpp and assumes the correctness of the test.

Calculation for 5 devices:
E_act: 					392.30769230769232
E_pre: 					415.38461538461542
E_RD:					1356.9230769230769
E_bg_act:				1632.6923076923078
E_bg_act_shared:		1586.5384615384617
E_bg_pre:				623.07692307692309
Total:					4420.3846153846162

E_act * 5:				1961.5384615384616
E_pre * 5:				2076.9230769230771
E_RD * 5:				6784.6153846153845
E_bg_act * 5:			8163.461538461539
E_bg_act_shared * 5:	7932.6923076923085
E_bg_pre * 5:			3115.38461538461545
Total * 5:				22101.923076923081
*/


#include <gtest/gtest.h>

#include <DRAMPower/command/Command.h>

#include <DRAMPower/standards/lpddr4/LPDDR4.h>

#include <memory>

#include <DRAMPower/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>
#include <variant>

#include <fstream>


using namespace DRAMPower;

class DramPowerTest_LPDDR4_MultiDevice : public ::testing::Test {
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
    std::unique_ptr<DRAMPower::LPDDR4> ddr;
    uint64_t numberOfDevices;

    virtual void SetUp()
    {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "lpddr4.json");

        if(!f.is_open()){
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }
        json data = json::parse(f);
        MemSpecContainer memspeccontainer = data;
        MemSpecLPDDR4 memSpec(std::get<DRAMUtils::Config::MemSpecLPDDR4>(memspeccontainer.memspec.getVariant()));
        memSpec.numberOfDevices = 5;

        numberOfDevices = memSpec.numberOfDevices;
        ddr = std::make_unique<LPDDR4>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR4_MultiDevice, Counters_and_Cycles){
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

TEST_F(DramPowerTest_LPDDR4_MultiDevice, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto energy = ddr->calcEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();

    ASSERT_EQ(energy.bank_energy.size(), numberOfDevices * ddr->memSpec.numberOfBanks);

    // Validate every device has the same bank energy
    for(size_t i = 0; i < numberOfDevices; i++){
        for(size_t j = 0; j < ddr->memSpec.numberOfBanks; j++){
            ASSERT_EQ(energy.bank_energy[i * ddr->memSpec.numberOfBanks + j].E_act, energy.bank_energy[j].E_act);
            ASSERT_EQ(energy.bank_energy[i * ddr->memSpec.numberOfBanks + j].E_pre, energy.bank_energy[j].E_pre);
            ASSERT_EQ(energy.bank_energy[i * ddr->memSpec.numberOfBanks + j].E_RD, energy.bank_energy[j].E_RD);
            ASSERT_EQ(energy.bank_energy[i * ddr->memSpec.numberOfBanks + j].E_bg_act, energy.bank_energy[j].E_bg_act);
            ASSERT_EQ(energy.bank_energy[i * ddr->memSpec.numberOfBanks + j].E_bg_pre, energy.bank_energy[j].E_bg_pre);
        }
    }

    ASSERT_EQ(std::round(total_energy.E_act*1e12), 1962);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 2077);
    ASSERT_EQ(std::round(total_energy.E_RD*1e12), 6785);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 8163);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 7933);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 3115);
    ASSERT_EQ(std::round(total_energy.total()*1e12), 22102);
}
