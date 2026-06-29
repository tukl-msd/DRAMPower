#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>
#include <DRAMPower/standards/test_accessor.h>

#include <memory>
#include <fstream>
#include <string>

using namespace DRAMPower;

class DramPowerTest_LPDDR5_22 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   5 + 1'000'000,  CmdType::SREFEN },
            {  15 + 1'000'000,  CmdType::SREFEX },
            {  25 + 1'000'000,  CmdType::SREFEN },
            {  35 + 1'000'000,  CmdType::DSMEN  },
            {  45 + 1'000'000,  CmdType::DSMEX  },
            {  55 + 1'000'000,  CmdType::SREFEX },
            {  80 + 1'000'000,  CmdType::SREFEN },
            { 100 + 1'000'000,  CmdType::DSMEN  },
            { 125 + 1'000'000,  CmdType::END_OF_SIMULATION },
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
        memSpec->bank_arch = MemSpecLPDDR5::MBG;
        memSpec->numberOfDevices = 1;
        memSpec->bitWidth = 16;


        memSpec->memTimingSpec.tRAS = 10;
        memSpec->memTimingSpec.tRCD = 20;
        memSpec->memTimingSpec.tRFC = 10;
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
        memSpec->memPowerSpec[0].iDD6X = 5e-3;

        memSpec->bwParams.bwPowerFactRho = 0.333333333;

        memSpec->burstLength = 16;
        memSpec->dataRate = 2;

        ddr = std::make_unique<LPDDR5>(*memSpec);
        ddr->setSimulationTime(1'000'000);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR5_22, Reset)
{
	for (const auto& command : testPattern) {
		ddr->doCoreCommand(command);
	};
	ddr->reset();
	
	auto stats = ddr->getStats();

	// Check bank command count: ACT
	ASSERT_EQ(stats.bank[1].counter.act, 0);

	// Check bank command count: PRE
	ASSERT_EQ(stats.bank[0].counter.pre, 0);

	// Check global cycles count
	ASSERT_EQ(stats.rank_total[0].cycles.act, 0);
	ASSERT_EQ(stats.rank_total[0].cycles.pre, 0);
	ASSERT_EQ(stats.rank_total[0].cycles.powerDownAct, 0);
	ASSERT_EQ(stats.rank_total[0].cycles.powerDownPre, 0);

	// Check bank specific ACT cycle count
	ASSERT_EQ(stats.bank[0].cycles.act, 0);
	ASSERT_EQ(stats.bank[1].cycles.act, 0);
	ASSERT_EQ(stats.bank[2].cycles.act, 0);
	ASSERT_EQ(stats.bank[3].cycles.act, 0);
	ASSERT_EQ(stats.bank[4].cycles.act, 0);
	ASSERT_EQ(stats.bank[5].cycles.act, 0);
	ASSERT_EQ(stats.bank[6].cycles.act, 0);
	ASSERT_EQ(stats.bank[7].cycles.act, 0);

	// Check bank specific PRE cycle count
	ASSERT_EQ(stats.bank[0].cycles.pre, 0);
	ASSERT_EQ(stats.bank[1].cycles.pre, 0);
	ASSERT_EQ(stats.bank[2].cycles.pre, 0);
	ASSERT_EQ(stats.bank[3].cycles.pre, 0);
	ASSERT_EQ(stats.bank[4].cycles.pre, 0);
	ASSERT_EQ(stats.bank[5].cycles.pre, 0);
	ASSERT_EQ(stats.bank[6].cycles.pre, 0);
	ASSERT_EQ(stats.bank[7].cycles.pre, 0);

}

TEST_F(DramPowerTest_LPDDR5_22, Test)
{
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    };

    const Rank & rank_1 = internal::LPDDR5TestAccessor.getRanks(ddr->getCore()).at(0);
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

TEST_F(DramPowerTest_LPDDR5_22, CalcEnergy)
{
    auto iterate_to_timestamp = [this](auto & command, const auto & container, timestamp_t timestamp) {
        while (command != container.end() && command->timestamp <= timestamp) {
            ddr->doCoreCommand(*command);
            ++command;
        }
    };

    auto command = testPattern.begin();
    iterate_to_timestamp(command, testPattern, 125 + 1'000'000);
    auto energy = ddr->calcCoreEnergy(125 + 1'000'000);
    auto total_energy = energy.aggregated_bank_energy();

    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 1968);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 816);
    ASSERT_EQ(std::round(energy.bank_energy[0].E_bg_act*1e12), 144);
    ASSERT_EQ(std::round(energy.bank_energy[1].E_bg_act*1e12), 144);
    ASSERT_EQ(std::round(energy.bank_energy[2].E_bg_act*1e12), 144);
    ASSERT_EQ(std::round(energy.bank_energy[3].E_bg_act*1e12), 144);
    ASSERT_EQ(std::round(energy.bank_energy[4].E_bg_act*1e12), 144);
    ASSERT_EQ(std::round(energy.bank_energy[5].E_bg_act*1e12), 144);
    ASSERT_EQ(std::round(energy.bank_energy[6].E_bg_act*1e12), 144);
    ASSERT_EQ(std::round(energy.bank_energy[7].E_bg_act*1e12), 144);

    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 320);
    ASSERT_EQ(std::round(energy.bank_energy[0].E_bg_pre*1e12), 40);
    ASSERT_EQ(std::round(energy.bank_energy[1].E_bg_pre*1e12), 40);
    ASSERT_EQ(std::round(energy.bank_energy[2].E_bg_pre*1e12), 40);
    ASSERT_EQ(std::round(energy.bank_energy[3].E_bg_pre*1e12), 40);
    ASSERT_EQ(std::round(energy.bank_energy[4].E_bg_pre*1e12), 40);
    ASSERT_EQ(std::round(energy.bank_energy[5].E_bg_pre*1e12), 40);
    ASSERT_EQ(std::round(energy.bank_energy[6].E_bg_pre*1e12), 40);
    ASSERT_EQ(std::round(energy.bank_energy[7].E_bg_pre*1e12), 40);

    ASSERT_EQ(std::round(energy.E_sref*1e12), 100);
};
