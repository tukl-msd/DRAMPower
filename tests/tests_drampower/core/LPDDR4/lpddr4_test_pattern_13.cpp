#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr4/LPDDR4.h>

#include <memory>
#include <fstream>
#include <string>

using namespace DRAMPower;

class DramPowerTest_LPDDR4_13 : public ::testing::Test {
protected:
    // Test pattern
	std::vector<Command> testPattern = {
	// Timestamp,   Cmd,  { Bank, BG, Rank}
		{   0, CmdType::ACT, { 0, 0, 0 }  },
		{  10, CmdType::RDA, { 0, 0, 0 }  },
		{  15, CmdType::ACT, { 1, 0, 0 }  },
		{  15, CmdType::ACT, { 2, 0, 0 }  },
		{  25, CmdType::WRA, { 1, 0, 0 }  },
		{  30, CmdType::PRE, { 0, 0, 0 }  },
		{  35, CmdType::RDA, { 2, 0, 0 }  },
		{  50, CmdType::ACT, { 0, 0, 0 }  },
		{  50, CmdType::ACT, { 2, 0, 0 }  },
		{  60, CmdType::WRA, { 0, 0, 0 }  },
		{  60, CmdType::RDA, { 2, 0, 0 }  },
		{  75, CmdType::ACT, { 1, 0, 0 }  },
		{  80, CmdType::ACT, { 0, 0, 0 }  },
		{  85, CmdType::WRA, { 0, 0, 0 }  },
		{  95, CmdType::RDA, { 1, 0, 0 }  },
		{ 125, CmdType::END_OF_SIMULATION },
	};

    // Test variables
    std::unique_ptr<DRAMPower::LPDDR4> ddr;

    virtual void SetUp()
    {
		auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr4.json");
        auto memSpec = DRAMPower::MemSpecLPDDR4::from_memspec(*data);

		memSpec.numberOfRanks = 1;
        memSpec.numberOfBanks = 8;
        memSpec.banksPerGroup = 8;
        memSpec.numberOfBankGroups = 1;
		memSpec.bitWidth = 16;
		memSpec.numberOfDevices = 1;

		memSpec.memTimingSpec.tRAS = 20;
		memSpec.memTimingSpec.tRTP = 10;
		memSpec.memTimingSpec.tWR = 11;
		memSpec.memTimingSpec.tWL = 0;
		memSpec.memTimingSpec.tCK = 1e-9;
        memSpec.memTimingSpec.tRP = 10;

		memSpec.memPowerSpec[0].vDDX = 1;
		memSpec.memPowerSpec[0].iDD0X = 64e-3;
		memSpec.memPowerSpec[0].iDD2NX = 8e-3;
		memSpec.memPowerSpec[0].iDD3NX = 32e-3;
		memSpec.memPowerSpec[0].iDD4RX = 72e-3;
		memSpec.memPowerSpec[0].iDD4WX = 96e-3;
        memSpec.memPowerSpec[0].iBeta = memSpec.memPowerSpec[0].iDD0X;

		memSpec.bwParams.bwPowerFactRho = 0.333333333;

		memSpec.burstLength = 16;
		memSpec.dataRate = 2;

        memSpec.prechargeOffsetRD      =   memSpec.memTimingSpec.tRTP;
        memSpec.prechargeOffsetWR      =  memSpec.memTimingSpec.tWL + memSpec.burstLength/2 + memSpec.memTimingSpec.tWR + 1;

        ddr = std::make_unique<LPDDR4>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR4_13, Test)
{
	for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    };

	Rank & rank_1 = ddr->ranks[0];
	auto stats = ddr->getStats();

	// Check global command count
	ASSERT_EQ(rank_1.commandCounter.get(CmdType::ACT), 7);
	ASSERT_EQ(rank_1.commandCounter.get(CmdType::PRE), 1);
	ASSERT_EQ(rank_1.commandCounter.get(CmdType::RDA), 4);
	ASSERT_EQ(rank_1.commandCounter.get(CmdType::WRA), 3);

	// Check bank command count: ACT
	ASSERT_EQ(stats.bank[0].counter.act, 3);
	ASSERT_EQ(stats.bank[1].counter.act, 2);
	ASSERT_EQ(stats.bank[2].counter.act, 2);
	ASSERT_EQ(stats.bank[3].counter.act, 0);
	ASSERT_EQ(stats.bank[4].counter.act, 0);
	ASSERT_EQ(stats.bank[5].counter.act, 0);
	ASSERT_EQ(stats.bank[6].counter.act, 0);
	ASSERT_EQ(stats.bank[7].counter.act, 0);

	// Check bank command count: RDA
	ASSERT_EQ(stats.bank[0].counter.readAuto, 1);
	ASSERT_EQ(stats.bank[1].counter.readAuto, 1);
	ASSERT_EQ(stats.bank[2].counter.readAuto, 2);
	ASSERT_EQ(stats.bank[3].counter.readAuto, 0);
	ASSERT_EQ(stats.bank[4].counter.readAuto, 0);
	ASSERT_EQ(stats.bank[5].counter.readAuto, 0);
	ASSERT_EQ(stats.bank[6].counter.readAuto, 0);
	ASSERT_EQ(stats.bank[7].counter.readAuto, 0);

	// Check bank command count: WRA
	ASSERT_EQ(stats.bank[0].counter.writeAuto, 2);
	ASSERT_EQ(stats.bank[1].counter.writeAuto, 1);
	ASSERT_EQ(stats.bank[2].counter.writeAuto, 0);
	ASSERT_EQ(stats.bank[3].counter.writeAuto, 0);
	ASSERT_EQ(stats.bank[4].counter.writeAuto, 0);
	ASSERT_EQ(stats.bank[5].counter.writeAuto, 0);
	ASSERT_EQ(stats.bank[6].counter.writeAuto, 0);
	ASSERT_EQ(stats.bank[7].counter.writeAuto, 0);

	// Check cycles count
	ASSERT_EQ(stats.rank_total[0].cycles.act, 100);
	ASSERT_EQ(stats.rank_total[0].cycles.pre, 25);

	// Check bank specific ACT cycle count
	ASSERT_EQ(stats.bank[0].cycles.act, 75);
	ASSERT_EQ(stats.bank[1].cycles.act, 60);
	ASSERT_EQ(stats.bank[2].cycles.act, 50);
	ASSERT_EQ(stats.bank[3].cycles.act, 0);
	ASSERT_EQ(stats.bank[4].cycles.act, 0);
	ASSERT_EQ(stats.bank[5].cycles.act, 0);
	ASSERT_EQ(stats.bank[6].cycles.act, 0);
	ASSERT_EQ(stats.bank[7].cycles.act, 0);

	// Check bank specific PRE cycle count
	ASSERT_EQ(stats.bank[0].cycles.pre, 50);
	ASSERT_EQ(stats.bank[1].cycles.pre, 65);
	ASSERT_EQ(stats.bank[2].cycles.pre, 75);
	ASSERT_EQ(stats.bank[3].cycles.pre, 125);
	ASSERT_EQ(stats.bank[4].cycles.pre, 125);
	ASSERT_EQ(stats.bank[5].cycles.pre, 125);
	ASSERT_EQ(stats.bank[6].cycles.pre, 125);
	ASSERT_EQ(stats.bank[7].cycles.pre, 125);
}

TEST_F(DramPowerTest_LPDDR4_13, CalcEnergy)
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

	ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 4560);
	ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 1600);
	ASSERT_EQ(std::round(energy.bank_energy[0].E_bg_act*1e12), 1200);
	ASSERT_EQ(std::round(energy.bank_energy[1].E_bg_act*1e12), 960);
	ASSERT_EQ(std::round(energy.bank_energy[2].E_bg_act*1e12), 800);
	ASSERT_EQ(std::round(energy.bank_energy[3].E_bg_act*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[4].E_bg_act*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[5].E_bg_act*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[6].E_bg_act*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[7].E_bg_act*1e12), 0);

    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 200);
    ASSERT_EQ(std::round(energy.bank_energy[0].E_bg_pre*1e12), 25);
    ASSERT_EQ(std::round(energy.bank_energy[1].E_bg_pre*1e12), 25);
    ASSERT_EQ(std::round(energy.bank_energy[2].E_bg_pre*1e12), 25);
    ASSERT_EQ(std::round(energy.bank_energy[3].E_bg_pre*1e12), 25);
    ASSERT_EQ(std::round(energy.bank_energy[4].E_bg_pre*1e12), 25);
    ASSERT_EQ(std::round(energy.bank_energy[5].E_bg_pre*1e12), 25);
    ASSERT_EQ(std::round(energy.bank_energy[6].E_bg_pre*1e12), 25);
    ASSERT_EQ(std::round(energy.bank_energy[7].E_bg_pre*1e12), 25);

	ASSERT_EQ(std::round(total_energy.E_RDA*1e12), 1280);
	ASSERT_EQ(std::round(energy.bank_energy[0].E_RDA*1e12), 320);
	ASSERT_EQ(std::round(energy.bank_energy[1].E_RDA*1e12), 320);
	ASSERT_EQ(std::round(energy.bank_energy[2].E_RDA*1e12), 640);
	ASSERT_EQ(std::round(energy.bank_energy[3].E_RDA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[4].E_RDA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[5].E_RDA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[6].E_RDA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[7].E_RDA*1e12), 0);

	ASSERT_EQ(std::round(total_energy.E_WRA*1e12), 1536);
	ASSERT_EQ(std::round(energy.bank_energy[0].E_WRA*1e12), 1024);
	ASSERT_EQ(std::round(energy.bank_energy[1].E_WRA*1e12), 512);
	ASSERT_EQ(std::round(energy.bank_energy[2].E_WRA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[3].E_WRA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[4].E_WRA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[5].E_WRA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[6].E_WRA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[7].E_WRA*1e12), 0);

	ASSERT_EQ(std::round(total_energy.E_pre_RDA*1e12), 2240);
	ASSERT_EQ(std::round(energy.bank_energy[0].E_pre_RDA*1e12), 560);
	ASSERT_EQ(std::round(energy.bank_energy[1].E_pre_RDA*1e12), 560);
	ASSERT_EQ(std::round(energy.bank_energy[2].E_pre_RDA*1e12), 1120);
	ASSERT_EQ(std::round(energy.bank_energy[3].E_pre_RDA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[4].E_pre_RDA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[5].E_pre_RDA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[6].E_pre_RDA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[7].E_pre_RDA*1e12), 0);

	ASSERT_EQ(std::round(total_energy.E_pre_WRA*1e12), 1120+560);
	ASSERT_EQ(std::round(energy.bank_energy[0].E_pre_WRA*1e12), 1120);
	ASSERT_EQ(std::round(energy.bank_energy[1].E_pre_WRA*1e12), 560);
	ASSERT_EQ(std::round(energy.bank_energy[2].E_pre_WRA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[3].E_pre_WRA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[4].E_pre_WRA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[5].E_pre_WRA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[6].E_pre_WRA*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[7].E_pre_WRA*1e12), 0);
}
