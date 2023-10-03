#include <gtest/gtest.h>

#include <DRAMPower/command/Command.h>

#include <DRAMPower/standards/ddr4/DDR4.h>

#include <memory>

using namespace DRAMPower;

class DramPowerTest_DDR4_13 : public ::testing::Test {
protected:
    // Test pattern
	std::vector<Command> testPattern = {
		{   0, CmdType::ACT,  { 0, 0, 0 }},
		{   5, CmdType::ACT,  { 1, 0, 0 }},
		{  10, CmdType::ACT,  { 2, 0, 0 }},
		{  15, CmdType::PRE,  { 0, 0, 0 }},
		{  20, CmdType::PREA, { 0, 0, 0 }},
		{  25, CmdType::PRE,  { 1, 0, 0 }},
		{  30, CmdType::PREA, { 0, 0, 0 }},
		{  40, CmdType::ACT,  { 0, 0, 0 }},
		{  40, CmdType::ACT,  { 3, 0, 0 }},
		{  50, CmdType::PRE,  { 3, 0, 0 }},
		{ 125, CmdType::END_OF_SIMULATION },
	};

    // Test variables
    std::unique_ptr<DRAMPower::DDR4> ddr;

    virtual void SetUp()
    {
        MemSpecDDR4 memSpec;
		memSpec.numberOfRanks = 1;
        memSpec.numberOfBanks = 8;
        memSpec.numberOfBankGroups = 1;

		memSpec.memTimingSpec.tRAS = 20;
		memSpec.memTimingSpec.tRTP = 10;
		memSpec.memTimingSpec.tWR = 20;

		memSpec.memTimingSpec.tRAS = 20;
		memSpec.memTimingSpec.tRTP = 10;
		memSpec.memTimingSpec.tWR = 12;
		memSpec.memTimingSpec.tWL = 0;
		memSpec.memTimingSpec.tCK = 1;
		memSpec.memTimingSpec.tRP = 10;

		memSpec.memPowerSpec.resize(8);
		memSpec.memPowerSpec[0].vXX = 1;
		memSpec.memPowerSpec[0].iXX0 = 64;
		memSpec.memPowerSpec[0].iXX2N = 8;
		memSpec.memPowerSpec[0].iXX3N = 32;
		memSpec.memPowerSpec[0].iXX4R = 72;
		memSpec.memPowerSpec[0].iXX4W = 72;
        memSpec.memPowerSpec[0].iBeta = memSpec.memPowerSpec[0].iXX0;

		memSpec.bwParams.bwPowerFactRho = 0.333333333;

		memSpec.burstLength = 16;
		memSpec.dataRate = 2;

        ddr = std::make_unique<DDR4>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_DDR4_13, Test)
{
	for (const auto& command : testPattern) {
        ddr->doCommand(command);
    };

	Rank & rank_1 = ddr->ranks[0];
	auto stats = ddr->getStats();

	// Check bank command count: ACT
	ASSERT_EQ(stats.bank[0].counter.act, 2);
	ASSERT_EQ(stats.bank[1].counter.act, 1);
	ASSERT_EQ(stats.bank[2].counter.act, 1);
	ASSERT_EQ(stats.bank[3].counter.act, 1);
	ASSERT_EQ(stats.bank[4].counter.act, 0);

	// Check cycles count
	ASSERT_EQ(stats.rank_total[0].cycles.act, 105);
	ASSERT_EQ(stats.rank_total[0].cycles.pre, 20);

	// Check bank specific ACT cycle count
	ASSERT_EQ(stats.bank[0].cycles.act, 100);
	ASSERT_EQ(stats.bank[1].cycles.act, 15);
	ASSERT_EQ(stats.bank[2].cycles.act, 10);
	ASSERT_EQ(stats.bank[3].cycles.act, 10);
	ASSERT_EQ(stats.bank[4].cycles.act, 0);

	// Check bank specific PRE cycle count
	ASSERT_EQ(stats.bank[0].cycles.pre, 25);
	ASSERT_EQ(stats.bank[1].cycles.pre, 110);
	ASSERT_EQ(stats.bank[2].cycles.pre, 115);
	ASSERT_EQ(stats.bank[3].cycles.pre, 115);
	ASSERT_EQ(stats.bank[4].cycles.pre, 125);
}

TEST_F(DramPowerTest_DDR4_13, CalcEnergy)
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

	ASSERT_EQ((int)total_energy.E_act, 2300*2);
	ASSERT_EQ((int)energy.bank_energy[0].E_act, 920*2);
	ASSERT_EQ((int)energy.bank_energy[1].E_act, 460*2);
	ASSERT_EQ((int)energy.bank_energy[2].E_act, 460*2);
	ASSERT_EQ((int)energy.bank_energy[3].E_act, 460*2);
	ASSERT_EQ((int)energy.bank_energy[4].E_act, 0);
	ASSERT_EQ((int)energy.bank_energy[5].E_act, 0);
	ASSERT_EQ((int)energy.bank_energy[6].E_act, 0);
	ASSERT_EQ((int)energy.bank_energy[7].E_act, 0);

	ASSERT_EQ((int)total_energy.E_pre, 2240);
	ASSERT_EQ((int)energy.bank_energy[0].E_pre, 560);
	ASSERT_EQ((int)energy.bank_energy[1].E_pre, 560);
	ASSERT_EQ((int)energy.bank_energy[2].E_pre, 560);
	ASSERT_EQ((int)energy.bank_energy[3].E_pre, 560);
	ASSERT_EQ((int)energy.bank_energy[4].E_pre, 0);
	ASSERT_EQ((int)energy.bank_energy[5].E_pre, 0);
	ASSERT_EQ((int)energy.bank_energy[6].E_pre, 0);
	ASSERT_EQ((int)energy.bank_energy[7].E_pre, 0);

	ASSERT_EQ(std::round(total_energy.E_bg_act), 1950);
	ASSERT_EQ(std::round(energy.E_bg_act_shared), 1680);
	ASSERT_EQ((int)energy.bank_energy[0].E_bg_act, 200);
	ASSERT_EQ((int)energy.bank_energy[1].E_bg_act, 30);
	ASSERT_EQ((int)energy.bank_energy[2].E_bg_act, 20);
	ASSERT_EQ((int)energy.bank_energy[3].E_bg_act, 20);
	ASSERT_EQ((int)energy.bank_energy[4].E_bg_act, 0);
	ASSERT_EQ((int)energy.bank_energy[5].E_bg_act, 0);
	ASSERT_EQ((int)energy.bank_energy[6].E_bg_act, 0);
	ASSERT_EQ((int)energy.bank_energy[7].E_bg_act, 0);

	ASSERT_EQ((int)total_energy.E_bg_pre, 160);
	ASSERT_EQ((int)energy.bank_energy[0].E_bg_pre, 20);
	ASSERT_EQ((int)energy.bank_energy[1].E_bg_pre, 20);
	ASSERT_EQ((int)energy.bank_energy[2].E_bg_pre, 20);
	ASSERT_EQ((int)energy.bank_energy[3].E_bg_pre, 20);
	ASSERT_EQ((int)energy.bank_energy[4].E_bg_pre, 20);
	ASSERT_EQ((int)energy.bank_energy[5].E_bg_pre, 20);
	ASSERT_EQ((int)energy.bank_energy[6].E_bg_pre, 20);
	ASSERT_EQ((int)energy.bank_energy[7].E_bg_pre, 20);
}
