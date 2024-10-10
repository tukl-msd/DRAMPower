#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/ddr5/DDR5.h>

#include <memory>
#include <fstream>
#include <string>

using namespace DRAMPower;

class DramPowerTest_DDR5_16 : public ::testing::Test {
protected:
    // Test pattern
	std::vector<Command> testPattern = {
		{  0, CmdType::REFSB, {0, 0, 0 }},
		{ 10, CmdType::REFSB, {1, 0, 0 }},
		{ 45, CmdType::REFSB, {2, 0, 0 }},
		{ 50, CmdType::ACT  , {7, 0, 0 }},
		{ 55, CmdType::ACT  , {3, 0, 0 }},
		{ 70, CmdType::PRE  , {7, 0, 0 }},
		{ 80, CmdType::PRESB, {3, 0, 0 }},
		{ 95, CmdType::REFSB, {3, 0, 0 }},
		{100, CmdType::REFSB, {0, 0, 0 }},
		{125, CmdType::END_OF_SIMULATION },
	};

    // Test variables
    std::unique_ptr<DRAMPower::DDR5> ddr;

    virtual void SetUp()
    {
		auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "ddr5.json");
        auto memSpec = DRAMPower::MemSpecDDR5::from_memspec(*data);

		memSpec.bitWidth = 16;
		memSpec.numberOfDevices = 1;
        memSpec.numberOfRanks = 1;
        memSpec.numberOfBanks = 8;
        memSpec.numberOfBankGroups = 2;
        memSpec.banksPerGroup = 4;

        memSpec.memTimingSpec.tRAS = 10;
        memSpec.memTimingSpec.tRTP = 10;
        memSpec.memTimingSpec.tRFCsb = 25;

        memSpec.memTimingSpec.tWR = 20;
        memSpec.memTimingSpec.tWL = 0;
        memSpec.memTimingSpec.tCK = 1;
        memSpec.memTimingSpec.tRP = 10;

        memSpec.memPowerSpec[0].vXX = 1;
        memSpec.memPowerSpec[0].iXX0 = 64;
        memSpec.memPowerSpec[0].iXX2N = 8;
        memSpec.memPowerSpec[0].iXX3N = 32;
        memSpec.memPowerSpec[0].iXX4R = 72;
        memSpec.memPowerSpec[0].iXX4W = 72;
        memSpec.memPowerSpec[0].iXX5C = 28;
        memSpec.memPowerSpec[0].iBeta = memSpec.memPowerSpec[0].iXX0;


        memSpec.bwParams.bwPowerFactRho = 0.333333333;

        memSpec.burstLength = 16;
        memSpec.dataRate = 2;


        ddr = std::make_unique<DDR5>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_DDR5_16, Test)
{
	for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    };

	auto stats = ddr->getStats();

	// Check bank command count: ACT
	ASSERT_EQ(stats.bank[3].counter.act, 1);
	ASSERT_EQ(stats.bank[7].counter.act, 1);

	// Check bank command count: PRE
	ASSERT_EQ(stats.bank[3].counter.pre, 1);
	ASSERT_EQ(stats.bank[7].counter.pre, 1);

	// Check bank command count: REFSB
	ASSERT_EQ(stats.bank[0].counter.refSameBank, 2);
    ASSERT_EQ(stats.bank[4].counter.refSameBank, 2);

	ASSERT_EQ(stats.bank[1].counter.refSameBank, 1);
	ASSERT_EQ(stats.bank[2].counter.refSameBank, 1);
	ASSERT_EQ(stats.bank[3].counter.refSameBank, 1);
	ASSERT_EQ(stats.bank[5].counter.refSameBank, 1);
	ASSERT_EQ(stats.bank[6].counter.refSameBank, 1);
	ASSERT_EQ(stats.bank[7].counter.refSameBank, 1);

	// Check global cycles count
	ASSERT_EQ(stats.rank_total[0].cycles.act, 100);
	ASSERT_EQ(stats.rank_total[0].cycles.pre, 25);

	// Check bank specific ACT cycle count
	ASSERT_EQ(stats.bank[0].cycles.act, 50);
	ASSERT_EQ(stats.bank[4].cycles.act, 50);
	ASSERT_EQ(stats.bank[3].cycles.act, 50);
	ASSERT_EQ(stats.bank[1].cycles.act, 25);
	ASSERT_EQ(stats.bank[2].cycles.act, 25);
	ASSERT_EQ(stats.bank[5].cycles.act, 25);
	ASSERT_EQ(stats.bank[6].cycles.act, 25);
	ASSERT_EQ(stats.bank[7].cycles.act, 45);

	// Check bank specific PRE cycle count
	ASSERT_EQ(stats.bank[0].cycles.pre, 75);
	ASSERT_EQ(stats.bank[3].cycles.pre, 75);
	ASSERT_EQ(stats.bank[4].cycles.pre, 75);
	ASSERT_EQ(stats.bank[1].cycles.pre, 100);
	ASSERT_EQ(stats.bank[2].cycles.pre, 100);
	ASSERT_EQ(stats.bank[5].cycles.pre, 100);
	ASSERT_EQ(stats.bank[6].cycles.pre, 100);
	ASSERT_EQ(stats.bank[7].cycles.pre, 80);
}

TEST_F(DramPowerTest_DDR5_16, CalcEnergy)
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

	ASSERT_EQ(std::round(total_energy.E_bg_act), 2190);
	ASSERT_EQ(std::round(energy.E_bg_act_shared), 1600);
	ASSERT_EQ((int)energy.bank_energy[0].E_bg_act, 100);
	ASSERT_EQ((int)energy.bank_energy[1].E_bg_act, 50);
	ASSERT_EQ((int)energy.bank_energy[2].E_bg_act, 50);
	ASSERT_EQ((int)energy.bank_energy[3].E_bg_act, 100);
	ASSERT_EQ((int)energy.bank_energy[4].E_bg_act, 100);
	ASSERT_EQ((int)energy.bank_energy[5].E_bg_act, 50);
	ASSERT_EQ((int)energy.bank_energy[6].E_bg_act, 50);
	ASSERT_EQ((int)energy.bank_energy[7].E_bg_act, 90);

	ASSERT_EQ((int)total_energy.E_bg_pre, 200);
	ASSERT_EQ((int)energy.bank_energy[0].E_bg_pre, 25);
	ASSERT_EQ((int)energy.bank_energy[1].E_bg_pre, 25);
	ASSERT_EQ((int)energy.bank_energy[2].E_bg_pre, 25);
	ASSERT_EQ((int)energy.bank_energy[3].E_bg_pre, 25);
	ASSERT_EQ((int)energy.bank_energy[4].E_bg_pre, 25);
	ASSERT_EQ((int)energy.bank_energy[5].E_bg_pre, 25);
	ASSERT_EQ((int)energy.bank_energy[6].E_bg_pre, 25);
	ASSERT_EQ((int)energy.bank_energy[7].E_bg_pre, 25);

	ASSERT_EQ((int)total_energy.E_pre, 1120);
	ASSERT_EQ((int)energy.bank_energy[3].E_pre, 560);
	ASSERT_EQ((int)energy.bank_energy[7].E_pre, 560);

    ASSERT_EQ((int)total_energy.E_ref_SB, 1000);
	ASSERT_EQ((int)energy.bank_energy[0].E_ref_SB, 200);
	ASSERT_EQ((int)energy.bank_energy[1].E_ref_SB, 100);
	ASSERT_EQ((int)energy.bank_energy[2].E_ref_SB, 100);
	ASSERT_EQ((int)energy.bank_energy[3].E_ref_SB, 100);
	ASSERT_EQ((int)energy.bank_energy[4].E_ref_SB, 200);
	ASSERT_EQ((int)energy.bank_energy[5].E_ref_SB, 100);
	ASSERT_EQ((int)energy.bank_energy[6].E_ref_SB, 100);
	ASSERT_EQ((int)energy.bank_energy[7].E_ref_SB, 100);
};
