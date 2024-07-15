#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/ddr4/DDR4.h>

#include <memory>
#include <fstream>
#include <string>

using namespace DRAMPower;

class DramPowerTest_DDR4_15 : public ::testing::Test {
protected:
    // Test pattern
	std::vector<Command> testPattern = {
		{  0,  CmdType::ACT, { 0,0,0} },
		{  5,  CmdType::PDEA },
		{ 30,  CmdType::PDXA },
		{ 40,  CmdType::PRE, {0,0,0} },
		{ 45,  CmdType::PDEP },
		{ 65,  CmdType::PDXP },
		{ 75,  CmdType::REFA },
		{ 85,  CmdType::PDEP },
		{ 90,  CmdType::PDXP },
		{125, CmdType::END_OF_SIMULATION },
	};

    // Test variables
    std::unique_ptr<DRAMPower::DDR4> ddr;

    virtual void SetUp()
    {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "ddr4.json");

        if(!f.is_open()){
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }
        json data = json::parse(f);
        MemSpecContainer memspeccontainer = data;
        MemSpecDDR4 memSpec(std::get<DRAMUtils::MemSpec::MemSpecDDR4>(memspeccontainer.memspec.getVariant()));

		memSpec.bitWidth = 16;

		memSpec.numberOfRanks = 1;
		memSpec.numberOfDevices = 1;
        memSpec.numberOfBanks = 8;
        memSpec.numberOfBankGroups = 2;

		memSpec.memTimingSpec.tRAS = 10;
		memSpec.memTimingSpec.tRTP = 10;
		memSpec.memTimingSpec.tRCD = 20;
		memSpec.memTimingSpec.tRFC = 25;

		memSpec.memTimingSpec.tWR = 20;
		memSpec.memTimingSpec.tRP = 20;
		memSpec.memTimingSpec.tWL = 0;
		memSpec.memTimingSpec.tCK = 1;

		memSpec.memPowerSpec.resize(8);
		memSpec.memPowerSpec[0].vXX = 1;
		memSpec.memPowerSpec[0].iXX0 = 64;
		memSpec.memPowerSpec[0].iXX2N = 8;
		memSpec.memPowerSpec[0].iXX2P = 6;
		memSpec.memPowerSpec[0].iXX3N = 32;
		memSpec.memPowerSpec[0].iXX3P = 20;
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

TEST_F(DramPowerTest_DDR4_15, Test)
{
	for (const auto& command : testPattern) {
		ddr->doCoreCommand(command);
	};

	Rank & rank_1 = ddr->ranks[0];
	auto stats = ddr->getStats();

	// Check bank command count: ACT
	ASSERT_EQ(stats.bank[0].counter.act, 1);

	// Check bank command count: PRE
	ASSERT_EQ(stats.bank[0].counter.pre, 1);

	// Check global cycles count
	ASSERT_EQ(stats.rank_total[0].cycles.act, 55);
	ASSERT_EQ(stats.rank_total[0].cycles.pre, 55);
	ASSERT_EQ(stats.rank_total[0].cycles.powerDownAct, 10);
	ASSERT_EQ(stats.rank_total[0].cycles.powerDownPre, 5);

	// Check bank specific ACT cycle count
	ASSERT_EQ(stats.bank[0].cycles.act, 55);
	ASSERT_EQ(stats.bank[1].cycles.act, 25);
	ASSERT_EQ(stats.bank[2].cycles.act, 25);
	ASSERT_EQ(stats.bank[3].cycles.act, 25);
	ASSERT_EQ(stats.bank[4].cycles.act, 25);
	ASSERT_EQ(stats.bank[5].cycles.act, 25);
	ASSERT_EQ(stats.bank[6].cycles.act, 25);
	ASSERT_EQ(stats.bank[7].cycles.act, 25);

	// Check bank specific PRE cycle count
	ASSERT_EQ(stats.bank[0].cycles.pre, 55);
	ASSERT_EQ(stats.bank[1].cycles.pre, 85);
	ASSERT_EQ(stats.bank[2].cycles.pre, 85);
	ASSERT_EQ(stats.bank[3].cycles.pre, 85);
	ASSERT_EQ(stats.bank[4].cycles.pre, 85);
	ASSERT_EQ(stats.bank[5].cycles.pre, 85);
	ASSERT_EQ(stats.bank[6].cycles.pre, 85);
	ASSERT_EQ(stats.bank[7].cycles.pre, 85);
}

TEST_F(DramPowerTest_DDR4_15, CalcWindow)
{
	SimulationStats window;

	auto iterate_to_timestamp = [this](auto & command, timestamp_t timestamp) {
		while (command != this->testPattern.end() && command->timestamp <= timestamp) {
			ddr->doCoreCommand(*command);
			++command;
		}

		return this->ddr->getWindowStats(timestamp);
	};

	auto command = testPattern.begin();


	// Inspect first rank
	auto & rank_1 = ddr->ranks[0];

	// Cycle 5
	window = iterate_to_timestamp(command, 5);
	ASSERT_EQ(window.rank_total[0].cycles.act, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 5);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 0);  ASSERT_EQ(window.bank[1].cycles.pre, 5);

	// Cycle 10
	window = iterate_to_timestamp(command, 10);
	ASSERT_EQ(window.rank_total[0].cycles.act, 10);
	ASSERT_EQ(window.bank[0].cycles.act, 10);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act,  0);   ASSERT_EQ(window.bank[1].cycles.pre, 10);

	// Cycle 15
	window = iterate_to_timestamp(command, 15);
	ASSERT_EQ(window.rank_total[0].cycles.act, 15);
	ASSERT_EQ(window.bank[0].cycles.act, 15);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 0);   ASSERT_EQ(window.bank[1].cycles.pre, 15);

	// Cycle 20
	window = iterate_to_timestamp(command, 20);
	ASSERT_EQ(window.rank_total[0].cycles.act, 20);
	ASSERT_EQ(window.bank[0].cycles.act, 20);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 0);   ASSERT_EQ(window.bank[1].cycles.pre, 20);

	// Cycle 25
	window = iterate_to_timestamp(command, 25);
	ASSERT_EQ(window.rank_total[0].cycles.act, 20);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 20);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 0);  ASSERT_EQ(window.bank[1].cycles.pre, 20);

	// Cycle 30
	window = iterate_to_timestamp(command, 30);
	ASSERT_EQ(window.rank_total[0].cycles.act, 20);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10);
	ASSERT_EQ(window.bank[0].cycles.act, 20);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 0);  ASSERT_EQ(window.bank[1].cycles.pre, 20);

	// Cycle 35
	window = iterate_to_timestamp(command, 35);
	ASSERT_EQ(window.rank_total[0].cycles.act, 25);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10);
	ASSERT_EQ(window.bank[0].cycles.act, 25);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act, 0);  ASSERT_EQ(window.bank[1].cycles.pre, 25);

	// Cycle 40
	window = iterate_to_timestamp(command, 40);
	ASSERT_EQ(window.rank_total[0].cycles.act, 30);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 0);
	ASSERT_EQ(window.bank[1].cycles.act,  0);  ASSERT_EQ(window.bank[1].cycles.pre, 30);

	// Cycle 45
	window = iterate_to_timestamp(command, 45);
	ASSERT_EQ(window.rank_total[0].cycles.act, 30);    ASSERT_EQ(window.rank_total[0].cycles.pre,  5);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 5);
	ASSERT_EQ(window.bank[1].cycles.act, 0);   ASSERT_EQ(window.bank[1].cycles.pre, 35);

	// Cycle 50
	window = iterate_to_timestamp(command, 50);
	ASSERT_EQ(window.rank_total[0].cycles.act, 30);    ASSERT_EQ(window.rank_total[0].cycles.pre, 10);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 10);
	ASSERT_EQ(window.bank[1].cycles.act, 0);   ASSERT_EQ(window.bank[1].cycles.pre, 40);

	// Cycle 55
	window = iterate_to_timestamp(command, 55);
	ASSERT_EQ(window.rank_total[0].cycles.act, 30);    ASSERT_EQ(window.rank_total[0].cycles.pre, 15);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 15);
	ASSERT_EQ(window.bank[1].cycles.act, 0);   ASSERT_EQ(window.bank[1].cycles.pre, 45);

	// Cycle 60
	window = iterate_to_timestamp(command, 60);
	ASSERT_EQ(window.rank_total[0].cycles.act, 30);    ASSERT_EQ(window.rank_total[0].cycles.pre, 20);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 20);
	ASSERT_EQ(window.bank[1].cycles.act, 0);   ASSERT_EQ(window.bank[1].cycles.pre, 50);

	// Cycle 65
	window = iterate_to_timestamp(command, 65);
	ASSERT_EQ(window.rank_total[0].cycles.act, 30);    ASSERT_EQ(window.rank_total[0].cycles.pre, 20);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 20);
	ASSERT_EQ(window.bank[1].cycles.act, 0);   ASSERT_EQ(window.bank[1].cycles.pre, 50);

	// Cycle 70
	window = iterate_to_timestamp(command, 70);
	ASSERT_EQ(window.rank_total[0].cycles.act, 30);    ASSERT_EQ(window.rank_total[0].cycles.pre, 25);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 25);
	ASSERT_EQ(window.bank[1].cycles.act, 0);   ASSERT_EQ(window.bank[1].cycles.pre, 55);

	// Cycle 75
	window = iterate_to_timestamp(command, 75);
	ASSERT_EQ(window.rank_total[0].cycles.act, 30);    ASSERT_EQ(window.rank_total[0].cycles.pre, 30);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 30);  ASSERT_EQ(window.bank[0].cycles.pre, 30);
	ASSERT_EQ(window.bank[1].cycles.act, 0);   ASSERT_EQ(window.bank[1].cycles.pre, 60);

	// Cycle 80
	window = iterate_to_timestamp(command, 80);
	ASSERT_EQ(window.rank_total[0].cycles.act, 35);    ASSERT_EQ(window.rank_total[0].cycles.pre, 30);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 35);  ASSERT_EQ(window.bank[0].cycles.pre, 30);
	ASSERT_EQ(window.bank[1].cycles.act, 5);   ASSERT_EQ(window.bank[1].cycles.pre, 60);

	// Cycle 85
	window = iterate_to_timestamp(command, 85);
	ASSERT_EQ(window.rank_total[0].cycles.act, 40);     ASSERT_EQ(window.rank_total[0].cycles.pre, 30);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 40);   ASSERT_EQ(window.bank[0].cycles.pre, 30);
	ASSERT_EQ(window.bank[1].cycles.act, 10);   ASSERT_EQ(window.bank[1].cycles.pre, 60);

	// Cycle 90
	window = iterate_to_timestamp(command, 90);
	ASSERT_EQ(window.rank_total[0].cycles.act, 45);     ASSERT_EQ(window.rank_total[0].cycles.pre, 30);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 45);   ASSERT_EQ(window.bank[0].cycles.pre, 30);
	ASSERT_EQ(window.bank[1].cycles.act, 15);   ASSERT_EQ(window.bank[1].cycles.pre, 60);

	// Cycle 95
	window = iterate_to_timestamp(command, 95);
	ASSERT_EQ(window.rank_total[0].cycles.act, 50);     ASSERT_EQ(window.rank_total[0].cycles.pre, 30);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 50);   ASSERT_EQ(window.bank[0].cycles.pre, 30);
	ASSERT_EQ(window.bank[1].cycles.act, 20);   ASSERT_EQ(window.bank[1].cycles.pre, 60);

	// Cycle 100
	window = iterate_to_timestamp(command, 100);
	ASSERT_EQ(window.rank_total[0].cycles.act, 55);     ASSERT_EQ(window.rank_total[0].cycles.pre, 30);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 55);   ASSERT_EQ(window.bank[0].cycles.pre, 30);
	ASSERT_EQ(window.bank[1].cycles.act, 25);   ASSERT_EQ(window.bank[1].cycles.pre, 60);

	// Cycle 105
	window = iterate_to_timestamp(command, 105);
	ASSERT_EQ(window.rank_total[0].cycles.act, 55);     ASSERT_EQ(window.rank_total[0].cycles.pre, 35);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 55);   ASSERT_EQ(window.bank[0].cycles.pre, 35);
	ASSERT_EQ(window.bank[1].cycles.act, 25);   ASSERT_EQ(window.bank[1].cycles.pre, 65);

	// Cycle 110
	window = iterate_to_timestamp(command, 110);
	ASSERT_EQ(window.rank_total[0].cycles.act, 55);     ASSERT_EQ(window.rank_total[0].cycles.pre, 40);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 55);   ASSERT_EQ(window.bank[0].cycles.pre, 40);
	ASSERT_EQ(window.bank[1].cycles.act, 25);   ASSERT_EQ(window.bank[1].cycles.pre, 70);

	// Cycle 115
	window = iterate_to_timestamp(command, 115);
	ASSERT_EQ(window.rank_total[0].cycles.act, 55);     ASSERT_EQ(window.rank_total[0].cycles.pre, 45);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 55);   ASSERT_EQ(window.bank[0].cycles.pre, 45);
	ASSERT_EQ(window.bank[1].cycles.act, 25);   ASSERT_EQ(window.bank[1].cycles.pre, 75);

	// Cycle 120
	window = iterate_to_timestamp(command, 120);
	ASSERT_EQ(window.rank_total[0].cycles.act, 55);     ASSERT_EQ(window.rank_total[0].cycles.pre, 50);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 55);   ASSERT_EQ(window.bank[0].cycles.pre, 50);
	ASSERT_EQ(window.bank[1].cycles.act, 25);   ASSERT_EQ(window.bank[1].cycles.pre, 80);

	// Cycle 125
	window = iterate_to_timestamp(command, 125);
	ASSERT_EQ(window.rank_total[0].cycles.act, 55);     ASSERT_EQ(window.rank_total[0].cycles.pre, 55);
	ASSERT_EQ(window.rank_total[0].cycles.powerDownAct, 10); ASSERT_EQ(window.rank_total[0].cycles.powerDownPre, 5);
	ASSERT_EQ(window.bank[0].cycles.act, 55);   ASSERT_EQ(window.bank[0].cycles.pre, 55);
	ASSERT_EQ(window.bank[1].cycles.act, 25);   ASSERT_EQ(window.bank[1].cycles.pre, 85);
};



TEST_F(DramPowerTest_DDR4_15, CalcEnergy)
{
	auto iterate_to_timestamp = [this](auto & command, const auto & container, timestamp_t timestamp) {
		while (command != container.end() && command->timestamp <= timestamp) {
			ddr->doCoreCommand(*command);
			++command;
		}
	};

	auto command = testPattern.begin();
	iterate_to_timestamp(command, testPattern, 125);
	auto energy = ddr->calcCoreEnergy(125);
	auto total_energy = energy.total_energy();

	ASSERT_EQ(std::round(total_energy.E_bg_act), 1340);
	ASSERT_EQ(std::round(energy.E_bg_act_shared), 880);
	ASSERT_EQ((int)energy.bank_energy[0].E_bg_act, 110);
	ASSERT_EQ((int)energy.bank_energy[1].E_bg_act, 50);
	ASSERT_EQ((int)energy.bank_energy[2].E_bg_act, 50);
	ASSERT_EQ((int)energy.bank_energy[3].E_bg_act, 50);
	ASSERT_EQ((int)energy.bank_energy[4].E_bg_act, 50);
	ASSERT_EQ((int)energy.bank_energy[5].E_bg_act, 50);
	ASSERT_EQ((int)energy.bank_energy[6].E_bg_act, 50);
	ASSERT_EQ((int)energy.bank_energy[7].E_bg_act, 50);

	ASSERT_EQ((int)total_energy.E_bg_pre, 440);
	ASSERT_EQ((int)energy.bank_energy[0].E_bg_pre, 55);
	ASSERT_EQ((int)energy.bank_energy[1].E_bg_pre, 55);
	ASSERT_EQ((int)energy.bank_energy[2].E_bg_pre, 55);
	ASSERT_EQ((int)energy.bank_energy[3].E_bg_pre, 55);
	ASSERT_EQ((int)energy.bank_energy[4].E_bg_pre, 55);
	ASSERT_EQ((int)energy.bank_energy[5].E_bg_pre, 55);
	ASSERT_EQ((int)energy.bank_energy[6].E_bg_pre, 55);
	ASSERT_EQ((int)energy.bank_energy[7].E_bg_pre, 55);

	ASSERT_EQ((int)energy.E_PDNA, 200);
	ASSERT_EQ((int)energy.E_PDNP, 30);
};
