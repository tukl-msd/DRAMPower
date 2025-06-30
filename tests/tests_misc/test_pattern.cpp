#include <gtest/gtest.h>

#include <DRAMPower/util/bus.h>

#include <DRAMPower/command/Command.h>
#include <DRAMPower/command/Pattern.h>

#include <bitset>
#include <initializer_list>

using namespace DRAMPower;


class PatternTest : public ::testing::Test {
protected:

	std::vector<DRAMPower::pattern_descriptor::t> pattern;
	std::vector<DRAMPower::pattern_descriptor::t> pattern2;
	std::vector<DRAMPower::pattern_descriptor::t> pattern3;

	virtual void SetUp()
	{
		using namespace pattern_descriptor;
		pattern = {
			H, L, X, V, AP, BL, H, L, // 8
			BA0, BA1, BA2, BA3, BA4, BA5, BA6, BA7, // 8
			BG0, BG1, BG2, X, X, X, X, X, // 8
			C0,  C1,  C2,  C3, C4, // 5
			R0, R1, R2, // 3
		};
		pattern2 = {
			H, L, H, 
			OPCODE, OPCODE, OPCODE, OPCODE,
			OPCODE, OPCODE, OPCODE, OPCODE,
			L, H, L
		};
		pattern3 = {
			BA0, BA1, BA2, BA3, X, X, X, X,
		};
	}

	virtual void TearDown()
	{
	}
};

TEST_F(PatternTest, Test_Override_Low)
{
	using namespace pattern_descriptor;

	auto encoder = PatternEncoder(PatternEncoderOverrides{
		{X, PatternEncoderBitSpec::L},
		{V, PatternEncoderBitSpec::L},
		{AP, PatternEncoderBitSpec::L},
		{BL, PatternEncoderBitSpec::L},
	});

	// Bank, Bank Group, Rank, Row, Column
	auto result = encoder.encode(Command{0, CmdType::ACT, { 1,2,3,4,17} }, pattern, 0);
	ASSERT_EQ(result, 2189443209);
};

TEST_F(PatternTest, Test_Pattern_LastBit)
{
	using namespace pattern_descriptor;

	auto encoder = PatternEncoder(PatternEncoderOverrides{
		{X, PatternEncoderBitSpec::LAST_BIT},
	});
	const uint64_t init_pattern = 0b0101;
	// Bank, Bank Group, Rank, Row, Column
	auto result = encoder.encode({ 0xB, 2,
		3, 4,}, pattern3, init_pattern);
	ASSERT_EQ(result, 0b1101'0101);
};

TEST_F(PatternTest, Test_Override_High)
{
	using namespace pattern_descriptor;

	auto encoder = PatternEncoder(PatternEncoderOverrides{
		{X, PatternEncoderBitSpec::H},
		{V, PatternEncoderBitSpec::H},
		{AP, PatternEncoderBitSpec::H},
		{BL, PatternEncoderBitSpec::H},
	});

	// Bank, Bank Group, Rank, Row, Column
	auto result = encoder.encode(Command{0, CmdType::ACT, { 1,2,3,4,17} }, pattern, 0);
	ASSERT_EQ(result, 3196084105);
};

TEST_F(PatternTest, Test_Override_Last)
{
	using namespace pattern_descriptor;

	auto encoder = PatternEncoder(PatternEncoderOverrides{
		{X, PatternEncoderBitSpec::LAST_BIT},
		{V, PatternEncoderBitSpec::LAST_BIT},
		{AP, PatternEncoderBitSpec::LAST_BIT},
		{BL, PatternEncoderBitSpec::LAST_BIT},
	});

	// last_pattern
	uint64_t init_pattern = 0xAA'AA'AA'AA'AA'AA'AA'AA; // 0b10101010...

	// Bank, Bank Group, Rank, Row, Column
	auto result = encoder.encode(Command{0, CmdType::ACT, { 1,2,3,4,17} }, pattern, init_pattern);
	ASSERT_EQ(result, 2860534409);
};

TEST_F(PatternTest, Test_Override_2_Patterns)
{
	using namespace pattern_descriptor;

	auto encoder = PatternEncoder(PatternEncoderOverrides{
		{X, PatternEncoderBitSpec::LAST_BIT},
		{V, PatternEncoderBitSpec::LAST_BIT},
		{AP, PatternEncoderBitSpec::LAST_BIT},
		{BL, PatternEncoderBitSpec::LAST_BIT},
	});

	// last_pattern
	uint64_t init_pattern = 0xAA'AA'AA'AA'AA'AA'AA'AA; // 0b10101010...

	// Set X after BG2 to one
	init_pattern |= 0b1111100000000;

	// Bank, Bank Group, Rank, Row, Column
	auto result = encoder.encode(Command{0, CmdType::ACT, { 1,2,3,4,17} }, pattern, init_pattern);
	ASSERT_EQ(result, 2860539785);
	result = encoder.encode(Command{0, CmdType::ACT, { 7,3,3,7,17} }, pattern, result);
	ASSERT_EQ(result, 2866864015);
};

TEST_F(PatternTest, Test_Opcode_Patterns)
{
	using namespace pattern_descriptor;

	auto encoder = PatternEncoder(PatternEncoderOverrides{
		{X, PatternEncoderBitSpec::LAST_BIT},
		{V, PatternEncoderBitSpec::LAST_BIT},
		{AP, PatternEncoderBitSpec::LAST_BIT},
		{BL, PatternEncoderBitSpec::LAST_BIT},
	});

	// last_pattern
	uint64_t init_pattern = 0x0;

	const uint64_t opcode = 0x1234'56A5; // 0b...10100101
	const uint16_t opcodeLength = 32; // 32 bits
	encoder.setOpcode(opcode, opcodeLength);

	// Bank, Bank Group, Rank, Row, Column
	auto result = encoder.encode(TargetCoordinate{ 1,2,3,4,17}, pattern2, init_pattern);
	// HLH
	uint64_t result_expected = 0b101 << 11;
	// OPCODE
	result_expected |= (opcode & 0xFF) << 3;
	// LHL
	result_expected |= 0b010;

	ASSERT_EQ(result, result_expected);
	result = encoder.encode(TargetCoordinate{ 7,6,3,7,21}, pattern2, result);
	ASSERT_EQ(result, result_expected);
};
