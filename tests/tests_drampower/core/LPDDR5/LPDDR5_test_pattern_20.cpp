#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>

#include <memory>

using namespace DRAMPower;

class DramPowerTest_LPDDR5_20 : public ::testing::Test {
protected:
    // Test pattern
	std::vector<Command> testPattern = {
		{  0, CmdType::REFA },
		{  5, CmdType::PDEP },
		{ 15, CmdType::PDXP }, 
		{ 30, CmdType::REFB, { 0,0,0} },
		{ 40, CmdType::ACT, { 1,0,0,}},
		{ 45, CmdType::PDEA },
		{ 75, CmdType::PDXA },
		{125, CmdType::END_OF_SIMULATION },
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
        memSpec.BGroupMode = true;


        memSpec.memTimingSpec.tRAS = 10;
		memSpec.memTimingSpec.tRTP = 10;
		memSpec.memTimingSpec.tRCD = 20;
		memSpec.memTimingSpec.tRFC = 25;
		memSpec.memTimingSpec.tRFCPB = 25;

		memSpec.memTimingSpec.tWR = 20;
		memSpec.memTimingSpec.tRP = 20;
		memSpec.memTimingSpec.tWL = 0;
		memSpec.memTimingSpec.tCK = 1;
        memSpec.memTimingSpec.tWCK = 1;
        memSpec.memTimingSpec.tREFI = 1;
		memSpec.memTimingSpec.WCKtoCK = 2;

		memSpec.memPowerSpec.resize(1);
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


        memSpec.bwParams.bwPowerFactRho = 0.333333333;

		memSpec.burstLength = 16;
		memSpec.dataRate = 2;

        ddr = std::make_unique<LPDDR5>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR5_20, Test)
{
	for (const auto& command : testPattern) {
		ddr->doCommand(command);
	};

	Rank & rank_1 = ddr->ranks[0];
	auto stats = ddr->getStats();

	// Check bank command count: ACT
	ASSERT_EQ(stats.bank[1].counter.act, 1);

	// Check global cycles count
	ASSERT_EQ(stats.total.cycles.act, 105);
	ASSERT_EQ(stats.total.cycles.pre, 5);
	ASSERT_EQ(stats.total.cycles.powerDownAct, 15);
	ASSERT_EQ(stats.total.cycles.powerDownPre, 0);

	// Check bank specific ACT cycle count
	ASSERT_EQ(stats.bank[0].cycles.act, 50);
	ASSERT_EQ(stats.bank[1].cycles.act, 95);
	ASSERT_EQ(stats.bank[2].cycles.act, 25);
	ASSERT_EQ(stats.bank[3].cycles.act, 25);
	ASSERT_EQ(stats.bank[4].cycles.act, 25);
	ASSERT_EQ(stats.bank[5].cycles.act, 25);
	ASSERT_EQ(stats.bank[6].cycles.act, 25);
	ASSERT_EQ(stats.bank[7].cycles.act, 25);

	// Check bank specific PRE cycle count
	ASSERT_EQ(stats.bank[0].cycles.pre, 60);
	ASSERT_EQ(stats.bank[1].cycles.pre, 15);
	ASSERT_EQ(stats.bank[2].cycles.pre, 85);
	ASSERT_EQ(stats.bank[3].cycles.pre, 85);
	ASSERT_EQ(stats.bank[4].cycles.pre, 85);
	ASSERT_EQ(stats.bank[5].cycles.pre, 85);
	ASSERT_EQ(stats.bank[6].cycles.pre, 85);
	ASSERT_EQ(stats.bank[7].cycles.pre, 85);
}

TEST_F(DramPowerTest_LPDDR5_20, CalcWindow)
{
	SimulationStats window;

	auto iterate_to_timestamp = [this](auto & command, timestamp_t timestamp) {
		while (command != this->testPattern.end() && command->timestamp <= timestamp) {
			ddr->doCommand(*command);
			++command;
		}

		return this->ddr->getWindowStats(timestamp);
	};

	auto command = testPattern.begin();


	// Inspect first rank
	auto & rank_1 = ddr->ranks[0];

	// Cycle 5
	window = iterate_to_timestamp(command, 5);
	ASSERT_EQ(window.total.cycles.act, 5);    ASSERT_EQ(window.total.cycles.pre, 0);
	ASSERT_EQ(window.bank[0].cycles.act, 5);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 5);  ASSERT_EQ(window.bank[1].cycles.pre, 0);
	ASSERT_EQ(window.bank[2].cycles.act, 5);  ASSERT_EQ(window.bank[2].cycles.pre, 0);
	ASSERT_EQ(window.total.cycles.powerDownAct, 0); ASSERT_EQ(window.total.cycles.powerDownPre, 0);

	// Cycle 25
	window = iterate_to_timestamp(command, 25);
	ASSERT_EQ(window.total.cycles.act, 25);    ASSERT_EQ(window.total.cycles.pre, 0);
	ASSERT_EQ(window.bank[0].cycles.act, 25);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 25);  ASSERT_EQ(window.bank[1].cycles.pre, 0);
	ASSERT_EQ(window.bank[2].cycles.act, 25);  ASSERT_EQ(window.bank[2].cycles.pre, 0);
	ASSERT_EQ(window.total.cycles.powerDownAct, 0); ASSERT_EQ(window.total.cycles.powerDownPre, 0);

	// Cycle 30
	window = iterate_to_timestamp(command, 30);
	ASSERT_EQ(window.total.cycles.act, 25);    ASSERT_EQ(window.total.cycles.pre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 25);  ASSERT_EQ(window.bank[0].cycles.pre, 5);
	ASSERT_EQ(window.bank[1].cycles.act, 25);  ASSERT_EQ(window.bank[1].cycles.pre, 5);
	ASSERT_EQ(window.bank[2].cycles.act, 25);  ASSERT_EQ(window.bank[2].cycles.pre, 5);
	ASSERT_EQ(window.total.cycles.powerDownAct, 0); ASSERT_EQ(window.total.cycles.powerDownPre, 0);

	// Cycle 35
	window = iterate_to_timestamp(command, 35);  
	ASSERT_EQ(window.total.cycles.act,   30);  ASSERT_EQ(window.total.cycles.pre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 5);
	ASSERT_EQ(window.bank[1].cycles.act, 25);  ASSERT_EQ(window.bank[1].cycles.pre, 10);
	ASSERT_EQ(window.bank[2].cycles.act, 25);  ASSERT_EQ(window.bank[2].cycles.pre, 10);
	ASSERT_EQ(window.total.cycles.powerDownAct, 0); ASSERT_EQ(window.total.cycles.powerDownPre, 0);

	// Cycle 40
	window = iterate_to_timestamp(command, 40);  
	ASSERT_EQ(window.total.cycles.act, 35);	   ASSERT_EQ(window.total.cycles.pre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 35);  ASSERT_EQ(window.bank[0].cycles.pre, 5 );
	ASSERT_EQ(window.bank[1].cycles.act, 25);  ASSERT_EQ(window.bank[1].cycles.pre, 15);
	ASSERT_EQ(window.bank[2].cycles.act, 25);  ASSERT_EQ(window.bank[2].cycles.pre, 15);
	ASSERT_EQ(window.total.cycles.powerDownAct, 0); ASSERT_EQ(window.total.cycles.powerDownPre, 0);

	// Cycle 50
	window = iterate_to_timestamp(command, 50);  
	ASSERT_EQ(window.total.cycles.act, 45);	   ASSERT_EQ(window.total.cycles.pre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 45);  ASSERT_EQ(window.bank[0].cycles.pre, 5);
	ASSERT_EQ(window.bank[1].cycles.act, 35);  ASSERT_EQ(window.bank[1].cycles.pre, 15);
	ASSERT_EQ(window.bank[2].cycles.act, 25);  ASSERT_EQ(window.bank[2].cycles.pre, 25);
	ASSERT_EQ(window.total.cycles.powerDownAct, 0); ASSERT_EQ(window.total.cycles.powerDownPre, 0);

	// Cycle 55
	window = iterate_to_timestamp(command, 55);  
	ASSERT_EQ(window.total.cycles.act, 50);	   ASSERT_EQ(window.total.cycles.pre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 50);  ASSERT_EQ(window.bank[0].cycles.pre, 5);
	ASSERT_EQ(window.bank[1].cycles.act, 40);  ASSERT_EQ(window.bank[1].cycles.pre, 15);
	ASSERT_EQ(window.bank[2].cycles.act, 25);  ASSERT_EQ(window.bank[2].cycles.pre, 30);
	ASSERT_EQ(window.total.cycles.powerDownAct, 0); ASSERT_EQ(window.total.cycles.powerDownPre, 0);

	// Cycle 60
	window = iterate_to_timestamp(command, 60);  
	ASSERT_EQ(window.total.cycles.act, 55);	   ASSERT_EQ(window.total.cycles.pre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 50);  ASSERT_EQ(window.bank[0].cycles.pre, 10);
	ASSERT_EQ(window.bank[1].cycles.act, 45);  ASSERT_EQ(window.bank[1].cycles.pre, 15);
	ASSERT_EQ(window.bank[2].cycles.act, 25);  ASSERT_EQ(window.bank[2].cycles.pre, 35);
	ASSERT_EQ(window.total.cycles.powerDownAct, 0); ASSERT_EQ(window.total.cycles.powerDownPre, 0);

	// Cycle 75
	window = iterate_to_timestamp(command, 75);
	ASSERT_EQ(window.total.cycles.act, 55);	   ASSERT_EQ(window.total.cycles.pre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 50);  ASSERT_EQ(window.bank[0].cycles.pre, 10);
	ASSERT_EQ(window.bank[1].cycles.act, 45);  ASSERT_EQ(window.bank[1].cycles.pre, 15);
	ASSERT_EQ(window.bank[2].cycles.act, 25);  ASSERT_EQ(window.bank[2].cycles.pre, 35);
	ASSERT_EQ(window.total.cycles.powerDownAct, 15); ASSERT_EQ(window.total.cycles.powerDownPre, 0);

	// Cycle 95
	window = iterate_to_timestamp(command, 95);
	ASSERT_EQ(window.total.cycles.act, 75);	   ASSERT_EQ(window.total.cycles.pre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 50);  ASSERT_EQ(window.bank[0].cycles.pre, 30);
	ASSERT_EQ(window.bank[1].cycles.act, 65);  ASSERT_EQ(window.bank[1].cycles.pre, 15);
	ASSERT_EQ(window.bank[2].cycles.act, 25);  ASSERT_EQ(window.bank[2].cycles.pre, 55);
	ASSERT_EQ(window.total.cycles.powerDownAct, 15); ASSERT_EQ(window.total.cycles.powerDownPre, 0);

	// Cycle 125
	window = iterate_to_timestamp(command, 125);
	ASSERT_EQ(window.total.cycles.act, 105);	   ASSERT_EQ(window.total.cycles.pre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 50);  ASSERT_EQ(window.bank[0].cycles.pre, 60);
	ASSERT_EQ(window.bank[1].cycles.act, 95);  ASSERT_EQ(window.bank[1].cycles.pre, 15);
	ASSERT_EQ(window.bank[2].cycles.act, 25);  ASSERT_EQ(window.bank[2].cycles.pre, 85);
	ASSERT_EQ(window.total.cycles.powerDownAct, 15); ASSERT_EQ(window.total.cycles.powerDownPre, 0);
};


TEST_F(DramPowerTest_LPDDR5_20, CalcEnergy)
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

    ASSERT_EQ(std::round(total_energy.E_bg_act), 6400);
    ASSERT_EQ(std::round(energy.E_bg_act_shared), 1680);
    ASSERT_EQ((int)energy.bank_energy[0].E_bg_act, 800);
    ASSERT_EQ((int)energy.bank_energy[1].E_bg_act, 1520);
    ASSERT_EQ((int)energy.bank_energy[2].E_bg_act, 400);
    ASSERT_EQ((int)energy.bank_energy[3].E_bg_act, 400);
    ASSERT_EQ((int)energy.bank_energy[4].E_bg_act, 400);
    ASSERT_EQ((int)energy.bank_energy[5].E_bg_act, 400);
    ASSERT_EQ((int)energy.bank_energy[6].E_bg_act, 400);
    ASSERT_EQ((int)energy.bank_energy[7].E_bg_act, 400);

    ASSERT_EQ((int)total_energy.E_bg_pre, 40);
    ASSERT_EQ((int)energy.bank_energy[0].E_bg_pre, 5);
    ASSERT_EQ((int)energy.bank_energy[1].E_bg_pre, 5);
    ASSERT_EQ((int)energy.bank_energy[2].E_bg_pre, 5);
    ASSERT_EQ((int)energy.bank_energy[3].E_bg_pre, 5);
    ASSERT_EQ((int)energy.bank_energy[4].E_bg_pre, 5);
    ASSERT_EQ((int)energy.bank_energy[5].E_bg_pre, 5);
    ASSERT_EQ((int)energy.bank_energy[6].E_bg_pre, 5);
    ASSERT_EQ((int)energy.bank_energy[7].E_bg_pre, 5);

    ASSERT_EQ((int)energy.E_PDNA, 300);
    ASSERT_EQ((int)energy.E_PDNP, 0);
};
