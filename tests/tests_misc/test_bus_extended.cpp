#include <gtest/gtest.h>

#include <DRAMPower/util/bus.h>
#include <array>

using namespace DRAMPower;


class ExtendedBusIdlePatternTest : public ::testing::Test {
protected:

	util::dynamic_bitset burst_ones;
	util::dynamic_bitset burst_zeroes;
	util::dynamic_bitset burst_custom;
	size_t buswidth = 128; // test bus width greater than 64

	virtual void SetUp()
	{
		for(int i = 0; i < buswidth; i++)
		{
			burst_ones.push_back(true);
			burst_zeroes.push_back(false);
			burst_custom.push_back(i % 3 ? true : false);
		}
	}

	virtual void TearDown()
	{
	}
};

#define ASSERT_EQ_BITSET(lhs, rhs) ASSERT_EQ(lhs, util::dynamic_bitset( lhs.size(), rhs))

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLow_1)
{
	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::L, util::Bus::BusInitPatternSpec::L);

	ASSERT_EQ(bus.at(0), burst_zeroes);
	ASSERT_EQ(bus.at(1), burst_zeroes);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLow_2)
{
	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::L, util::Bus::BusInitPatternSpec::H);

	ASSERT_EQ(bus.at(0), burst_zeroes);
	ASSERT_EQ(bus.at(1), burst_zeroes);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLow_3)
{
	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::L, burst_custom);

	ASSERT_EQ(bus.at(0), burst_zeroes);
	ASSERT_EQ(bus.at(1), burst_zeroes);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleHigh_1)
{
	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::H, util::Bus::BusInitPatternSpec::L);

	ASSERT_EQ(bus.at(0), burst_ones);
	ASSERT_EQ(bus.at(1), burst_ones);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleHigh_2)
{
	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::H, util::Bus::BusInitPatternSpec::H);

	ASSERT_EQ(bus.at(0), burst_ones);
	ASSERT_EQ(bus.at(1), burst_ones);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleHigh_3)
{
	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::H, burst_custom);

	ASSERT_EQ(bus.at(0), burst_ones);
	ASSERT_EQ(bus.at(1), burst_ones);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLastPattern_1)
{
	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::LAST_PATTERN, util::Bus::BusInitPatternSpec::L);

	ASSERT_EQ(bus.at(0), burst_zeroes);
	ASSERT_EQ(bus.at(1), burst_zeroes);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLastPattern_2)
{
	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::LAST_PATTERN, util::Bus::BusInitPatternSpec::H);

	ASSERT_EQ(bus.at(0), burst_ones);
	ASSERT_EQ(bus.at(1), burst_ones);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLastPattern_3)
{
	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::LAST_PATTERN, burst_custom);

	ASSERT_EQ(bus.at(0), burst_custom);
	ASSERT_EQ(bus.at(1), burst_custom);
};

TEST_F(ExtendedBusIdlePatternTest, Load_Width_8)
{
	util::Bus bus(8, util::Bus::BusIdlePatternSpec::L, util::Bus::BusInitPatternSpec::L);

	// Init load overrides init pattern
	bus.load(0, 0b0010'1010'1001'0110, 2);
	ASSERT_EQ_BITSET(bus.at(0), 0b0010'1010);
	ASSERT_EQ_BITSET(bus.at(1), 0b1001'0110);
};

TEST_F(ExtendedBusIdlePatternTest, Load_Width_64)
{
	const uint32_t buswidth = 8 * 8;
	const uint32_t number_bytes = (buswidth + 7) / 8;

	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::L, util::Bus::BusInitPatternSpec::L);
	std::array<uint8_t, number_bytes> data = { 0 }; 
	
	auto expected = util::Bus::burst_t();

	auto pattern_gen = [] (size_t i) -> uint8_t {
		return i;
	};

	// Load bus
	for (size_t i = 0; i < number_bytes; i++) {
		data[i] = pattern_gen(i);
	}
	bus.load(0, data.data(), buswidth);

	// Create expected burst
	uint8_t byte = 0;
	size_t byteidx = 0;
	for (size_t i = 0; i < buswidth; i++) {
		if (i % 8 == 0) {
			byte = pattern_gen(byteidx++);
		}
		expected.push_back(byte & (1 << (i % 8)));
	}
	
	ASSERT_EQ(bus.at(0), expected);
};

TEST_F(ExtendedBusIdlePatternTest, Load_Width_512)
{
	const uint32_t buswidth = 64 * 8;
	const uint32_t number_bytes = (buswidth + 7) / 8;

	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::L, util::Bus::BusInitPatternSpec::L);
	std::array<uint8_t, number_bytes> data = { 0 }; 
	
	auto expected = util::Bus::burst_t();

	auto pattern_gen = [] (size_t i) -> uint8_t {
		return i;
	};

	// Load bus
	for (size_t i = 0; i < number_bytes; i++) {
		data[i] = pattern_gen(i);
	}
	bus.load(0, data.data(), buswidth);

	// Create expected burst
	uint8_t byte = 0;
	size_t byteidx = 0;
	for (size_t i = 0; i < buswidth; i++) {
		if (i % 8 == 0) {
			byte = pattern_gen(byteidx++);
		}
		expected.push_back(byte & (1 << (i % 8)));
	}
	
	ASSERT_EQ(bus.at(0), expected);
};

class ExtendedBusStatsTest : public ::testing::Test {
protected:

	util::dynamic_bitset burst_ones;
	util::dynamic_bitset burst_zeroes;
	util::dynamic_bitset burst_custom;
	const size_t buswidth = 128; // test bus width greater than 64

	virtual void SetUp()
	{
		for(int i = 0; i < buswidth; i++)
		{
			burst_ones.push_back(true);
			burst_zeroes.push_back(false);
			burst_custom.push_back(i % 3 ? true : false);
		}
	}

	virtual void TearDown()
	{
	}
};

TEST_F(ExtendedBusStatsTest, Stats_Pattern_1)
{
	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::L, burst_custom);
	std::size_t custom_ones = burst_custom.count();
	std::size_t custom_zeroes = buswidth - custom_ones;
	uint8_t burst_ones_data[(buswidth + 7) / 8] = { 0xFF };
	uint8_t burst_zeroes_data[(buswidth + 7) / 8] = { 0 };

	for(auto i = 0; i < (buswidth + 7) / 8; i++) {
		burst_ones_data[i] = 0xFF;
	}

	ASSERT_EQ(buswidth, 128);
	ASSERT_EQ(custom_ones, 85);
	ASSERT_EQ(custom_zeroes, 43);

	auto stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 128);
	ASSERT_EQ(stats.ones_to_zeroes, 85);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 85);

	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 0 + 0);
	ASSERT_EQ(stats.zeroes, 128 + 128);
	ASSERT_EQ(stats.ones_to_zeroes, 85 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 0);
	ASSERT_EQ(stats.bit_changes, 85 + 0);

	bus.load(2, burst_ones_data, buswidth);
	
	stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 0 + 0 + 128);
	ASSERT_EQ(stats.zeroes, 128 + 128 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 85 + 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 0 + 128);
	ASSERT_EQ(stats.bit_changes, 85 + 0 + 128);

	stats = bus.get_stats(4);
	ASSERT_EQ(stats.ones, 0 + 0 + 128 + 0);
	ASSERT_EQ(stats.zeroes, 128 + 128 + 0 + 128);
	ASSERT_EQ(stats.ones_to_zeroes, 85 + 0 + 0 + 128);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 0 + 128 + 0);
	ASSERT_EQ(stats.bit_changes, 85 + 0 + 128 + 128);

	bus.load(4, burst_ones_data, buswidth);

	stats = bus.get_stats(5);
	ASSERT_EQ(stats.ones, 0 + 0 + 128 + 0 + 128);
	ASSERT_EQ(stats.zeroes, 128 + 128 + 0 + 128 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 85 + 0 + 0 + 128 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 0 + 128 + 0 + 128);
	ASSERT_EQ(stats.bit_changes, 85 + 0 + 128 + 128 + 128);

	stats = bus.get_stats(6);
	ASSERT_EQ(stats.ones, 0 + 0 + 128 + 0 + 128 + 0);
	ASSERT_EQ(stats.zeroes, 128 + 128 + 0 + 128 + 0 + 128);
	ASSERT_EQ(stats.ones_to_zeroes, 85 + 0 + 0 + 128 + 0 + 128);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 0 + 128 + 0 + 128 + 0);
	ASSERT_EQ(stats.bit_changes, 85 + 0 + 128 + 128 + 128 + 128);
};

TEST_F(ExtendedBusStatsTest, Stats_Pattern_2)
{
	util::Bus bus(buswidth, util::Bus::BusIdlePatternSpec::LAST_PATTERN, burst_custom);
	std::size_t custom_ones = burst_custom.count();
	std::size_t custom_zeroes = buswidth - custom_ones;
	uint8_t burst_ones_data[(buswidth + 7) / 8] = { 0xFF };
	uint8_t burst_zeroes_data[(buswidth + 7) / 8] = { 0 };

	for(auto i = 0; i < (buswidth + 7) / 8; i++) {
		burst_ones_data[i] = 0xFF;
	}

	ASSERT_EQ(buswidth, 128);
	ASSERT_EQ(custom_ones, 85);
	ASSERT_EQ(custom_zeroes, 43);

	auto stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 85); // 85
	ASSERT_EQ(stats.zeroes, 43); // 43
	ASSERT_EQ(stats.ones_to_zeroes, 0); // 0
	ASSERT_EQ(stats.zeroes_to_ones, 0); // 0
	ASSERT_EQ(stats.bit_changes, 0); // 0

	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 85 + 85); // 85 + 0
	ASSERT_EQ(stats.zeroes, 43 + 43); // 43 + 128
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0); // 0 + 85
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 0); // 0 + 0
	ASSERT_EQ(stats.bit_changes, 0 + 0); // 0 + 85

	bus.load(2, burst_ones_data, buswidth);
	
	stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 85 + 85 + 128);  // 85 + 0 + 128
	ASSERT_EQ(stats.zeroes, 43 + 43 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 0 + 43);
	ASSERT_EQ(stats.bit_changes, 0 + 0 + 43);

	stats = bus.get_stats(4);
	ASSERT_EQ(stats.ones, 85 + 85 + 128 + 128);
	ASSERT_EQ(stats.zeroes, 43 + 43 + 0 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0 + 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 0 + 43 + 0);
	ASSERT_EQ(stats.bit_changes, 0 + 0 + 43 + 0);

	bus.load(4, burst_ones_data, buswidth);

	stats = bus.get_stats(5);
	ASSERT_EQ(stats.ones, 85 + 85 + 128 + 128 + 128);
	ASSERT_EQ(stats.zeroes, 43 + 43 + 0 + 0 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0 + 0 + 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 0 + 43 + 0 + 0);
	ASSERT_EQ(stats.bit_changes, 0 + 0 + 43 + 0 + 0);

	stats = bus.get_stats(6);
	ASSERT_EQ(stats.ones,  85 + 85 + 128 + 128 + 128 + 128);
	ASSERT_EQ(stats.zeroes, 43 + 43 + 0 + 0 + 0 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0 + 0 + 0 + 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 0 + 43 + 0 + 0 + 0);
	ASSERT_EQ(stats.bit_changes, 0 + 0 + 43 + 0 + 0 + 0);
};
