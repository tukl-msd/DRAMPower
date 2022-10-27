#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include "DRAMPower/dram/dram_base.h"

#include <memory>

using namespace DRAMPower;

template<CmdType Type, uint64_t Pattern>
struct CommandPattern
{
	constexpr static auto type() { return Type; };
	constexpr static auto pattern() { return Pattern; };
};

template<typename Head, typename... Tail>
struct pattern_helper
{
	constexpr static uint64_t get(CmdType type)
	{
		if (Head::type() == type)
			return Head::pattern();

		return pattern_helper<Tail...>::get(type);
	};
};

template<typename Head>
struct pattern_helper<Head>
{
	constexpr static uint64_t get(CmdType type)
	{
		if (Head::type() == type)
			return Head::pattern();

		return 0x00;
	};
};

template<typename... PatternList>
struct CommandPatternMap
{
	constexpr static uint64_t getPattern(CmdType type)
	{
		return pattern_helper<PatternList...>::get(type);
	};
};



class DramPowerDataTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

TEST_F(DramPowerDataTest, Test_1)
{
	Command command{ 10, CmdType::ACT, {} };

	using TestPatterMap = CommandPatternMap<
		CommandPattern<CmdType::ACT, 0x1001>,
		CommandPattern<CmdType::PRE, 0x1011>
	>;
	
	ASSERT_EQ(TestPatterMap::getPattern(CmdType::ACT), 0x1001);
	ASSERT_EQ(TestPatterMap::getPattern(CmdType::PRE), 0x1011);
	ASSERT_EQ(TestPatterMap::getPattern(CmdType::NOP), 0x0000);
};