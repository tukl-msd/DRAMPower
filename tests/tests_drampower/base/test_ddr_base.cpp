#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include "DRAMPower/dram/dram_base.h"

#include "DRAMPower/command/Pattern.h"
#include "DRAMPower/util/cli_architecture_config.h"

#include <memory>

using namespace DRAMPower;

class test_ddr : public dram_base<CmdType>
{
	// Overrides
private:
    timestamp_t update_toggling_rate(timestamp_t, const std::optional<ToggleRateDefinition>&) override {return 0;};
public:
	energy_t calcCoreEnergy(timestamp_t) override { return energy_t(1); };
    interface_energy_info_t calcInterfaceEnergy(timestamp_t) override { return interface_energy_info_t(); };
	util::CLIArchitectureConfig getCLIArchitectureConfig() override { return util::CLIArchitectureConfig{}; };
	SimulationStats getWindowStats(timestamp_t) override { return SimulationStats(); };

	std::vector<timestamp_t> execution_order;

	test_ddr()
	{
		getCommandCoreRouter().routeCommand<CmdType::ACT>([this](const Command & command) {
			execution_order.push_back(command.timestamp);
		});

		getCommandCoreRouter().routeCommand<CmdType::PRE>([this](const Command & command) {
			execution_order.push_back(command.timestamp);
			auto next_timestamp = command.timestamp + 10;
			getImplicitCommandHandler().addImplicitCommand(next_timestamp, [this, next_timestamp]() {
				execution_order.push_back(next_timestamp);
			});
		});

		getCommandCoreRouter().routeCommand<CmdType::PREA>([this](const Command & command) {
			execution_order.push_back(command.timestamp);
			auto next_timestamp = command.timestamp + 1;
			getImplicitCommandHandler().addImplicitCommand(next_timestamp, [this, next_timestamp]() {
				execution_order.push_back(next_timestamp);
			});
		});
	};
};

class DDR_Base_Test : public ::testing::Test {
protected:
    // Test variables
    std::unique_ptr<test_ddr> ddr;

    virtual void SetUp()
    {
        ddr = std::make_unique<test_ddr>();
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DDR_Base_Test, DoCommand)
{
    ASSERT_EQ(ddr->getCommandCoreCount(CmdType::ACT), 0);
    this->ddr->doCoreCommand({ 10, CmdType::ACT, { 1, 0, 0 } });
    ASSERT_EQ(ddr->getCommandCoreCount(CmdType::ACT), 1);

	ASSERT_EQ(ddr->execution_order.size(), 1);
	ASSERT_EQ(ddr->execution_order[0], 10);
}

TEST_F(DDR_Base_Test, ImplicitCommand)
{
	this->ddr->doCoreCommand({ 10, CmdType::PRE, { 1, 0, 0 } });
	this->ddr->doCoreCommand({ 15, CmdType::PREA, { 1, 0, 0 } });
	this->ddr->doCoreCommand({ 50, CmdType::ACT, { 1, 0, 0 } });

	ASSERT_EQ(ddr->execution_order.size(), 5);
	ASSERT_EQ(ddr->execution_order[0], 10);
	ASSERT_EQ(ddr->execution_order[1], 15);
	ASSERT_EQ(ddr->execution_order[2], 16);
	ASSERT_EQ(ddr->execution_order[3], 20);
	ASSERT_EQ(ddr->execution_order[4], 50);
}
