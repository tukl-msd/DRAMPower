#include <gtest/gtest.h>

#include <DRAMPower/util/bus.h>

#include <DRAMPower/command/Command.h>
#include <DRAMPower/command/Pattern.h>

#include <bitset>
#include <initializer_list>

using namespace DRAMPower;


class PatternTest : public ::testing::Test {
protected:
	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

TEST_F(PatternTest, Test)
{
	using namespace pattern_descriptor;

	std::bitset<64> bitset;

	const auto pattern_wr = {
		H,   L,   H,   L,  L,   BL,
		BA0, BA1, BA2, V,  C7,  AP,
		L,   H,   L,   L,  H,   C6,
		C0,  C1,  C2,  C3, C4,  C5
	};

	//auto wr = PatternEncoder::encode(Command{ 0, CmdType::ACT, { 1,2,3,4,5} }, pattern_wr);
	//ASSERT_EQ(wr, 0b101001'100000'010010'101000);

	//auto act = PatternEncoder::encode(Command{ 0, CmdType::ACT, { 1, 0, 0, 2, 0} }, { H, L, L, H, R1, R2, BA0, R0 });
	//ASSERT_EQ(act, 0b1001'1010);
};
