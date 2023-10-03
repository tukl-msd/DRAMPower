#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>

#include <memory>

using namespace DRAMPower;

class DramPowerTest_LPDDR5_21 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   5,  CmdType::SREFEN },
            {  15,  CmdType::SREFEX },
            {  25,  CmdType::SREFEN },
            {  35,  CmdType::DSMEN  },
            {  45,  CmdType::DSMEX  },
            {  55,  CmdType::SREFEX },
            {  80,  CmdType::SREFEN },
            { 100,  CmdType::DSMEN  },
            { 125,  CmdType::END_OF_SIMULATION },
    };

    // Test variables
    std::unique_ptr<DRAMPower::LPDDR5> ddr;

    virtual void SetUp()
    {
        MemSpecLPDDR5 memSpec;
        memSpec.numberOfRanks = 1;
        memSpec.numberOfBanks = 8;
        memSpec.numberOfBankGroups = 2;
        memSpec.banksPerGroup = 4;
        memSpec.bank_arch = MemSpecLPDDR5::MBG;


        memSpec.memTimingSpec.tRAS = 10;
        memSpec.memTimingSpec.tRTP = 10;
        memSpec.memTimingSpec.tRCD = 20;
        memSpec.memTimingSpec.tRFC = 10;
        memSpec.memTimingSpec.tRFCPB = 25;

        memSpec.memTimingSpec.tWR = 20;
        memSpec.memTimingSpec.tRP = 20;
        memSpec.memTimingSpec.tWL = 0;
        memSpec.memTimingSpec.tCK = 1;
        memSpec.memTimingSpec.tWCK = 1;
        memSpec.memTimingSpec.tREFI = 1;
        memSpec.memTimingSpec.WCKtoCK = 2;

        memSpec.memPowerSpec.resize(8);
        memSpec.memPowerSpec[0].vDDX = 1;
        memSpec.memPowerSpec[0].iDD0X = 64;
        memSpec.memPowerSpec[0].iDD2NX = 8;
        memSpec.memPowerSpec[0].iDD2PX = 6;
        memSpec.memPowerSpec[0].iDD3NX = 32;
        memSpec.memPowerSpec[0].iDD3PX = 20;
        memSpec.memPowerSpec[0].iDD4RX = 72;
        memSpec.memPowerSpec[0].iDD4WX = 72;
        memSpec.memPowerSpec[0].iDD5PBX = 30;
        memSpec.memPowerSpec[0].iBeta = memSpec.memPowerSpec[0].iDD0X;
        memSpec.memPowerSpec[0].iDD6X = 5;

        memSpec.bwParams.bwPowerFactRho = 0.333333333;

        memSpec.burstLength = 16;
        memSpec.dataRate = 2;

        ddr = std::make_unique<LPDDR5>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR5_21, Test)
{
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    };

    Rank & rank_1 = ddr->ranks[0];
    auto stats = ddr->getStats();

    // Check counter
    ASSERT_EQ(rank_1.counter.selfRefresh, 3);

    // Check global cycles count
    ASSERT_EQ(stats.rank_total[0].cycles.act, 30);
    ASSERT_EQ(stats.rank_total[0].cycles.pre, 40);
    ASSERT_EQ(stats.rank_total[0].cycles.selfRefresh, 20);
    ASSERT_EQ(stats.rank_total[0].cycles.deepSleepMode, 35);

    // Check bank specific ACT cycle count
    ASSERT_EQ(stats.bank[0].cycles.act, 30);
    ASSERT_EQ(stats.bank[1].cycles.act, 30);
    ASSERT_EQ(stats.bank[2].cycles.act, 30);
    ASSERT_EQ(stats.bank[3].cycles.act, 30);
    ASSERT_EQ(stats.bank[4].cycles.act, 30);
    ASSERT_EQ(stats.bank[5].cycles.act, 30);
    ASSERT_EQ(stats.bank[6].cycles.act, 30);
    ASSERT_EQ(stats.bank[7].cycles.act, 30);
}

TEST_F(DramPowerTest_LPDDR5_21, CalcEnergy)
{
    auto iterate_to_timestamp = [this](auto & command, const auto & container, timestamp_t timestamp) {
        while (command != container.end() && command->timestamp <= timestamp) {
            ddr->doCommand(*command);
            ++command;
        }
    };

    auto command = testPattern.begin();
    iterate_to_timestamp(command, testPattern, 125);
    auto energy = ddr->calcEnergy(125);
    auto total_energy = energy.total_energy();

    ASSERT_EQ(std::round(total_energy.E_bg_act), 4320);
    ASSERT_EQ(std::round(energy.E_bg_act_shared), 480);
    ASSERT_EQ((int)energy.bank_energy[0].E_bg_act, 480);
    ASSERT_EQ((int)energy.bank_energy[1].E_bg_act, 480);
    ASSERT_EQ((int)energy.bank_energy[2].E_bg_act, 480);
    ASSERT_EQ((int)energy.bank_energy[3].E_bg_act, 480);
    ASSERT_EQ((int)energy.bank_energy[4].E_bg_act, 480);
    ASSERT_EQ((int)energy.bank_energy[5].E_bg_act, 480);
    ASSERT_EQ((int)energy.bank_energy[6].E_bg_act, 480);
    ASSERT_EQ((int)energy.bank_energy[7].E_bg_act, 480);

    ASSERT_EQ((int)total_energy.E_bg_pre, 320);
    ASSERT_EQ((int)energy.bank_energy[0].E_bg_pre, 40);
    ASSERT_EQ((int)energy.bank_energy[1].E_bg_pre, 40);
    ASSERT_EQ((int)energy.bank_energy[2].E_bg_pre, 40);
    ASSERT_EQ((int)energy.bank_energy[3].E_bg_pre, 40);
    ASSERT_EQ((int)energy.bank_energy[4].E_bg_pre, 40);
    ASSERT_EQ((int)energy.bank_energy[5].E_bg_pre, 40);
    ASSERT_EQ((int)energy.bank_energy[6].E_bg_pre, 40);
    ASSERT_EQ((int)energy.bank_energy[7].E_bg_pre, 40);

    ASSERT_EQ(energy.E_sref, 100);
};
