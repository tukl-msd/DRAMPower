#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>

#include <memory>

#include <iostream>

using namespace DRAMPower;

class DramPowerTest_LPDDR5_12 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
    // Timestamp,   Cmd,  { Bank, BG, Rank}
        {   0, CmdType::ACT, { 0, 0, 0 } },
        {  10, CmdType::ACT, { 1, 0, 0 } },
        {  15, CmdType::PRE, { 0, 0, 0 } },
        {  25, CmdType::ACT, { 3, 0, 0 } },
        {  25, CmdType::PRE, { 1, 0, 0 } },
        {  30, CmdType::ACT, { 0, 0, 0 } },
        {  35, CmdType::ACT, { 2, 0, 0 } },
        {  40, CmdType::PRE, { 1, 0, 0 } },
        {  40, CmdType::PRE, { 3, 0, 0 } },
        {  45, CmdType::PRE, { 0, 0, 0 } },
        {  50, CmdType::PRE, { 2, 0, 0 } },
        {  85, CmdType::ACT, { 7, 0, 0 } },
        { 100, CmdType::PRE, { 7, 0, 0 } },
        { 120, CmdType::ACT, { 6, 0, 0 } },
        { 125, CmdType::END_OF_SIMULATION},
    };

    // Test variables
    std::unique_ptr<DRAMPower::LPDDR5> ddr;

    virtual void SetUp()
    {
        MemSpecLPDDR5 memSpec;
        memSpec.numberOfRanks = 1;
        memSpec.numberOfBanks = 8;
        memSpec.banksPerGroup = 8;
        memSpec.numberOfBankGroups = 1;
        memSpec.burstLength = 8;
        memSpec.dataRate = 2;
        memSpec.BGroupMode = false;

		memSpec.memTimingSpec.tRAS = 10;
		memSpec.memTimingSpec.tRP = 10;
        memSpec.memTimingSpec.tCK = 1;
        memSpec.memTimingSpec.tWCK = 1;
        memSpec.memTimingSpec.WCKtoCK = 2;

		memSpec.memPowerSpec.resize(4);
		memSpec.memPowerSpec[0].vDDX = 1;
		memSpec.memPowerSpec[0].iDD0X = 64;
		memSpec.memPowerSpec[0].iDD3NX = 32;
		memSpec.memPowerSpec[0].iDD2NX = 8;
        memSpec.memPowerSpec[0].iBeta = memSpec.memPowerSpec[0].iDD0X;
		memSpec.bwParams.bwPowerFactRho = 0.333333333;


        ddr = std::make_unique<LPDDR5>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR5_12, Pattern1)
{
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    };

	// Inspect first rank
	auto & rank_1 = ddr->ranks[0];
	auto stats = ddr->getStats();

	// Check global count
	ASSERT_EQ(stats.total.cycles.act, 70);
	ASSERT_EQ(stats.total.cycles.pre, 55);

	// Check per-bank ACT count
	ASSERT_EQ(stats.bank[0].cycles.act, 30);

	ASSERT_EQ(stats.bank[1].cycles.act, 15);
	ASSERT_EQ(stats.bank[2].cycles.act, 15);
	ASSERT_EQ(stats.bank[3].cycles.act, 15);
	ASSERT_EQ(stats.bank[7].cycles.act, 15);

	ASSERT_EQ(stats.bank[6].cycles.act, 5);

	ASSERT_EQ(stats.bank[4].cycles.act, 0);
	ASSERT_EQ(stats.bank[5].cycles.act, 0);

	// Check per-bank PRE count
	ASSERT_EQ(stats.bank[0].cycles.pre, 95);

	ASSERT_EQ(stats.bank[1].cycles.pre, 110);
	ASSERT_EQ(stats.bank[2].cycles.pre, 110);
	ASSERT_EQ(stats.bank[3].cycles.pre, 110);
	ASSERT_EQ(stats.bank[7].cycles.pre, 110);

	ASSERT_EQ(stats.bank[6].cycles.pre, 120);

	ASSERT_EQ(stats.bank[4].cycles.pre, 125);
	ASSERT_EQ(stats.bank[5].cycles.pre, 125);

	// Check global command count
	ASSERT_EQ(rank_1.commandCounter.get(CmdType::ACT), 7);
	ASSERT_EQ(rank_1.commandCounter.get(CmdType::PRE), 7);

	// per-bank ACT command count
	ASSERT_EQ(stats.bank[0].counter.act, 2);

	ASSERT_EQ(stats.bank[1].counter.act, 1);
	ASSERT_EQ(stats.bank[2].counter.act, 1);
	ASSERT_EQ(stats.bank[3].counter.act, 1);
	ASSERT_EQ(stats.bank[6].counter.act, 1);
	ASSERT_EQ(stats.bank[7].counter.act, 1);

	ASSERT_EQ(stats.bank[4].counter.act, 0);
	ASSERT_EQ(stats.bank[5].counter.act, 0);

	// per-bank PRE command count
	ASSERT_EQ(stats.bank[0].counter.pre, 2);

	ASSERT_EQ(stats.bank[1].counter.pre, 1);
	ASSERT_EQ(stats.bank[2].counter.pre, 1);
	ASSERT_EQ(stats.bank[3].counter.pre, 1);
	ASSERT_EQ(stats.bank[7].counter.pre, 1);

	ASSERT_EQ(stats.bank[6].counter.pre, 0);
	ASSERT_EQ(stats.bank[4].counter.pre, 0);
	ASSERT_EQ(stats.bank[5].counter.pre, 0);
}

TEST_F(DramPowerTest_LPDDR5_12, CalcEnergy)
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

    ASSERT_EQ((int)total_energy.E_act, 2240);
    ASSERT_EQ((int)energy.bank_energy[0].E_act, 640);
    ASSERT_EQ((int)energy.bank_energy[1].E_act, 320);
    ASSERT_EQ((int)energy.bank_energy[2].E_act, 320);
    ASSERT_EQ((int)energy.bank_energy[3].E_act, 320);
    ASSERT_EQ((int)energy.bank_energy[4].E_act, 0);
    ASSERT_EQ((int)energy.bank_energy[5].E_act, 0);
    ASSERT_EQ((int)energy.bank_energy[6].E_act, 320);
    ASSERT_EQ((int)energy.bank_energy[7].E_act, 320);

    ASSERT_EQ((int)total_energy.E_pre, 3360);
    ASSERT_EQ((int)energy.bank_energy[0].E_pre, 1120);
    ASSERT_EQ((int)energy.bank_energy[1].E_pre, 560);
    ASSERT_EQ((int)energy.bank_energy[2].E_pre, 560);
    ASSERT_EQ((int)energy.bank_energy[3].E_pre, 560);
    ASSERT_EQ((int)energy.bank_energy[4].E_pre, 0);
    ASSERT_EQ((int)energy.bank_energy[5].E_pre, 0);
    ASSERT_EQ((int)energy.bank_energy[6].E_pre, 0);
    ASSERT_EQ((int)energy.bank_energy[7].E_pre, 560);

    ASSERT_EQ((int)energy.bank_energy[0].E_bg_act, 480);
    ASSERT_EQ((int)energy.bank_energy[1].E_bg_act, 240);
    ASSERT_EQ((int)energy.bank_energy[2].E_bg_act, 240);
    ASSERT_EQ((int)energy.bank_energy[3].E_bg_act, 240);
    ASSERT_EQ((int)energy.bank_energy[4].E_bg_act, 0);
    ASSERT_EQ((int)energy.bank_energy[5].E_bg_act, 0);
    ASSERT_EQ((int)energy.bank_energy[6].E_bg_act, 80);
    ASSERT_EQ((int)energy.bank_energy[7].E_bg_act, 240);
    ASSERT_EQ(std::round(energy.E_bg_act_shared), 1120);
    ASSERT_EQ(std::round(total_energy.E_bg_act), 2640);


    ASSERT_EQ((int)total_energy.E_bg_pre, 440);
    ASSERT_EQ((int)energy.bank_energy[0].E_bg_pre, 55);
    ASSERT_EQ((int)energy.bank_energy[1].E_bg_pre, 55);
    ASSERT_EQ((int)energy.bank_energy[2].E_bg_pre, 55);
    ASSERT_EQ((int)energy.bank_energy[3].E_bg_pre, 55);
    ASSERT_EQ((int)energy.bank_energy[4].E_bg_pre, 55);
    ASSERT_EQ((int)energy.bank_energy[5].E_bg_pre, 55);
    ASSERT_EQ((int)energy.bank_energy[6].E_bg_pre, 55);
    ASSERT_EQ((int)energy.bank_energy[7].E_bg_pre, 55);
}