#include <gtest/gtest.h>

#include <DRAMPower/util/cycle_stats.h>

using namespace DRAMPower;


class IntervalTest : public ::testing::Test {
protected:
	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

TEST_F(IntervalTest, Constructor)
{
	util::interval_counter<uint64_t> i;

	ASSERT_FALSE(i.is_open());
	ASSERT_FALSE(i.is_closed());
	ASSERT_EQ(i.get_count(), 0);
	ASSERT_EQ(i.get_count_at(50), 0);
};

TEST_F(IntervalTest, Open)
{
	util::interval_counter<uint64_t> i(20);

	ASSERT_TRUE(i.is_open());
	ASSERT_FALSE(i.is_closed());

	ASSERT_EQ(i.get_start(), 20);

	ASSERT_EQ(i.get_count(), 0);
	ASSERT_EQ(i.get_count_at(15), 0);
	ASSERT_EQ(i.get_count_at(20),  0);
	ASSERT_EQ(i.get_count_at(25),  5);
};

TEST_F(IntervalTest, Close)
{
	util::interval_counter<uint64_t> i(20);

	ASSERT_TRUE(i.is_open());
	ASSERT_FALSE(i.is_closed());

	ASSERT_EQ(i.close_interval(30), 10);

	ASSERT_FALSE(i.is_open());
	ASSERT_TRUE(i.is_closed());

	ASSERT_EQ(i.get_count(), 10);
	ASSERT_EQ(i.get_count_at(30), 10);
	ASSERT_EQ(i.get_count_at(35), 10);
};

TEST_F(IntervalTest, Reset)
{
	util::interval_counter<uint64_t> i;
	i.start_interval(20);
	ASSERT_EQ(i.close_interval(30), 10);

	i.reset_interval();

	ASSERT_FALSE(i.is_open());
	ASSERT_FALSE(i.is_closed());

	ASSERT_EQ(i.get_count(), 10);
	ASSERT_EQ(i.get_count_at(30), 10);
	ASSERT_EQ(i.get_count_at(35), 10);
};


TEST_F(IntervalTest, Restart)
{
	util::interval_counter<uint64_t> i;
	i.start_interval(20);
	ASSERT_EQ(i.close_interval(30), 10);

	i.start_interval(50);

	ASSERT_TRUE(i.is_open());
	ASSERT_FALSE(i.is_closed());

	ASSERT_EQ(i.get_count(), 10);
	ASSERT_EQ(i.get_count_at(55), 15);

	ASSERT_EQ(i.close_interval(60), 10);
	ASSERT_EQ(i.get_count(), 20);
	ASSERT_EQ(i.get_count_at(65), 20);
};