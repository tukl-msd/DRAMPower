#include <gtest/gtest.h>

#include <DRAMPower/util/bus.h>

using namespace DRAMPower;


class BusTest : public ::testing::Test {
protected:
	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

#define ASSERT_EQ_BITSET(lhs, rhs) ASSERT_EQ(lhs, util::dynamic_bitset( lhs.size(), rhs))

TEST_F(BusTest, EmptyTest)
{
	util::Bus bus(8, util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	});

	ASSERT_EQ(bus.at(0), util::dynamic_bitset( 8, 0b0000'0000 ));
	ASSERT_EQ_BITSET(bus.at(1), 0b0000'0000);
};

TEST_F(BusTest, Load_Width_8_Single)
{
	util::Bus bus(8, util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	});

	bus.load(0, 0b1010'1111, 1);
	ASSERT_EQ_BITSET(bus.at(0), 0b1010'1111);
	ASSERT_EQ_BITSET(bus.at(1), 0b0000'0000);
};

TEST_F(BusTest, Load_Width_4)
{
	util::Bus bus(4, util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	});

	bus.load(0, 0b1010'1111, 2);
	ASSERT_EQ_BITSET(bus.at(0), 0b1010);
	ASSERT_EQ_BITSET(bus.at(1), 0b1111);
};

TEST_F(BusTest, Load_Width_8)
{
	util::Bus bus(8, util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	});

	bus.load(0, 0b0010'1010'1001'0110, 2);
	ASSERT_EQ_BITSET(bus.at(0), 0b0010'1010);
	ASSERT_EQ_BITSET(bus.at(1), 0b1001'0110);
};


TEST_F(BusTest, Load_Width_4_Cont)
{
	util::Bus bus(4, util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	});

	bus.load(0, 0b1010'1111, 2);
	ASSERT_EQ_BITSET(bus.at(0), 0b1010);
	ASSERT_EQ_BITSET(bus.at(1), 0b1111);
	ASSERT_EQ_BITSET(bus.at(2), 0b0000);

	bus.load(2, 0b0101'0001, 2);
	ASSERT_EQ_BITSET(bus.at(2), 0b0101);
	ASSERT_EQ_BITSET(bus.at(3), 0b0001);
	ASSERT_EQ_BITSET(bus.at(4), 0b0000);

	bus.load(5, 0b0101'0001'1111, 3);
	ASSERT_EQ_BITSET(bus.at(5), 0b0101);
	ASSERT_EQ_BITSET(bus.at(6), 0b0001);
	ASSERT_EQ_BITSET(bus.at(7), 0b1111);
	ASSERT_EQ_BITSET(bus.at(8), 0b0000);
};


TEST_F(BusTest, Stats_Empty)
{
	util::Bus bus(4, util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	});

	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.bit_changes, 0);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 0);
};

TEST_F(BusTest, Stats_4)
{
	util::Bus bus(4, util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	});

	bus.load(0, 0b1010'1111, 2);

	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.bit_changes, 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 2);

	stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 2 + 4);
	ASSERT_EQ(stats.zeroes, 2 + 0);
	ASSERT_EQ(stats.bit_changes, 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2);

	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 2 + 4 + 0);
	ASSERT_EQ(stats.zeroes, 2 + 0 + 4);
	ASSERT_EQ(stats.bit_changes, 2 + 2 + 4);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0 + 4);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2 + 0);
};

TEST_F(BusTest, Stats_4_Idle)
{
	util::Bus bus(4, util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	});

	bus.load(0, 0b1010'1111, 2);

	auto stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 6);
	ASSERT_EQ(stats.bit_changes, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 4);

	stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 6 + 4);
	ASSERT_EQ(stats.bit_changes, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 4);

	stats = bus.get_stats(4);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 6 + 4 + 4);
	ASSERT_EQ(stats.bit_changes, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 4);
	ASSERT_EQ(stats.zeroes_to_ones, 4);

};

TEST_F(BusTest, Stats_8)
{
	util::Bus bus(8, util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	});

	bus.load(0, 0b1010'1111'0110'1001, 2);
	// 1010'1111
	// 0110'1001

	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.bit_changes, 6);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 6);

	stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 6 + 4);
	ASSERT_EQ(stats.zeroes, 2 + 4);
	ASSERT_EQ(stats.bit_changes, 6 + 4);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 3);
	ASSERT_EQ(stats.zeroes_to_ones, 6 + 1);
};

TEST_F(BusTest, Stats_Second_Load_4)
{
	util::Bus bus(4, util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	});

	bus.load(0, 0b1010'1111, 2);

	auto stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.bit_changes, 4);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 4);

	bus.load(2, 0b0110'1001, 2);

	stats = bus.get_stats(2);  // 1111 -> 0110
	ASSERT_EQ(stats.ones, 6 + 2);
	ASSERT_EQ(stats.zeroes, 2 + 2);
	ASSERT_EQ(stats.bit_changes, 4 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 2);
	ASSERT_EQ(stats.zeroes_to_ones, 4 + 0);

	stats = bus.get_stats(3);  // 0110 -> 1001
	ASSERT_EQ(stats.ones, 6 + 2 + 2);
	ASSERT_EQ(stats.zeroes, 2 + 2 + 2);
	ASSERT_EQ(stats.bit_changes, 4 + 2 + 4);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 2 + 2);
	ASSERT_EQ(stats.zeroes_to_ones, 4 + 0 + 2);
};

TEST_F(BusTest, Load_4_cycles)
{
	util::Bus bus(4, util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	});

	bus.load(0, 0b1010'1111'1001'0011, 4);

	/*
	1010'
	1111'
	1001'
	0011
	0000
	*/

	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.bit_changes, 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0);
	ASSERT_EQ(stats.zeroes_to_ones, 2);

	stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 2 + 4);
	ASSERT_EQ(stats.zeroes, 2 + 0);
	ASSERT_EQ(stats.bit_changes, 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2);

	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 2 + 4 + 2);
	ASSERT_EQ(stats.zeroes, 2 + 0 + 2);
	ASSERT_EQ(stats.bit_changes, 2 + 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0 + 2);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2 + 0);

	stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 2 + 4 + 2 + 2);
	ASSERT_EQ(stats.zeroes, 2 + 0 + 2 + 2);
	ASSERT_EQ(stats.bit_changes, 2 + 2 + 2 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 0 + 0 + 2 + 1);
	ASSERT_EQ(stats.zeroes_to_ones, 2 + 2 + 0 + 1);

	stats = bus.get_stats(4);
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

	util::Bus bus{ 16 , util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	}};

	bus.load(0, data, sizeof(data) * 8);

	auto stats = bus.get_stats(0); //  0x0000 -> 0...0b0000'0001
	ASSERT_EQ(stats.ones, 1);
	ASSERT_EQ(stats.zeroes, 15);
	ASSERT_EQ(stats.zeroes_to_ones, 1);
	ASSERT_EQ(stats.ones_to_zeroes, 0);

	stats = bus.get_stats(1); // 0...0b0000'0001 -> 0x0000
	ASSERT_EQ(stats.ones, 1 + 0);
	ASSERT_EQ(stats.zeroes, 15 + 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1);

	stats = bus.get_stats(2); //  0x0000 -> 0...0b0000'0001
	ASSERT_EQ(stats.ones, 1 + 0 + 1);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1);

	stats = bus.get_stats(3); // 0...0b0000'0001 -> 0x0000
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1);

	stats = bus.get_stats(4); //  0x0000 -> 0...0b0000'0010
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0 + 1);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16 + 15);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1 + 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1);

	stats = bus.get_stats(5); // 0...0b0000'0010 -> 0x0000
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0 + 1 + 0);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16 + 15 + 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1 + 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1 + 1);

	stats = bus.get_stats(6); //  0x0000 -> 0...0b0000'0011
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0 + 1 + 0 + 2);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16 + 15 + 16 + 14);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1 + 1 + 2);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1 + 1);

	stats = bus.get_stats(7); //  0...0b0000'0011 -> 0x0000
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0 + 1 + 0 + 2);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16 + 15 + 16 + 14 + 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1 + 1 + 2 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1 + 1 + 2);

	stats = bus.get_stats(8); //  0x0000 -> 0x0000
	ASSERT_EQ(stats.ones, 1 + 0 + 1 + 0 + 1 + 0 + 2);
	ASSERT_EQ(stats.zeroes, 15 + 16 + 15 + 16 + 15 + 16 + 14 + 16 + 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1 + 1 + 1 + 2 + 0);
	ASSERT_EQ(stats.ones_to_zeroes, 1 + 1 + 1 + 2);
};

TEST_F(BusTest, Test_001)
{
	constexpr uint8_t data[] = {
		0b1010'0000,
		0b0000'0100,
		0b0010'0110,
		0b0001'0000,
		0b0000'0000,
		0b1000'0010,
	};

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


	util::Bus bus{ 6 , util::BusSettings{
		.idle_pattern = util::BusIdlePatternSpec::L
	}};

	bus.load(0, cmd_1.to_ulong(), 4);

	auto stats = bus.get_stats(0);
	ASSERT_EQ(stats.ones, 1);
	ASSERT_EQ(stats.zeroes, 5);
	ASSERT_EQ(stats.zeroes_to_ones, 1);
	ASSERT_EQ(stats.ones_to_zeroes, 0);

	stats = bus.get_stats(1);
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 10);
	ASSERT_EQ(stats.zeroes_to_ones, 1);
	ASSERT_EQ(stats.ones_to_zeroes, 0);

	stats = bus.get_stats(2);
	ASSERT_EQ(stats.ones, 2);
	ASSERT_EQ(stats.zeroes, 16);
	ASSERT_EQ(stats.zeroes_to_ones, 1);
	ASSERT_EQ(stats.ones_to_zeroes, 1);

	stats = bus.get_stats(3);
	ASSERT_EQ(stats.ones, 3);
	ASSERT_EQ(stats.zeroes, 21);
	ASSERT_EQ(stats.zeroes_to_ones, 2);
	ASSERT_EQ(stats.ones_to_zeroes, 1);

	bus.load(4, cmd_2.to_ulong(), 4);

	stats = bus.get_stats(4);
	ASSERT_EQ(stats.ones, 5);
	ASSERT_EQ(stats.zeroes, 25);
	ASSERT_EQ(stats.zeroes_to_ones, 4);
	ASSERT_EQ(stats.ones_to_zeroes, 2);

	stats = bus.get_stats(5);
	ASSERT_EQ(stats.ones, 6);
	ASSERT_EQ(stats.zeroes, 30);
	ASSERT_EQ(stats.zeroes_to_ones, 5);
	ASSERT_EQ(stats.ones_to_zeroes, 4);

	stats = bus.get_stats(6);
	ASSERT_EQ(stats.ones, 8);
	ASSERT_EQ(stats.zeroes, 34);
	ASSERT_EQ(stats.zeroes_to_ones, 7);
	ASSERT_EQ(stats.ones_to_zeroes, 5);

	stats = bus.get_stats(7);
	ASSERT_EQ(stats.ones, 9);
	ASSERT_EQ(stats.zeroes, 39);
	ASSERT_EQ(stats.zeroes_to_ones, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 7);

	stats = bus.get_stats(8);
	ASSERT_EQ(stats.ones, 9);
	ASSERT_EQ(stats.zeroes, 45);
	ASSERT_EQ(stats.zeroes_to_ones, 8);
	ASSERT_EQ(stats.ones_to_zeroes, 8);

	stats = bus.get_stats(9);

	bus.load(10, cmd_3.to_ulong(), 4);

	stats = bus.get_stats(10);
	ASSERT_EQ(stats.ones, 11);
	ASSERT_EQ(stats.zeroes, 55);
	ASSERT_EQ(stats.zeroes_to_ones, 10);
	ASSERT_EQ(stats.ones_to_zeroes, 8);
};