#include <gtest/gtest.h>

#include <DRAMPower/util/bus.h>
#include <DRAMPower/util/dynamic_bitset.h>

using namespace DRAMPower;

using Bus_512 = util::Bus<512>;
using Bus_128 = util::Bus<128>;
using Bus_64 = util::Bus<64>;
using Bus_16 = util::Bus<16>;
using Bus_8 = util::Bus<8>;
using Bus_6 = util::Bus<6>;
using Bus_4 = util::Bus<4>;

class BusTest : public ::testing::Test {
protected:
	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

#define ASSERT_HAS_DATA(lhs) ASSERT_TRUE(lhs.has_value())
#define ASSERT_NO_DATA(lhs) ASSERT_FALSE(lhs.has_value())
#define ASSERT_EQ_BITSET(N, lhs, rhs) ASSERT_HAS_DATA(lhs); ASSERT_EQ(lhs.value(), util::dynamic_bitset<N>(N, rhs))

TEST_F(BusTest, EmptyTest)
{
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

	// auto [hasData, data] = bus.at(0);

	ASSERT_HAS_DATA(bus.at(0));
	ASSERT_EQ(bus.at(0).value(), Bus_8::burst_t(8, 0b0000'0000));
	ASSERT_EQ_BITSET(8, bus.at(1), 0b0000'0000);
};

TEST_F(BusTest, Load_Width_8_Single)
{
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

	bus.load(0, 0b1010'1111, 1);
	ASSERT_EQ_BITSET(8, bus.at(0), 0b1010'1111);
	ASSERT_EQ_BITSET(8, bus.at(1), 0b0000'0000);
};

TEST_F(BusTest, Load_Width_4)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

	bus.load(0, 0b1010'1111, 2);
	ASSERT_EQ_BITSET(4, bus.at(0), 0b1010);
	ASSERT_EQ_BITSET(4, bus.at(1), 0b1111);
};

TEST_F(BusTest, Load_HighImpedance_Width_4_0)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::Z, util::BusInitPatternSpec::L);
	
	// Bursts
	// -1 LLLL
	//  0 ZZZZ
	//  1 ZZZZ
	//  2 ZZZZ
	//  3 1010
	//  4 1111
	//  5 ZZZZ

	ASSERT_NO_DATA(bus.at(0));
	ASSERT_NO_DATA(bus.at(1));
	ASSERT_NO_DATA(bus.at(2));

	auto stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 0);

	bus.load(3, 0b1010'1111, 2);

	ASSERT_EQ_BITSET(4, bus.at(3), 0b1010);
	ASSERT_EQ_BITSET(4, bus.at(4), 0b1111);
	ASSERT_NO_DATA(bus.at(5));
	
	stats = bus.get_stats(4); // ZZZZ -> 1010
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 0);

	stats = bus.get_stats(5); // 1010 -> 1111
	ASSERT_EQ(stats.ones, 2 + 4);
	ASSERT_EQ(stats.zeroes, 2 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 2);
	ASSERT_EQ(stats.bit_changes, 0 + 2);
};

TEST_F(BusTest, Load_HighImpedance_Width_4_1)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::Z, util::BusInitPatternSpec::Z);

	// Bursts
	// -1 ZZZZ
	//  0 ZZZZ
	//  1 ZZZZ
	//  2 ZZZZ
	//  3 1010
	//  4 1111
	//  5 ZZZZ

	ASSERT_NO_DATA(bus.at(0));
	ASSERT_NO_DATA(bus.at(1));
	ASSERT_NO_DATA(bus.at(2));

	bus.load(3, 0b1010'1111, 2);

	ASSERT_EQ_BITSET(4, bus.at(3), 0b1010);
	ASSERT_EQ_BITSET(4, bus.at(4), 0b1111);
	ASSERT_NO_DATA(bus.at(5));

	auto stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 0);

	stats = bus.get_stats(4); // ZZZZ -> 1010
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 0);

	stats = bus.get_stats(5); // 1010 -> 1111
	ASSERT_EQ(stats.ones, 2 + 4);
	ASSERT_EQ(stats.zeroes, 2 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 2);
	ASSERT_EQ(stats.bit_changes, 0 + 2);
};

TEST_F(BusTest, Load_HighImpedance_Width_4_2)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::LAST_PATTERN, util::BusInitPatternSpec::Z);

	// Bursts
	// -1 ZZZZ
	//  0 ZZZZ
	//  1 ZZZZ
	//  2 ZZZZ
	//  3 1010
	//  4 0101
	//  5 0101
	//  6 0101

	ASSERT_NO_DATA(bus.at(0));
	ASSERT_NO_DATA(bus.at(1));
	ASSERT_NO_DATA(bus.at(2));

	bus.load(3, 0b1010'0101, 2);

	ASSERT_EQ_BITSET(4, bus.at(3), 0b1010);
	ASSERT_EQ_BITSET(4, bus.at(4), 0b0101);
	ASSERT_HAS_DATA(bus.at(5));
	ASSERT_HAS_DATA(bus.at(6));

	auto stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 0);

	stats = bus.get_stats(4); // ZZZZ -> 1010
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 0);

	stats = bus.get_stats(5); // 1010 -> 0101
	ASSERT_EQ(stats.ones, 2 + 2);
	ASSERT_EQ(stats.zeroes, 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 2);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 2);
	ASSERT_EQ(stats.bit_changes, 0 + 4);

	stats = bus.get_stats(7); // 0101 -> 0101 -> 0101
	ASSERT_EQ(stats.ones, 2 + 2 + 2 + 2);
	ASSERT_EQ(stats.zeroes, 2 + 2 + 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 2 + 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0 + 2 + 0 + 0);
	ASSERT_EQ(stats.bit_changes, 0 + 4 + 0 + 0);
};

TEST_F(BusTest, Load_HighImpedance_Width_4_3)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::Z, util::BusInitPatternSpec::L);

	// Bursts
	// -1 0000
	//  0 1010
	//  1 0101
	//  2 ZZZZ
	//  3 ZZZZ

	bus.load(0, 0b1010'0101, 2);
	ASSERT_EQ_BITSET(4, bus.at(0), 0b1010);
	ASSERT_EQ_BITSET(4, bus.at(1), 0b0101);
	ASSERT_NO_DATA(bus.at(2));
	ASSERT_NO_DATA(bus.at(3));

	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 0);

	stats = bus.get_stats(1); // 0000 -> 1010
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 2);
	ASSERT_EQ(stats.bit_changes, 2);

	stats = bus.get_stats(2); // 1010 -> 0101
	ASSERT_EQ(stats.ones, 2 + 2);
	ASSERT_EQ(stats.zeroes, 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 2);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2);
	ASSERT_EQ(stats.bit_changes, 2 + 4);

	stats = bus.get_stats(4); // 0101 -> ZZZZ -> ZZZZ
	ASSERT_EQ(stats.ones, 2 + 2 + 0 + 0);
	ASSERT_EQ(stats.zeroes, 2 + 2 + 0 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 2 + 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2 + 0 + 0);
	ASSERT_EQ(stats.bit_changes, 2 + 4 + 0 + 0);
};

TEST_F(BusTest, Load_Width_8)
{
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

	bus.load(0, 0b0010'1010'1001'0110, 2);
	ASSERT_EQ_BITSET(8, bus.at(0), 0b0010'1010);
	ASSERT_EQ_BITSET(8, bus.at(1), 0b1001'0110);
};


TEST_F(BusTest, Load_Width_4_Cont)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

	bus.load(0, 0b1010'1111, 2);
	ASSERT_EQ_BITSET(4, bus.at(0), 0b1010);
	ASSERT_EQ_BITSET(4, bus.at(1), 0b1111);
	ASSERT_EQ_BITSET(4, bus.at(2), 0b0000);

	bus.load(2, 0b0101'0001, 2);
	ASSERT_EQ_BITSET(4, bus.at(2), 0b0101);
	ASSERT_EQ_BITSET(4, bus.at(3), 0b0001);
	ASSERT_EQ_BITSET(4, bus.at(4), 0b0000);

	bus.load(5, 0b0101'0001'1111, 3);
	ASSERT_EQ_BITSET(4, bus.at(5), 0b0101);
	ASSERT_EQ_BITSET(4, bus.at(6), 0b0001);
	ASSERT_EQ_BITSET(4, bus.at(7), 0b1111);
	ASSERT_EQ_BITSET(4, bus.at(8), 0b0000);
};

TEST_F(BusTest, Stats_Empty_1)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);
	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.bit_changes, 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
};

TEST_F(BusTest, Stats_Empty_2)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);
	bus.load(0, 0b1010'1111, 2);
	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.bit_changes, 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
};

TEST_F(BusTest, Stats_Basic_1)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::H);

	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.bit_changes, 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);

	bus.load(1, 0b1111, 1);
	stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 4);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 4);

	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 4);
	ASSERT_EQ(stats.zeroes, 4);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 4);
	ASSERT_EQ(stats.bit_changes, 8);

	stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 4);
	ASSERT_EQ(stats.zeroes, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 8);
	ASSERT_EQ(stats.zeroes_to_ones, 4);
	ASSERT_EQ(stats.bit_changes, 12);

	stats = bus.get_stats(4);
	ASSERT_EQ(stats.ones, 4);
	ASSERT_EQ(stats.zeroes, 12);
	ASSERT_EQ(stats.ones_to_zeroes, 8);
	ASSERT_EQ(stats.zeroes_to_ones, 4);
	ASSERT_EQ(stats.bit_changes, 12);
};

TEST_F(BusTest, Stats_Basic_2)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::H);

	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.bit_changes, 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);

	stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 4);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 4);

	bus.load(2, 0b1111, 1);
	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 4);

	stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 4);
	ASSERT_EQ(stats.zeroes, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 4);
	ASSERT_EQ(stats.bit_changes, 8);

	stats = bus.get_stats(4);
	ASSERT_EQ(stats.ones, 4);
	ASSERT_EQ(stats.zeroes, 12);
	ASSERT_EQ(stats.ones_to_zeroes, 8);
	ASSERT_EQ(stats.zeroes_to_ones, 4);
	ASSERT_EQ(stats.bit_changes, 12);
};

TEST_F(BusTest, Stats_Basic_3)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::H);

	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.bit_changes, 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);

	stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 4);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 4);

	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 4);

	stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 12);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 4);

	bus.load(4, 0b1111, 1);
	stats = bus.get_stats(4);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 16);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
	ASSERT_EQ(stats.bit_changes, 4);
};

TEST_F(BusTest, Stats_4)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

	bus.load(0, 0b1010'1111, 2);

	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.bit_changes, 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);

	stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.bit_changes, 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 2);

	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 2 + 4);
	ASSERT_EQ(stats.zeroes, 2 + 0);
	ASSERT_EQ(stats.bit_changes, 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2);

	stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 2 + 4 + 0);
	ASSERT_EQ(stats.zeroes, 2 + 0 + 4);
	ASSERT_EQ(stats.bit_changes, 2 + 2 + 4);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0 + 4);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2 + 0);
};

TEST_F(BusTest, Stats_4_Idle)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

	bus.load(0, 0b1010'1111, 2);

	auto stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 6);
	ASSERT_EQ(stats.bit_changes, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 4);

	stats = bus.get_stats(4);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 6 + 4);
	ASSERT_EQ(stats.bit_changes, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 4);

	stats = bus.get_stats(5);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 6 + 4 + 4);
	ASSERT_EQ(stats.bit_changes, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 4);

};

TEST_F(BusTest, Stats_8)
{
	Bus_8 bus(8, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

	bus.load(0, 0b1010'1111'0110'1001, 2);
	// 1010'1111
	// 0110'1001

	auto stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.bit_changes, 6);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 6);

	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 6 + 4);
	ASSERT_EQ(stats.zeroes, 2 + 4);
	ASSERT_EQ(stats.bit_changes, 6 + 4);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 3);
	ASSERT_EQ(stats.zeroes_to_ones, 6 + 1);
};

TEST_F(BusTest, Stats_Second_Load_4)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

	bus.load(0, 0b1010'1111, 2);

	auto stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.bit_changes, 4);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 4);

	bus.load(2, 0b0110'1001, 2);

	stats = bus.get_stats(3);  // 1111 -> 0110
	ASSERT_EQ(stats.ones, 6 + 2);
	ASSERT_EQ(stats.zeroes, 2 + 2);
	ASSERT_EQ(stats.bit_changes, 4 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 2);
	ASSERT_EQ(stats.zeroes_to_ones, 4 + 0);

	stats = bus.get_stats(4);  // 0110 -> 1001
	ASSERT_EQ(stats.ones, 6 + 2 + 2);
	ASSERT_EQ(stats.zeroes, 2 + 2 + 2);
	ASSERT_EQ(stats.bit_changes, 4 + 2 + 4);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 2 + 2);
	ASSERT_EQ(stats.zeroes_to_ones, 4 + 0 + 2);
};

TEST_F(BusTest, Load_4_cycles)
{
	Bus_4 bus(4, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L);

	bus.load(0, 0b1010'1111'1001'0011, 4);

	/*
	1010'
	1111'
	1001'
	0011
	0000
	*/

	auto stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.bit_changes, 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 2);

	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 2 + 4);
	ASSERT_EQ(stats.zeroes, 2 + 0);
	ASSERT_EQ(stats.bit_changes, 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2);

	stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 2 + 4 + 2);
	ASSERT_EQ(stats.zeroes, 2 + 0 + 2);
	ASSERT_EQ(stats.bit_changes, 2 + 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0 + 2);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2 + 0);

	stats = bus.get_stats(4);
	ASSERT_EQ(stats.ones, 2 + 4 + 2 + 2);
	ASSERT_EQ(stats.zeroes, 2 + 0 + 2 + 2);
	ASSERT_EQ(stats.bit_changes, 2 + 2 + 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0 + 2 + 1);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2 + 0 + 1);

	stats = bus.get_stats(5);
	ASSERT_EQ(stats.ones, 2 + 4 + 2 + 2 + 0);
	ASSERT_EQ(stats.zeroes, 2 + 0 + 2 + 2 + 4);
	ASSERT_EQ(stats.bit_changes, 2 + 2 + 2 + 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0 + 2 + 1 + 2);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2 + 0 + 1 + 0);
};

TEST_F(BusTest, Load_Data)
{
	constexpr uint8_t data[] = {
		0, 0,
		0, 0b0000'0011,
		0, 0,
		0, 0b0000'0010,
		0, 0,
		0, 0b0000'0001,
		0, 0,
		0, 0b0000'0001,
	};

	Bus_16 bus{16, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L};

	bus.load(0, data, sizeof(data) * 8);

	auto stats = bus.get_stats(1); //  0x0000 -> 0...0b0000'0001
	ASSERT_EQ(stats.ones, 1);
	ASSERT_EQ(stats.zeroes, 15);
	ASSERT_EQ(stats.zeroes_to_ones, 1);
	ASSERT_EQ(stats.ones_to_zeroes, 0);

	stats = bus.get_stats(2); // 0...0b0000'0001 -> 0x0000
	ASSERT_EQ(stats.ones, 1 + 0);
	ASSERT_EQ(stats.zeroes, 15 + 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1);

	stats = bus.get_stats(3); //  0x0000 -> 0...0b0000'0001
	ASSERT_EQ(stats.ones, 1 + 0 + 1);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1);

	stats = bus.get_stats(4); // 0...0b0000'0001 -> 0x0000
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1);

	stats = bus.get_stats(5); //  0x0000 -> 0...0b0000'0010
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0 + 1);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16 + 15);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1 + 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1);

	stats = bus.get_stats(6); // 0...0b0000'0010 -> 0x0000
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0 + 1 + 0);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16 + 15 + 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1 + 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1 + 1);

	stats = bus.get_stats(7); //  0x0000 -> 0...0b0000'0011
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0 + 1 + 0 + 2);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16 + 15 + 16 + 14);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1 + 1 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1 + 1);

	stats = bus.get_stats(8); //  0...0b0000'0011 -> 0x0000
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0 + 1 + 0 + 2);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16 + 15 + 16 + 14 + 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1 + 1 + 2 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1 + 1 + 2);

	stats = bus.get_stats(9); //  0x0000 -> 0x0000
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0 + 1 + 0 + 2);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16 + 15 + 16 + 14 + 16 + 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1 + 1 + 2 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1 + 1 + 2);
};

TEST_F(BusTest, Test_001)
{
	// constexpr uint8_t data[] = {
	// 	0b1010'0000,
	// 	0b0000'0100,
	// 	0b0010'0110,
	// 	0b0001'0000,
	// 	0b0000'0000,
	// 	0b1000'0010,
	// };

	std::bitset<6 * 4> cmd_1("100000100000000000010000");
	std::bitset<6 * 4> cmd_2("001001100000010010100000");
	std::bitset<6 * 4> cmd_3("010001100000010010100000");

	/*
	1 0 0 0 0 0
	1 0 0 0 0 0
	0 0 0 0 0 0
	0 1 0 0 0 0
	0 0 1 0 0 1
	1 0 0 0 0 0
	0 1 0 0 1 0
	1 0 0 0 0 0
	*/


	Bus_6 bus{6, 1, util::BusIdlePatternSpec::L, util::BusInitPatternSpec::L};

	bus.load(0, cmd_1.to_ulong(), 4);

	auto stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 1);
	ASSERT_EQ(stats.zeroes, 5);
	ASSERT_EQ(stats.zeroes_to_ones, 1);
	ASSERT_EQ(stats.ones_to_zeroes, 0);

	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 10);
	ASSERT_EQ(stats.zeroes_to_ones, 1);
	ASSERT_EQ(stats.ones_to_zeroes, 0);

	stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1);

	stats = bus.get_stats(4);
	ASSERT_EQ(stats.ones, 3);
	ASSERT_EQ(stats.zeroes, 21);
	ASSERT_EQ(stats.zeroes_to_ones, 2);
	ASSERT_EQ(stats.ones_to_zeroes, 1);

	bus.load(4, cmd_2.to_ulong(), 4);

	stats = bus.get_stats(5);
	ASSERT_EQ(stats.ones, 5);
	ASSERT_EQ(stats.zeroes, 25);
	ASSERT_EQ(stats.zeroes_to_ones, 4);
	ASSERT_EQ(stats.ones_to_zeroes, 2);

	stats = bus.get_stats(6);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 30);
	ASSERT_EQ(stats.zeroes_to_ones, 5);
	ASSERT_EQ(stats.ones_to_zeroes, 4);

	stats = bus.get_stats(7);
	ASSERT_EQ(stats.ones, 8);
	ASSERT_EQ(stats.zeroes, 34);
	ASSERT_EQ(stats.zeroes_to_ones, 7);
	ASSERT_EQ(stats.ones_to_zeroes, 5);

	stats = bus.get_stats(8);
	ASSERT_EQ(stats.ones, 9);
	ASSERT_EQ(stats.zeroes, 39);
	ASSERT_EQ(stats.zeroes_to_ones, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 7);

	stats = bus.get_stats(9);
	ASSERT_EQ(stats.ones, 9);
	ASSERT_EQ(stats.zeroes, 45);
	ASSERT_EQ(stats.zeroes_to_ones, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 8);

	stats = bus.get_stats(10);

	bus.load(10, cmd_3.to_ulong(), 4);

	stats = bus.get_stats(11);
	ASSERT_EQ(stats.ones, 11);
	ASSERT_EQ(stats.zeroes, 55);
	ASSERT_EQ(stats.zeroes_to_ones, 10);
	ASSERT_EQ(stats.ones_to_zeroes, 8);
};