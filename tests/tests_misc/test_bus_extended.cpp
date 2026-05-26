#include <gtest/gtest.h>

#include "DRAMPower/util/bus_types.h"
#include <DRAMPower/util/bus.h>

#include <bitset>
#include <array>

using namespace DRAMPower;
using Bus_512 = util::Bus<512>;
using Bus_128 = util::Bus<128>;
using Bus_64 = util::Bus<64>;
using Bus_8 = util::Bus<8>;

class ExtendedBusIdlePatternTest : public ::testing::Test {
protected:

	template<size_t N>
	void Init(
		typename util::Bus<N>::burst_t& burst_ones,
		typename util::Bus<N>::burst_t& burst_zeroes,
		typename util::Bus<N>::burst_t& burst_custom
	) {
		burst_ones.set();
		burst_zeroes.reset();
		for(size_t i = 0; i < N; i++)
		{
			burst_custom.set(i, (i % 3) ? true : false);
		}
	}

	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

#define ASSERT_EQ_BITSET(N, lhs, rhs) ASSERT_EQ(lhs, std::bitset<N>(rhs))
#define ASSERT_EQ_BURST(lhs, rhs) ASSERT_EQ(lhs, rhs)

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLow_1)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L);

	ASSERT_EQ_BURST(bus.at(0), burst_zeroes);
	ASSERT_EQ_BURST(bus.at(1), burst_zeroes);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLow_2)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L);

	ASSERT_EQ_BURST(bus.at(0), burst_zeroes);
	ASSERT_EQ_BURST(bus.at(1), burst_zeroes);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLow_3)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L, true);

	ASSERT_EQ_BURST(bus.at(0), burst_zeroes);
	ASSERT_EQ_BURST(bus.at(1), burst_zeroes);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleHigh_1)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::H);

	ASSERT_EQ_BURST(bus.at(0), burst_ones);
	ASSERT_EQ_BURST(bus.at(1), burst_ones);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleHigh_2)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::H);

	ASSERT_EQ_BURST(bus.at(0), burst_ones);
	ASSERT_EQ_BURST(bus.at(1), burst_ones);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleHigh_3)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::H, true);

	ASSERT_EQ_BURST(bus.at(0), burst_ones);
	ASSERT_EQ_BURST(bus.at(1), burst_ones);
};

TEST_F(ExtendedBusIdlePatternTest, Load_Width_8)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L);

	// Init load overrides init pattern
	bus.load(0, 0b0010'1010'1001'0110, 2);
	ASSERT_EQ_BITSET(8, bus.at(0), 0b0010'1010);
	ASSERT_EQ_BITSET(8, bus.at(1), 0b1001'0110);
};

TEST_F(ExtendedBusIdlePatternTest, Load_Width_64)
{
	Bus_64::burst_t burst_ones;
	Bus_64::burst_t burst_zeroes;
	Bus_64::burst_t burst_custom;
	Init<64>(burst_ones, burst_zeroes, burst_custom);
	const uint32_t buswidth = 8 * 8;
	const uint32_t number_bytes = (buswidth + 7) / 8;

	Bus_64 bus(64, 1, util::BusIdlePatternSpec::L);
	std::array<uint8_t, number_bytes> data = { 0 }; 
	
	auto expected = Bus_64::burst_t(64);

	auto pattern_gen = [] (size_t i) -> uint8_t {
		return static_cast<uint8_t>(i);
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
		expected.set(i, ((byte >> (i % 8)) & 1));
	}
	
	ASSERT_EQ_BURST(bus.at(0), expected);
};

TEST_F(ExtendedBusIdlePatternTest, Load_Width_512)
{
	const uint32_t buswidth = 64 * 8;
	const uint32_t number_bytes = (buswidth + 7) / 8;

	Bus_512 bus(512, 1, util::BusIdlePatternSpec::L);
	std::array<uint8_t, number_bytes> data = { 0 }; 
	
	auto expected = Bus_512::burst_t(512);

	auto pattern_gen = [] (size_t i) -> uint8_t {
		return static_cast<uint8_t>(i);
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
		expected.set(i, ((byte >> (i % 8)) & 1));
	}
	
	ASSERT_EQ_BURST(bus.at(0), expected);
};

class ExtendedBusStatsTest : public ::testing::Test {
protected:

	std::bitset<128> burst_ones{128};
	std::bitset<128> burst_zeroes{128};
	std::bitset<128> burst_custom{128};
	const static constexpr size_t buswidth = 128; // test bus width greater than 64
	const static constexpr size_t bus_array_size = (buswidth + 7) / 8;

	
	template<size_t N>
	void Init(
		typename util::Bus<N>::burst_t& burst_ones,
		typename util::Bus<N>::burst_t& burst_zeroes,
		typename util::Bus<N>::burst_t& burst_custom
	) {
		burst_ones.set();
		burst_zeroes.reset();
		for(size_t i = 0; i < N; i++)
		{
			burst_custom.set(i, (i % 3 ? true : false));
		}
	}

	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

TEST_F(ExtendedBusStatsTest, Stats_Pattern_Datarate_1)
{
	uint_fast8_t datarate = 2;
	timestamp_t timestamp = 3;
	Bus_128 bus(128, datarate, util::BusIdlePatternSpec::L);

	auto stats = bus.get_stats(timestamp); // 3 cycles with double data rate
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, buswidth * timestamp * datarate);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 0);
}

TEST_F(ExtendedBusStatsTest, Stats_Pattern_Datarate_2)
{
	uint_fast8_t datarate = 13;
	timestamp_t timestamp = 47;
	Bus_128 bus(128, datarate, util::BusIdlePatternSpec::L);


	auto stats = bus.get_stats(timestamp); // 3 cycles with double data rate
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, buswidth * timestamp * datarate);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 0);
}
