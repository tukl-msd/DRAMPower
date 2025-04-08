#include <gtest/gtest.h>

#include <DRAMPower/util/binary_ops.h>

#include <bitset>
#include <array>
#include <optional>

#include <DRAMPower/util/burst_storage.h>
#include <DRAMPower/util/dynamic_bitset.h>

using namespace DRAMPower;	

class MiscTest : public ::testing::Test {
protected:
    // Test variables
	using input_t = std::tuple<uint64_t, uint64_t>;
	using test_list_t = std::vector<input_t>;

	using data_t = std::array<uint8_t, 12>;

	test_list_t pattern = {
		{ 
			0b0000000000000000000000000000000000000000000000000000000000000000,
			0b1111111111111111111111111111111111111111111111111111111111111111
		},
		{
			0b1111111111111111111111111111111111111111111111111111111111111111,
			0b0000000000000000000000000000000000000000000000000000000000000000
		},
		{
			0b1111111111111111111111111111111111111111111111111111111111111111,
			0b1111111111111111111111111111111111111111111111111111111111111111
		},
		{
			0b0000000000000000000000000000000000000000000000000000000000000000,
			0b0000000000000000000000000000000000000000000000000000000000000001
		},
		{
			0b1000000000000000000000000000000000000000000000000000000000000000,
			0b0000000000000000000000000000000000000000000000000000000000000001
		},
		{
			0b0000000000000000000000000000000000000000000000000000000010101010,
			0b0000000000000000000000000000000000000000000000000000000001010101
		},
		{
			0b0000000000000000000000000000000000000000000000000000000000000000,
			0b0000000000000000000000000000000000000000000000000000000000001010
		}
	};

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

#define ASSERT_EQ_BITSET(N, lhs, rhs) ASSERT_EQ(lhs, util::sub_bitset<N>(N, rhs))

TEST_F(MiscTest, TestChunking)
{
	// Test data
	data_t data = {
		0b1100'0000,
		0b1010'1111,
		0b0000'0010,
		0b0011'1111,

		0b1111'0000,
		0b1010'1011,
		0b0001'0101,
		0b1111'0000,

		0b1010'1011,
		0b1100'0000,
		0b0000'1111,
		0b1111'1100,
	};

	// Test setup
	constexpr std::size_t width = 6;
	util::burst_storage<width> burst_storage{width};

	burst_storage.insert_data(data.data(), data.size() * 8);

	// Test assertions
	ASSERT_EQ(burst_storage.size(), 16);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst( 0), 0b111111);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst( 1), 0b000000);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst( 2), 0b111111);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst( 3), 0b000000);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst( 4), 0b101010);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst( 5), 0b111111);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst( 6), 0b000000);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst( 7), 0b010101);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst( 8), 0b101010);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst( 9), 0b111111);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst(10), 0b000000);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst(11), 0b111111);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst(12), 0b000000);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst(13), 0b101010);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst(14), 0b111111);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst(15), 0b000000);
};

TEST_F(MiscTest, TestBurstLoad_Uneven)
{
	// Test data
	data_t data = {
		0b0001'0101,
		0b1010'1111,
	};

	// Test setup
	constexpr std::size_t width = 4;
	util::burst_storage<width> burst_storage{width};

	burst_storage.insert_data(data.data(), 12);

	// Test assertions
	ASSERT_EQ(burst_storage.size(), 3);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst(0), 0b1111);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst(1), 0b0001);
	ASSERT_EQ_BITSET(width, burst_storage.get_burst(2), 0b0101);
};

TEST_F(MiscTest, BitChanges)
{
	auto apply = [](const input_t & input) {
		return util::BinaryOps::bit_changes(std::get<0>(input), std::get<1>(input));
	};

	ASSERT_EQ(apply(pattern[0]), 64);
	ASSERT_EQ(apply(pattern[1]), 64);
	ASSERT_EQ(apply(pattern[2]), 0);
	ASSERT_EQ(apply(pattern[3]), 1);
	ASSERT_EQ(apply(pattern[4]), 2);
	ASSERT_EQ(apply(pattern[5]), 8);
	ASSERT_EQ(apply(pattern[6]), 2);
}

TEST_F(MiscTest, ZeroToOne)
{
	auto apply = [](const input_t & input) {
		return util::BinaryOps::zero_to_ones(std::get<0>(input), std::get<1>(input));
	};

	ASSERT_EQ(apply(pattern[0]), 64);
	ASSERT_EQ(apply(pattern[1]), 0);
	ASSERT_EQ(apply(pattern[2]), 0);
	ASSERT_EQ(apply(pattern[3]), 1);
	ASSERT_EQ(apply(pattern[4]), 1);
	ASSERT_EQ(apply(pattern[5]), 4);
	ASSERT_EQ(apply(pattern[6]), 2);
}

TEST_F(MiscTest, OneToZero)
{
	auto apply = [](const input_t & input) {
		return util::BinaryOps::one_to_zeroes(std::get<0>(input), std::get<1>(input));
	};

	ASSERT_EQ(apply(pattern[0]), 0);
	ASSERT_EQ(apply(pattern[1]), 64);
	ASSERT_EQ(apply(pattern[2]), 0);
	ASSERT_EQ(apply(pattern[3]), 0);
	ASSERT_EQ(apply(pattern[4]), 1);
	ASSERT_EQ(apply(pattern[5]), 4);
	ASSERT_EQ(apply(pattern[6]), 0);
}
