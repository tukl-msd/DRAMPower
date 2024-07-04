#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr4/LPDDR4.h>

#include <memory>

using namespace DRAMPower;

class DramPowerTest_LPDDR4_12 : public ::testing::Test {
protected:
    // Test pattern
	std::vector<Command> testPattern = {
		{   0, CmdType::ACT, { 0, 0, 0} },
		{   5, CmdType::ACT, { 1, 0, 0} },
		{  10, CmdType::ACT, { 2, 0, 0} },
		{  15, CmdType::ACT, { 3, 0, 0} },
		{  20, CmdType::ACT, { 4, 0, 0} },
		{  25, CmdType::ACT, { 5, 0, 0} },
		{  30, CmdType::ACT, { 6, 0, 0} },
		{  35, CmdType::ACT, { 7, 0, 0} },
		{  35, CmdType::RD,  { 0, 0, 0} },
		{  40, CmdType::RD,  { 0, 0, 0} },
		{  40, CmdType::RD,  { 1, 0, 0} },
		{  45, CmdType::RD,  { 0, 0, 0} },
		{  45, CmdType::RD,  { 2, 0, 0} },
		{  50, CmdType::RD,  { 3, 0, 0} },
		{  55, CmdType::RD,  { 4, 0, 0} },
		{  60, CmdType::RD,  { 5, 0, 0} },
		{  65, CmdType::RD,  { 6, 0, 0} },
		{  70, CmdType::RD,  { 7, 0, 0} },
		{  75, CmdType::RD,  { 7, 0, 0} },
		{  80, CmdType::RD,  { 7, 0, 0} },
		{  85, CmdType::WR,  { 7, 0, 0} },
		{  90, CmdType::WR,  { 6, 0, 0} },
		{  95, CmdType::WR,  { 5, 0, 0} },
		{ 100, CmdType::WR,  { 4, 0, 0} },
		{ 105, CmdType::WR,  { 3, 0, 0} },
		{ 110, CmdType::WR,  { 2, 0, 0} },
		{ 110, CmdType::WR,  { 0, 0, 0} },
		{ 115, CmdType::WR,  { 1, 0, 0} },
		{ 115, CmdType::RD,  { 0, 0, 0} },
		{ 120, CmdType::WR,  { 0, 0, 0} },
		{ 120, CmdType::END_OF_SIMULATION },
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

		memSpec.memTimingSpec.tCK = 1e-9;

		memSpec.memPowerSpec.resize(8);
		memSpec.memPowerSpec[0].vDDX = 1;
		memSpec.memPowerSpec[0].iDD0X = 64e-3;
		memSpec.memPowerSpec[0].iDD3NX = 32e-3;
		memSpec.memPowerSpec[0].iDD4RX = 72e-3;
		memSpec.memPowerSpec[0].iDD4WX = 96e-3;
        memSpec.memPowerSpec[0].iBeta = memSpec.memPowerSpec[0].iDD0X;
        memSpec.bwParams.bwPowerFactRho = 1;

        memSpec.burstLength = 16;
		memSpec.dataRate = 2;

        ddr = std::make_unique<LPDDR4>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR4_12, Pattern_2)
{
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    };

	// Inspect first rank
	Rank & rank_1 = ddr->ranks[0];

	// Check global command count
	ASSERT_EQ(rank_1.commandCounter.get(CmdType::RD), 13);
	ASSERT_EQ(rank_1.commandCounter.get(CmdType::WR), 9);

	// Check bank RD command count
	ASSERT_EQ(rank_1.banks[0].counter.reads, 4);
	ASSERT_EQ(rank_1.banks[1].counter.reads, 1);
	ASSERT_EQ(rank_1.banks[2].counter.reads, 1);
	ASSERT_EQ(rank_1.banks[3].counter.reads, 1);
	ASSERT_EQ(rank_1.banks[4].counter.reads, 1);
	ASSERT_EQ(rank_1.banks[5].counter.reads, 1);
	ASSERT_EQ(rank_1.banks[6].counter.reads, 1);
	ASSERT_EQ(rank_1.banks[7].counter.reads, 3);

	// Check bank WR command count
	ASSERT_EQ(rank_1.banks[0].counter.writes, 2);
	ASSERT_EQ(rank_1.banks[1].counter.writes, 1);
	ASSERT_EQ(rank_1.banks[2].counter.writes, 1);
	ASSERT_EQ(rank_1.banks[3].counter.writes, 1);
	ASSERT_EQ(rank_1.banks[4].counter.writes, 1);
	ASSERT_EQ(rank_1.banks[5].counter.writes, 1);
	ASSERT_EQ(rank_1.banks[6].counter.writes, 1);
	ASSERT_EQ(rank_1.banks[7].counter.writes, 1);
}

TEST_F(DramPowerTest_LPDDR4_12, CalcEnergy)
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

	ASSERT_EQ(std::round(total_energy.E_RD*1e12), 4160);
	ASSERT_EQ(std::round(energy.bank_energy[0].E_RD*1e12), 1280);
	ASSERT_EQ(std::round(energy.bank_energy[1].E_RD*1e12), 320);
	ASSERT_EQ(std::round(energy.bank_energy[2].E_RD*1e12), 320);
	ASSERT_EQ(std::round(energy.bank_energy[3].E_RD*1e12), 320);
	ASSERT_EQ(std::round(energy.bank_energy[4].E_RD*1e12), 320);
	ASSERT_EQ(std::round(energy.bank_energy[5].E_RD*1e12), 320);
	ASSERT_EQ(std::round(energy.bank_energy[6].E_RD*1e12), 320);
	ASSERT_EQ(std::round(energy.bank_energy[7].E_RD*1e12), 960);

	ASSERT_EQ(std::round(total_energy.E_WR*1e12), 4608);
	ASSERT_EQ(std::round(energy.bank_energy[0].E_WR*1e12), 1024);
	ASSERT_EQ(std::round(energy.bank_energy[1].E_WR*1e12), 512);
	ASSERT_EQ(std::round(energy.bank_energy[2].E_WR*1e12), 512);
	ASSERT_EQ(std::round(energy.bank_energy[3].E_WR*1e12), 512);
	ASSERT_EQ(std::round(energy.bank_energy[4].E_WR*1e12), 512);
	ASSERT_EQ(std::round(energy.bank_energy[5].E_WR*1e12), 512);
	ASSERT_EQ(std::round(energy.bank_energy[6].E_WR*1e12), 512);
	ASSERT_EQ(std::round(energy.bank_energy[7].E_WR*1e12), 512);
}
