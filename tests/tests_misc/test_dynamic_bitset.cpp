#include <gtest/gtest.h>

#include <DRAMPower/util/dynamic_bitset.h>

using namespace DRAMPower;


class DynamicBitsetTest : public ::testing::Test {
protected:
	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

TEST_F(DynamicBitsetTest, Constructor_1)
{
	util::dynamic_bitset bitset{};
	ASSERT_EQ(bitset.size(), 0);
};

TEST_F(DynamicBitsetTest, Constructor_2)
{
	util::dynamic_bitset bitset{8, 0b11110011};
	ASSERT_EQ(bitset.size(), 8);
};

TEST_F(DynamicBitsetTest, Constructor_3)
{
	util::dynamic_bitset bitset{4, 0b101010};
	ASSERT_EQ(bitset.size(), 4);
};

TEST_F(DynamicBitsetTest, Clear_1)
{
	util::dynamic_bitset bitset{ 8, 0b11110011 };
	ASSERT_EQ(bitset.size(), 8);

	bitset.clear();
	ASSERT_EQ(bitset.size(), 0);
};

TEST_F(DynamicBitsetTest, PushBack_1)
{
	util::dynamic_bitset bitset;
	ASSERT_EQ(bitset.size(), 0);

	bitset.push_back(false);
	ASSERT_EQ(bitset.size(), 1);

	bitset.push_back(true);
	ASSERT_EQ(bitset.size(), 2);
};

TEST_F(DynamicBitsetTest, ElementAccess_1)
{
	util::dynamic_bitset bitset{ 4, 0b10101010 };
	ASSERT_EQ(bitset[0], false);
	ASSERT_EQ(bitset[1], true);
	ASSERT_EQ(bitset[2], false);
	ASSERT_EQ(bitset[3], true);
};

TEST_F(DynamicBitsetTest, ElementAccess_2)
{
	util::dynamic_bitset bitset;
	bitset.push_back(false);
	ASSERT_EQ(bitset[0], false);

	bitset.push_back(true);
	ASSERT_EQ(bitset[1], true);
};

TEST_F(DynamicBitsetTest, ElementAccess_3)
{
	util::dynamic_bitset bitset;
	bitset.push_back(false);
	ASSERT_EQ(bitset[0], false);

	bitset[0] = true;
	ASSERT_EQ(bitset[0], true);
};

TEST_F(DynamicBitsetTest, Count_1)
{
	util::dynamic_bitset bitset_1{ 4, 0b00001111 };
	util::dynamic_bitset bitset_2{ 4, 0b11110000 };
	util::dynamic_bitset bitset_3{ 6, 0b11111111 };
	util::dynamic_bitset bitset_4{ 8, 0b11110000 };
	util::dynamic_bitset bitset_5{ 8, 0b00001111 };
	util::dynamic_bitset bitset_6{ 8, 0b10010110 };

	ASSERT_EQ(bitset_1.count(), 4);
	ASSERT_EQ(bitset_2.count(), 0);
	ASSERT_EQ(bitset_3.count(), 6);
	ASSERT_EQ(bitset_4.count(), 4);
	ASSERT_EQ(bitset_5.count(), 4);
	ASSERT_EQ(bitset_6.count(), 4);
};

TEST_F(DynamicBitsetTest, Compare_1)
{
	util::dynamic_bitset bitset_1;
	util::dynamic_bitset bitset_2;

	ASSERT_EQ(bitset_1, bitset_2);
};

TEST_F(DynamicBitsetTest, Compare_2)
{
	util::dynamic_bitset bitset_1{ 4, 0b00001111 };
	util::dynamic_bitset bitset_2{ 4, 0b11111111 };
	util::dynamic_bitset bitset_3{ 4, 0b11110000 };
	util::dynamic_bitset bitset_4{ 8, 0b00001111 };

	ASSERT_EQ(bitset_1, bitset_1);
	ASSERT_EQ(bitset_1, bitset_2);

	ASSERT_NE(bitset_1, bitset_3);
	ASSERT_NE(bitset_2, bitset_3);
	ASSERT_NE(bitset_2, bitset_4);
};

TEST_F(DynamicBitsetTest, Compare_3)
{
	util::dynamic_bitset bitset_1{ 4, 0b00001111 };

	ASSERT_EQ(bitset_1, 0b1111);
	ASSERT_EQ(bitset_1, 0b00001111);
	ASSERT_EQ(bitset_1, 0b11111111);

	ASSERT_NE(bitset_1, 0b11);
	ASSERT_NE(bitset_1, 0b11110000);
};

TEST_F(DynamicBitsetTest, Flip_1)
{
	util::dynamic_bitset bitset;
	bitset.push_back(true);
	ASSERT_EQ(bitset[0], true);
	bitset.flip(0);
	ASSERT_EQ(bitset[0], false);
};

TEST_F(DynamicBitsetTest, Negate_1)
{
	util::dynamic_bitset bitset{8, 0b0000'1111};
	ASSERT_NE(bitset, ~bitset);
	ASSERT_EQ(~bitset, 0b1111'0000);
	ASSERT_NE(~bitset, 0b1111'1111);
	ASSERT_NE(~bitset, 0b0000'0000);
};

TEST_F(DynamicBitsetTest, BitwiseOp_1)
{
	util::dynamic_bitset bitset_1{ 8, 0b0000'0000 };
	util::dynamic_bitset bitset_2{ 8, 0b1111'1111 };
	util::dynamic_bitset bitset_3{ 8, 0b1000'0001 };
	util::dynamic_bitset bitset_4{ 8, 0b1001'1001 };

	ASSERT_EQ(bitset_1 & bitset_1, 0);
	ASSERT_EQ(bitset_1 & bitset_2, 0);
	ASSERT_EQ(bitset_2 & bitset_3, 0b1000'0001);
	ASSERT_EQ(bitset_3 & bitset_4, 0b1000'0001);
	ASSERT_EQ(bitset_4 & bitset_4, bitset_4);
};

TEST_F(DynamicBitsetTest, BitwiseOp_2)
{
	util::dynamic_bitset bitset_1{ 8, 0b0000'0000 };
	util::dynamic_bitset bitset_2{ 8, 0b1111'1111 };
	util::dynamic_bitset bitset_3{ 8, 0b1000'0000 };
	util::dynamic_bitset bitset_4{ 8, 0b0001'1001 };

	ASSERT_EQ(bitset_1 | bitset_1, 0);
	ASSERT_EQ(bitset_1 | bitset_2, 0b1111'1111);
	ASSERT_EQ(bitset_1 | bitset_3, bitset_3);
	ASSERT_EQ(bitset_3 | bitset_4, 0b1001'1001);
};

TEST_F(DynamicBitsetTest, BitwiseOp_3)
{
	util::dynamic_bitset bitset_1{ 8, 0b0000'0000 };
	util::dynamic_bitset bitset_2{ 8, 0b1110'1111 };
	util::dynamic_bitset bitset_3{ 8, 0b1000'0000 };
	util::dynamic_bitset bitset_4{ 8, 0b0001'1001 };

	ASSERT_EQ(bitset_1 ^ bitset_1, 0);
	ASSERT_EQ(bitset_1 ^ bitset_2, 0b1110'1111);
	ASSERT_EQ(bitset_2 ^ bitset_3, 0b0110'1111);
	ASSERT_EQ(bitset_1 ^ bitset_3, bitset_3);
	ASSERT_EQ(bitset_3 ^ bitset_4, 0b1001'1001);
};