#include <gtest/gtest.h>

#include <DRAMPower/util/clock.h>

using namespace DRAMPower;


class ClockTest : public ::testing::Test {
protected:
	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

TEST_F(ClockTest, Test_1)
{
	util::Clock clock;

	util::Clock::clock_stats_t stats;

	stats = clock.get_stats_at(0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.ones, 0);

	stats = clock.get_stats_at(1);
	ASSERT_EQ(stats.zeroes, 1);
	ASSERT_EQ(stats.ones, 1);

	stats = clock.get_stats_at(2);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.ones, 2);

	stats = clock.get_stats_at(10);
	ASSERT_EQ(stats.zeroes, 10);
	ASSERT_EQ(stats.ones, 10);
};

TEST_F(ClockTest, Test_Period)
{
	util::Clock clock;

	util::Clock::clock_stats_t stats;

	// _ _ - - _ _ - -

	stats = clock.get_stats_at(0);
	ASSERT_EQ(stats.zeroes, 0);
	ASSERT_EQ(stats.ones, 0);

	stats = clock.get_stats_at(1);
	ASSERT_EQ(stats.zeroes, 1);
	ASSERT_EQ(stats.ones, 1);

	stats = clock.get_stats_at(2);
	ASSERT_EQ(stats.zeroes, 2);
	ASSERT_EQ(stats.ones, 2);

	stats = clock.get_stats_at(3);
	ASSERT_EQ(stats.zeroes, 3);
	ASSERT_EQ(stats.ones, 3);

	stats = clock.get_stats_at(4);
	ASSERT_EQ(stats.zeroes, 4);
	ASSERT_EQ(stats.ones, 4);	
	
	stats = clock.get_stats_at(5);
	ASSERT_EQ(stats.zeroes, 5);
	ASSERT_EQ(stats.ones, 5);

	stats = clock.get_stats_at(6);
	ASSERT_EQ(stats.zeroes, 6);
	ASSERT_EQ(stats.ones, 6);

	stats = clock.get_stats_at(7);
	ASSERT_EQ(stats.zeroes, 7);
	ASSERT_EQ(stats.ones, 7);
};

TEST_F(ClockTest, Test_Stop_Start)
{
	util::Clock clock;

	util::Clock::clock_stats_t stats;

	clock.stop(5);

	stats = clock.get_stats_at(5);
	ASSERT_EQ(stats.zeroes, 5);
	ASSERT_EQ(stats.ones, 5);

	stats = clock.get_stats_at(6);
	ASSERT_EQ(stats.zeroes, 5);
	ASSERT_EQ(stats.ones, 5);

	clock.start(7);

	stats = clock.get_stats_at(7);
	ASSERT_EQ(stats.zeroes, 5);
	ASSERT_EQ(stats.ones, 5);

	stats = clock.get_stats_at(8);
	ASSERT_EQ(stats.zeroes, 6);
	ASSERT_EQ(stats.ones, 6);
};