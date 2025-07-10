#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>

#include <memory>
#include <fstream>
#include <string>

using namespace DRAMPower;

class DramPowerTest_LPDDR5_18 : public ::testing::Test {
protected:
    // Test pattern
	std::vector<Command> testPattern = {
		{  0,  CmdType::ACT, {0, 0, 0} },
		{ 20,  CmdType::PDEA },
		{ 30,  CmdType::PDXA },
		{ 40,  CmdType::PRE, {0, 0, 0} },
		{ 60,  CmdType::PDEP },
		{ 65,  CmdType::PDXP },
		{ 75,  CmdType::REFA },
		{100,  CmdType::PDEP },
		{125, CmdType::END_OF_SIMULATION },
	};

    // Test variables
    std::unique_ptr<DRAMPower::MemSpecLPDDR5> memSpec;
    std::unique_ptr<DRAMPower::LPDDR5> ddr;

    virtual void SetUp()
    {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "lpddr5.json");
        memSpec = std::make_unique<DRAMPower::MemSpecLPDDR5>(DRAMPower::MemSpecLPDDR5::from_memspec(*data));

		memSpec->numberOfRanks = 1;
        memSpec->numberOfBanks = 8;
        memSpec->numberOfBankGroups = 2;
        memSpec->banksPerGroup = 4;
		memSpec->perTwoBankOffset = 2;
        memSpec->bank_arch = MemSpecLPDDR5::MBG;
        memSpec->numberOfDevices = 1;
        memSpec->bitWidth = 16;


        memSpec->memTimingSpec.tRAS = 10;
		memSpec->memTimingSpec.tRCD = 20;
		memSpec->memTimingSpec.tRFC = 25;
		memSpec->memTimingSpec.tRFCPB = 25;

		memSpec->memTimingSpec.tWR = 20;
		memSpec->memTimingSpec.tRP = 20;
		memSpec->memTimingSpec.tWL = 0;
		memSpec->memTimingSpec.tCK = 1e-9;
        memSpec->memTimingSpec.tWCK = 1e-9;
        memSpec->memTimingSpec.tREFI = 1;
		memSpec->memTimingSpec.WCKtoCK = 2;

		memSpec->memPowerSpec[0].vDDX = 1;
		memSpec->memPowerSpec[0].iDD0X = 64e-3;
		memSpec->memPowerSpec[0].iDD2NX = 8e-3;
		memSpec->memPowerSpec[0].iDD2PX = 6e-3;
		memSpec->memPowerSpec[0].iDD3NX = 32e-3;
		memSpec->memPowerSpec[0].iDD3PX = 20e-3;
		memSpec->memPowerSpec[0].iDD4RX = 72e-3;
		memSpec->memPowerSpec[0].iDD4WX = 72e-3;
		memSpec->memPowerSpec[0].iDD5PBX = 30e-3;
        memSpec->memPowerSpec[0].iBeta = memSpec->memPowerSpec[0].iDD0X;


        memSpec->bwParams.bwPowerFactRho = 0.333333333;

		memSpec->burstLength = 16;
		memSpec->dataRate = 2;

        ddr = std::make_unique<LPDDR5>(*memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR5_18, Test)
{
	for (const auto& command : testPattern) {
		ddr->doCoreCommand(command);
	};

	auto stats = ddr->getStats();

	// Check bank command count: ACT
	ASSERT_EQ(stats.bank[0].counter.act, 1);

	// Check bank command count: PRE
	ASSERT_EQ(stats.bank[0].counter.pre, 1);

	// Check global cycles count
	ASSERT_EQ(stats.rank_total[0].cycles.activeTime(), 55);
	ASSERT_EQ(stats.rank_total[0].cycles.act, 55);
	ASSERT_EQ(stats.rank_total[0].cycles.pre, 30);
	ASSERT_EQ(stats.rank_total[0].cycles.powerDownAct, 10);
	ASSERT_EQ(stats.rank_total[0].cycles.powerDownPre, 30);

	// Check bank specific ACT cycle count
	ASSERT_EQ(stats.bank[0].cycles.act, 55); ASSERT_EQ(stats.bank[0].cycles.activeTime(), 55);
	ASSERT_EQ(stats.bank[1].cycles.act, 25); ASSERT_EQ(stats.bank[1].cycles.activeTime(), 25);
	ASSERT_EQ(stats.bank[2].cycles.act, 25); ASSERT_EQ(stats.bank[2].cycles.activeTime(), 25);
	ASSERT_EQ(stats.bank[3].cycles.act, 25); ASSERT_EQ(stats.bank[3].cycles.activeTime(), 25);
	ASSERT_EQ(stats.bank[4].cycles.act, 25); ASSERT_EQ(stats.bank[4].cycles.activeTime(), 25);
	ASSERT_EQ(stats.bank[5].cycles.act, 25); ASSERT_EQ(stats.bank[5].cycles.activeTime(), 25);
	ASSERT_EQ(stats.bank[6].cycles.act, 25); ASSERT_EQ(stats.bank[6].cycles.activeTime(), 25);
	ASSERT_EQ(stats.bank[7].cycles.act, 25); ASSERT_EQ(stats.bank[7].cycles.activeTime(), 25);

	// Check bank specific PRE cycle count
	ASSERT_EQ(stats.bank[0].cycles.pre, 30);
	ASSERT_EQ(stats.bank[1].cycles.pre, 60);
	ASSERT_EQ(stats.bank[2].cycles.pre, 60);
	ASSERT_EQ(stats.bank[3].cycles.pre, 60);
	ASSERT_EQ(stats.bank[4].cycles.pre, 60);
	ASSERT_EQ(stats.bank[5].cycles.pre, 60);
	ASSERT_EQ(stats.bank[6].cycles.pre, 60);
	ASSERT_EQ(stats.bank[7].cycles.pre, 60);
}

TEST_F(DramPowerTest_LPDDR5_18, CalcEnergy)
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
	auto total_energy = energy.aggregated_bank_energy();

    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 4560);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 880);
    ASSERT_EQ(std::round(energy.bank_energy[0].E_bg_act*1e12), 880);
    ASSERT_EQ(std::round(energy.bank_energy[1].E_bg_act*1e12), 400);
    ASSERT_EQ(std::round(energy.bank_energy[2].E_bg_act*1e12), 400);
    ASSERT_EQ(std::round(energy.bank_energy[3].E_bg_act*1e12), 400);
    ASSERT_EQ(std::round(energy.bank_energy[4].E_bg_act*1e12), 400);
    ASSERT_EQ(std::round(energy.bank_energy[5].E_bg_act*1e12), 400);
    ASSERT_EQ(std::round(energy.bank_energy[6].E_bg_act*1e12), 400);
    ASSERT_EQ(std::round(energy.bank_energy[7].E_bg_act*1e12), 400);

    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 240);
    ASSERT_EQ(std::round(energy.bank_energy[0].E_bg_pre*1e12), 30);
    ASSERT_EQ(std::round(energy.bank_energy[1].E_bg_pre*1e12), 30);
    ASSERT_EQ(std::round(energy.bank_energy[2].E_bg_pre*1e12), 30);
    ASSERT_EQ(std::round(energy.bank_energy[3].E_bg_pre*1e12), 30);
    ASSERT_EQ(std::round(energy.bank_energy[4].E_bg_pre*1e12), 30);
    ASSERT_EQ(std::round(energy.bank_energy[5].E_bg_pre*1e12), 30);
    ASSERT_EQ(std::round(energy.bank_energy[6].E_bg_pre*1e12), 30);
    ASSERT_EQ(std::round(energy.bank_energy[7].E_bg_pre*1e12), 30);

    ASSERT_EQ(std::round(energy.E_PDNA*1e12), 200);
    ASSERT_EQ(std::round(energy.E_PDNP*1e12), 180);
};
