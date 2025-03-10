#include <gtest/gtest.h>

#include <DRAMPower/util/bus.h>
#include <DRAMPower/util/dynamic_bitset.h>
#include <array>
#include <optional>
#include <algorithm>

using namespace DRAMPower;
constexpr size_t buswidth = 128; // test bus width greater than 64
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
		for(size_t i = 0; i < N; i++)
		{
			burst_ones.push_back(true);
			burst_zeroes.push_back(false);
			burst_custom.push_back((i % 3 ? true : false));
		}
	}

	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

#define ASSERT_HAS_DATA(lhs) ASSERT_TRUE(lhs.has_value())
#define ASSERT_NO_DATA(lhs) ASSERT_FALSE(!lhs.has_value())
#define ASSERT_EQ_BITSET(N, lhs, rhs) ASSERT_HAS_DATA(lhs); ASSERT_EQ(lhs.value(), util::dynamic_bitset<N>(N, rhs))
#define ASSERT_EQ_BURST(lhs, rhs) ASSERT_HAS_DATA(lhs); ASSERT_EQ(lhs.value(), rhs)

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLow_1)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

	ASSERT_EQ_BURST(bus.at(0), burst_zeroes);
	ASSERT_EQ_BURST(bus.at(1), burst_zeroes);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLow_2)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::H);

	ASSERT_EQ_BURST(bus.at(0), burst_zeroes);
	ASSERT_EQ_BURST(bus.at(1), burst_zeroes);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLow_3)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L, burst_custom);

	ASSERT_EQ_BURST(bus.at(0), burst_zeroes);
	ASSERT_EQ_BURST(bus.at(1), burst_zeroes);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleHigh_1)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::H, util::BusInitPatternSpec::L);

	ASSERT_EQ_BURST(bus.at(0), burst_ones);
	ASSERT_EQ_BURST(bus.at(1), burst_ones);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleHigh_2)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::H, util::BusInitPatternSpec::H);

	ASSERT_EQ_BURST(bus.at(0), burst_ones);
	ASSERT_EQ_BURST(bus.at(1), burst_ones);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleHigh_3)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::H, burst_custom);

	ASSERT_EQ_BURST(bus.at(0), burst_ones);
	ASSERT_EQ_BURST(bus.at(1), burst_ones);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLastPattern_1)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::LAST_PATTERN, util::BusInitPatternSpec::L);

	ASSERT_EQ_BURST(bus.at(0), burst_zeroes);
	ASSERT_EQ_BURST(bus.at(1), burst_zeroes);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLastPattern_2)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::LAST_PATTERN, util::BusInitPatternSpec::H);

	ASSERT_EQ_BURST(bus.at(0), burst_ones);
	ASSERT_EQ_BURST(bus.at(1), burst_ones);
};

TEST_F(ExtendedBusIdlePatternTest, EmptyIdleLastPattern_3)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::LAST_PATTERN, burst_custom);

	ASSERT_EQ_BURST(bus.at(0), burst_custom);
	ASSERT_EQ_BURST(bus.at(1), burst_custom);
};

TEST_F(ExtendedBusIdlePatternTest, Load_Width_8)
{
	Bus_8::burst_t burst_ones;
	Bus_8::burst_t burst_zeroes;
	Bus_8::burst_t burst_custom;
	Init<8>(burst_ones, burst_zeroes, burst_custom);
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

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

	Bus_64 bus(64, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);
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

	Bus_512 bus(512, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);
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

	util::dynamic_bitset<128> burst_ones{128};
	util::dynamic_bitset<128> burst_zeroes{128};
	util::dynamic_bitset<128> burst_custom{128};
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
	Bus_128 bus(128, datarate, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

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
	Bus_128 bus(128, datarate, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);


	auto stats = bus.get_stats(timestamp); // 3 cycles with double data rate
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, buswidth * timestamp * datarate);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 0);
}

TEST_F(ExtendedBusStatsTest, Stats_Pattern_1)
{
	Bus_128::burst_t burst_ones{128};
	Bus_128::burst_t burst_zeroes{128};
	Bus_128::burst_t burst_custom{128};
	Init<128>(burst_ones, burst_zeroes, burst_custom);

	Bus_128 bus(128, 1, util::BusIdlePatternSpec::L, burst_custom);
	std::size_t custom_ones = burst_custom.count();
	std::size_t custom_zeroes = buswidth - custom_ones;
	uint8_t burst_ones_data[bus_array_size];
	std::fill_n(burst_ones_data, bus_array_size, 0xFF);

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
	Bus_128::burst_t burst_ones{128};
	Bus_128::burst_t burst_zeroes{128};
	Bus_128::burst_t burst_custom{128};
	Init<128>(burst_ones, burst_zeroes, burst_custom);

	Bus_128 bus(128, 1, util::BusIdlePatternSpec::LAST_PATTERN, burst_custom);
	std::size_t custom_ones = burst_custom.count();
	std::size_t custom_zeroes = buswidth - custom_ones;
	uint8_t burst_ones_data[bus_array_size];
	std::fill_n(burst_ones_data, bus_array_size, 0xFF);

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
