#include <algorithm>
#include <gtest/gtest.h>

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"

#include "DRAMPower/dram/dram_base.h"

#include "DRAMPower/util/cli_architecture_config.h"

#include <memory>
#include <stdexcept>

using namespace DRAMPower;

class test_ddr : public dram_base<CmdType>
{
public:
    energy_t calcCoreEnergy(timestamp_t) override { return energy_t(1); };
    interface_energy_info_t calcInterfaceEnergy(timestamp_t) override { return interface_energy_info_t(); };
    SimulationStats getWindowStats(timestamp_t) override { return SimulationStats(); };
    util::CLIArchitectureConfig getCLIArchitectureConfig() override { return util::CLIArchitectureConfig{}; };
    bool isSerializable() const override {
        return false;
    }
    

	test_ddr() = default;

private:
    void serialize_impl(std::ostream&) const override {}
    void deserialize_impl(std::istream&) override {}
    void doCoreCommandImpl(const Command& command) override {
        implicitCommandHandler.processImplicitCommandQueue(command.timestamp, last_command_time);
        __doCoreCommand(command);
    }
    void doInterfaceCommandImpl(const Command& command) override {
        implicitCommandHandler.processImplicitCommandQueue(command.timestamp, last_command_time);
        __doInterfaceCommand(command);
    }

    void __doInterfaceCommand(const Command&) {
        return;
    }
    void __doCoreCommand(const Command& command) {
        auto next_timestamp = command.timestamp;
        switch (command.type) {
            case CmdType::ACT:
                break;
            case CmdType::PRE:
                next_timestamp += 10;
                implicitCommandHandler.addImplicitCommand(next_timestamp, [this, next_timestamp]() {
                    execution_order.push_back(next_timestamp);
                });
                break;
            case CmdType::PREA:
                next_timestamp += 1;
                implicitCommandHandler.addImplicitCommand(next_timestamp, [this, next_timestamp]() {
                    execution_order.push_back(next_timestamp);
                });
                break;
            default:
                throw std::runtime_error("Invalid type");
        }
        execution_order.push_back(command.timestamp);
        last_command_time = std::max(last_command_time, command.timestamp);
    }
    timestamp_t getLastCommandTime_impl() const override {
        return last_command_time;
    }

    timestamp_t last_command_time;

public:
	std::vector<timestamp_t> execution_order;
    ImplicitCommandHandler_t implicitCommandHandler;
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
    this->ddr->doCoreCommand({ 10, CmdType::ACT, { 1, 0, 0 } });

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
