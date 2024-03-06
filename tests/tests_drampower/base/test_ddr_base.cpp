#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include "DRAMPower/dram/dram_base.h"

#include "DRAMPower/command/Pattern.h"

#include <memory>

using namespace DRAMPower;

class test_ddr : public dram_base<CmdType>
{
public:
	void handle_interface(const Command& cmd) override {};
	std::vector<timestamp_t> execution_order;

	test_ddr() : dram_base<CmdType>(PatternEncoderOverrides{})
	{
		this->routeCommand<CmdType::ACT>([this](const Command & command) {
			execution_order.push_back(command.timestamp);
		});

		this->routeCommand<CmdType::PRE>([this](const Command & command) {
			execution_order.push_back(command.timestamp);
			auto next_timestamp = command.timestamp + 10;
			addImplicitCommand(next_timestamp, [this, next_timestamp]() {
				execution_order.push_back(next_timestamp);
			});
		});

		this->routeCommand<CmdType::PREA>([this](const Command & command) {
			execution_order.push_back(command.timestamp);
			auto next_timestamp = command.timestamp + 1;
			addImplicitCommand(next_timestamp, [this, next_timestamp]() {
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
    ASSERT_EQ(ddr->getCommandCount(CmdType::ACT), 0);
    this->ddr->doCommand({ 10, CmdType::ACT, { 1, 0, 0 } });
    ASSERT_EQ(ddr->getCommandCount(CmdType::ACT), 1);

	ASSERT_EQ(ddr->execution_order.size(), 1);
	ASSERT_EQ(ddr->execution_order[0], 10);
}

TEST_F(DDR_Base_Test, ImplicitCommand)
{
	this->ddr->doCommand({ 10, CmdType::PRE, { 1, 0, 0 } });
	this->ddr->doCommand({ 15, CmdType::PREA, { 1, 0, 0 } });
	this->ddr->doCommand({ 50, CmdType::ACT, { 1, 0, 0 } });

	ASSERT_EQ(ddr->execution_order.size(), 5);
	ASSERT_EQ(ddr->execution_order[0], 10);
	ASSERT_EQ(ddr->execution_order[1], 15);
	ASSERT_EQ(ddr->execution_order[2], 16);
	ASSERT_EQ(ddr->execution_order[3], 20);
	ASSERT_EQ(ddr->execution_order[4], 50);
}
