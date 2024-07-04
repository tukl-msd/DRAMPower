#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr4/LPDDR4.h>

#include <memory>

using namespace DRAMPower;

class DramPowerTest_LPDDR4_15 : public ::testing::Test {
protected:
    // Test pattern
	std::vector<Command> testPattern = {
		{  0, CmdType::ACT,  { 0, 0, 0}},
		{  0, CmdType::ACT,  { 1, 0, 0}},
		{  0, CmdType::ACT,  { 2, 0, 0}},
		{ 15, CmdType::PRE,  { 0, 0, 0}},
		{ 20, CmdType::REFB, { 0, 0, 0}},
		{ 25, CmdType::PRE,  { 1, 0, 0}},
		{ 30, CmdType::RDA,  { 2, 0, 0}},
		{ 50, CmdType::REFB, { 2, 0, 0}},
		{ 60, CmdType::ACT,  { 1, 0, 0}},
		{ 80, CmdType::PRE,  { 1, 0, 0}},
		{ 85, CmdType::REFB, { 1, 0, 0}},
		{ 85, CmdType::REFB, { 0, 0, 0}},
		{ 125, CmdType::END_OF_SIMULATION },
	};

    // Test variables
    std::unique_ptr<DRAMPower::LPDDR4> ddr;

    virtual void SetUp()
    {
        MemSpecLPDDR4 memSpec;
		memSpec.numberOfRanks = 1;
        memSpec.numberOfBanks = 8;
        memSpec.banksPerGroup = 8;
        memSpec.numberOfBankGroups = 1;
		memSpec.bitWidth = 16;
		memSpec.numberOfDevices = 1;

		memSpec.memTimingSpec.tRAS = 10;
		memSpec.memTimingSpec.tRTP = 10;
        memSpec.memTimingSpec.tRFCPB = 25;
		memSpec.memTimingSpec.tWR = 20;
		memSpec.memTimingSpec.tWL = 0;
		memSpec.memTimingSpec.tCK = 1e-9;
        memSpec.memTimingSpec.tRP = 10;
		memSpec.memTimingSpec.tREFI = 1;


        memSpec.memPowerSpec.resize(8);
		memSpec.memPowerSpec[0].vDDX = 1;
		memSpec.memPowerSpec[0].iDD0X = 64e-3;
		memSpec.memPowerSpec[0].iDD2NX = 8e-3;
		memSpec.memPowerSpec[0].iDD3NX = 32e-3;
		memSpec.memPowerSpec[0].iDD4RX = 72e-3;
		memSpec.memPowerSpec[0].iDD4WX = 72e-3;
		memSpec.memPowerSpec[0].iDD5PBX = 6008e-3;
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

TEST_F(DramPowerTest_LPDDR4_15, Test)
{
	for (const auto& command : testPattern) {
        ddr->doCommand(command);
    };

	Rank & rank_1 = ddr->ranks[0];
	auto stats = ddr->getStats();

	// Check bank command count: ACT
	ASSERT_EQ(stats.bank[0].counter.act, 1);
	ASSERT_EQ(stats.bank[1].counter.act, 2);
	ASSERT_EQ(stats.bank[2].counter.act, 1);
	ASSERT_EQ(stats.bank[3].counter.act, 0);

	// Check bank command count: PRE
	ASSERT_EQ(stats.bank[0].counter.pre, 1);
	ASSERT_EQ(stats.bank[1].counter.pre, 2);
	ASSERT_EQ(stats.bank[2].counter.pre, 1);
	ASSERT_EQ(stats.bank[3].counter.pre, 0);

	// Check bank command count: REFPB
	ASSERT_EQ(stats.bank[0].counter.refPerBank, 2);
	ASSERT_EQ(stats.bank[1].counter.refPerBank, 1);
	ASSERT_EQ(stats.bank[2].counter.refPerBank, 1);
	ASSERT_EQ(stats.bank[3].counter.refPerBank, 0);

	// Check global cycles count
	ASSERT_EQ(stats.rank_total[0].cycles.act, 100);
	ASSERT_EQ(stats.rank_total[0].cycles.pre, 25);

	// Check bank specific ACT cycle count
	ASSERT_EQ(stats.bank[0].cycles.act, 65);
	ASSERT_EQ(stats.bank[1].cycles.act, 70);
	ASSERT_EQ(stats.bank[2].cycles.act, 65);
	ASSERT_EQ(stats.bank[3].cycles.act, 0);

	// Check bank specific PRE cycle count
	ASSERT_EQ(stats.bank[0].cycles.pre, 60);
	ASSERT_EQ(stats.bank[1].cycles.pre, 55);
	ASSERT_EQ(stats.bank[2].cycles.pre, 60);
	ASSERT_EQ(stats.bank[3].cycles.pre, 125);
}

TEST_F(DramPowerTest_LPDDR4_15, CalcEnergy)
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

	ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 4800);
	ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 1600);
	ASSERT_EQ(std::round(energy.bank_energy[0].E_bg_act*1e12), 1040);
	ASSERT_EQ(std::round(energy.bank_energy[1].E_bg_act*1e12), 1120);
	ASSERT_EQ(std::round(energy.bank_energy[2].E_bg_act*1e12), 1040);
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

	ASSERT_EQ(std::round(total_energy.E_ref_PB*1e12), 600);
	ASSERT_EQ(std::round(energy.bank_energy[0].E_ref_PB*1e12), 300);
	ASSERT_EQ(std::round(energy.bank_energy[1].E_ref_PB*1e12), 150);
	ASSERT_EQ(std::round(energy.bank_energy[2].E_ref_PB*1e12), 150);
	ASSERT_EQ(std::round(energy.bank_energy[3].E_ref_PB*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[4].E_ref_PB*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[5].E_ref_PB*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[6].E_ref_PB*1e12), 0);
	ASSERT_EQ(std::round(energy.bank_energy[7].E_ref_PB*1e12), 0);
};
