#include "DRAMPower/util/bus_types.h"
#include "DRAMPower/util/pin_types.h"
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>

#include <DRAMPower/util/pin.h>

using namespace DRAMPower;

// Use builder to make sure all fields are checked in the tests
struct StatsBuilder {
    using stats_t = util::bus_stats_t;

    StatsBuilder& z(uint64_t z) {
        m_mask |= 0b00001;
        m_target.zeroes = z;
        return *this;
    }

    StatsBuilder& o(uint64_t o) {
        m_mask |= 0b00010;
        m_target.ones = o;
        return *this;
    }

    StatsBuilder& zto(uint64_t z2o) {
        m_mask |= 0b00100;
        m_target.zeroes_to_ones = z2o;
        return *this;
    }

    StatsBuilder& otz(uint64_t o2z) {
        m_mask |= 0b01000;
        m_target.ones_to_zeroes = o2z;
        return *this;
    }

    StatsBuilder& bc(uint64_t bc) {
        m_mask |= 0b10000;
        m_target.bit_changes = bc;
        return *this;
    }

    StatsBuilder& infer_changes() {
        m_mask |= 0b10000;
        m_target.bit_changes = m_target.ones_to_zeroes + m_target.zeroes_to_ones;
        return *this;
    }

    stats_t build() {
        // All fields of stats must be set
        EXPECT_EQ(m_mask, 0b11111);
        return m_target;
    }

private:
    stats_t m_target{}; // Default constructor
    uint8_t m_mask = 0;
};

class PinTest : public ::testing::Test {
protected:

	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

TEST_F(PinTest, Idle_L_0)
{
    util::Pin<8> pin_8{util::PinState::H, util::PinState::L};
    auto builder = StatsBuilder{}
        .o(0)
        .zto(0)
        .otz(0)
        .infer_changes();

    // First cycle
    util::bus_stats_t stats_0_1 = pin_8.get_stats_at(0, 1);
    util::bus_stats_t stats_0_2 = pin_8.get_stats_at(0, 2);
    EXPECT_EQ(stats_0_1, stats_0_2);
    EXPECT_EQ(stats_0_1, builder.z(0).build());

    // Second cycle
    util::bus_stats_t stats_1_1 = pin_8.get_stats_at(1, 1);
    EXPECT_EQ(stats_1_1, builder.z(1).otz(1).infer_changes().build());

    util::bus_stats_t stats_48_1 = pin_8.get_stats_at(48, 1);
    util::bus_stats_t stats_24_2 = pin_8.get_stats_at(24, 2);
    EXPECT_EQ(stats_48_1, stats_24_2);
    EXPECT_EQ(stats_48_1, builder.z(48).build());

    util::bus_stats_t stats_96_1 = pin_8.get_stats_at(96, 1);
    util::bus_stats_t stats_48_2 = pin_8.get_stats_at(48, 2);
    EXPECT_EQ(stats_96_1, stats_48_2);
    EXPECT_EQ(stats_96_1, builder.z(96).build());

    // Cycles
    for (std::size_t i = 97; i < 1024; i ++) {
        EXPECT_EQ(pin_8.get_stats_at(i, 1), builder.z(i).build());
    }
};

TEST_F(PinTest, Idle_H_0)
{
    util::Pin<8> pin_8{util::PinState::L, util::PinState::H};
    auto builder = StatsBuilder{}
        .z(0)
        .zto(0)
        .otz(0)
        .infer_changes();

    // First cycle
    util::bus_stats_t stats_0_1 = pin_8.get_stats_at(0, 1);
    util::bus_stats_t stats_0_2 = pin_8.get_stats_at(0, 2);
    EXPECT_EQ(stats_0_1, stats_0_2);
    EXPECT_EQ(stats_0_1, builder.o(0).build());

    // Second cycle
    util::bus_stats_t stats_1_1 = pin_8.get_stats_at(1, 1);
    EXPECT_EQ(stats_1_1, builder.o(1).zto(1).infer_changes().build());

    util::bus_stats_t stats_48_1 = pin_8.get_stats_at(48, 1);
    util::bus_stats_t stats_24_2 = pin_8.get_stats_at(24, 2);
    EXPECT_EQ(stats_48_1, stats_24_2);
    EXPECT_EQ(stats_48_1, builder.o(48).infer_changes().build());

    util::bus_stats_t stats_96_1 = pin_8.get_stats_at(96, 1);
    util::bus_stats_t stats_48_2 = pin_8.get_stats_at(48, 2);
    EXPECT_EQ(stats_96_1, stats_48_2);
    EXPECT_EQ(stats_96_1, builder.o(96).build());

    // Cycles
    for (std::size_t i = 97; i < 1024; i ++) {
        EXPECT_EQ(pin_8.get_stats_at(i, 1), builder.o(i).build());
    }
};

TEST_F(PinTest, Load_Init_L_Idle_L_0)
{
    util::Pin<8> pin_8{util::PinState::L, util::PinState::L};

    pin_8.set(0, util::PinState::H);

    EXPECT_EQ( // t = 0
        pin_8.get_stats_at(0, 1),
        StatsBuilder{}.z(0).o(0).zto(0).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 1
        pin_8.get_stats_at(1, 1),
        StatsBuilder{}.z(0).o(1).zto(1).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 2
        pin_8.get_stats_at(2, 1),
        StatsBuilder{}.z(1).o(1).zto(1).otz(1).infer_changes().build()
    );
    EXPECT_EQ( // t = 23
        pin_8.get_stats_at(23, 1),
        StatsBuilder{}.z(22).o(1).zto(1).otz(1).infer_changes().build()
    );

    pin_8.set(24, util::PinState::L);

    EXPECT_EQ( // t = 45
        pin_8.get_stats_at(45, 1),
        StatsBuilder{}.z(44).o(1).zto(1).otz(1).infer_changes().build()
    );
};

TEST_F(PinTest, Load_Init_H_Idle_L_0)
{
    util::Pin<8> pin_8{util::PinState::H, util::PinState::L};

    pin_8.set(0, util::PinState::H);

    EXPECT_EQ( // t = 0
        pin_8.get_stats_at(0, 1),
        StatsBuilder{}.z(0).o(0).zto(0).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 1
        pin_8.get_stats_at(1, 1),
        StatsBuilder{}.z(0).o(1).zto(0).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 2
        pin_8.get_stats_at(2, 1),
        StatsBuilder{}.z(1).o(1).zto(0).otz(1).infer_changes().build()
    );
    EXPECT_EQ( // t = 23
        pin_8.get_stats_at(23, 1),
        StatsBuilder{}.z(22).o(1).zto(0).otz(1).infer_changes().build()
    );

    pin_8.set(24, util::PinState::L);

    EXPECT_EQ( // t = 45
        pin_8.get_stats_at(45, 1),
        StatsBuilder{}.z(44).o(1).zto(0).otz(1).infer_changes().build()
    );
};

TEST_F(PinTest, Load_Init_H_Idle_H_1)
{
    util::Pin<8> pin_8{util::PinState::H, util::PinState::H};

    EXPECT_EQ( // t = 0
        pin_8.get_stats_at(0, 1),
        StatsBuilder{}.z(0).o(0).zto(0).otz(0).infer_changes().build()
    );

    pin_8.set(1, util::PinState::L);
    
    EXPECT_EQ( // t = 1
        pin_8.get_stats_at(1, 1),
        StatsBuilder{}.z(0).o(1).zto(0).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 2
        pin_8.get_stats_at(2, 1),
        StatsBuilder{}.z(1).o(1).zto(0).otz(1).infer_changes().build()
    );
    EXPECT_EQ( // t = 3
        pin_8.get_stats_at(3, 1),
        StatsBuilder{}.z(1).o(2).zto(1).otz(1).infer_changes().build()
    );
    EXPECT_EQ( // t = 23
        pin_8.get_stats_at(23, 1),
        StatsBuilder{}.z(1).o(22).zto(1).otz(1).infer_changes().build()
    );

    pin_8.set(24, util::PinState::L);

    EXPECT_EQ( // t = 46
        pin_8.get_stats_at(46, 1),
        StatsBuilder{}.z(2).o(44).zto(2).otz(2).infer_changes().build()
    );
};

TEST_F(PinTest, Load_Init_L_Idle_H_1)
{
    util::Pin<8> pin_8{util::PinState::L, util::PinState::H};

    EXPECT_EQ( // t = 0
        pin_8.get_stats_at(0, 1),
        StatsBuilder{}.z(0).o(0).zto(0).otz(0).infer_changes().build()
    );

    pin_8.set(1, util::PinState::L);

    EXPECT_EQ( // t = 1
        pin_8.get_stats_at(1, 1),
        StatsBuilder{}.z(0).o(1).zto(1).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 2
        pin_8.get_stats_at(2, 1),
        StatsBuilder{}.z(1).o(1).zto(1).otz(1).infer_changes().build()
    );
    EXPECT_EQ( // t = 2
        pin_8.get_stats_at(3, 1),
        StatsBuilder{}.z(1).o(2).zto(2).otz(1).infer_changes().build()
    );
    EXPECT_EQ( // t = 23
        pin_8.get_stats_at(23, 1),
        StatsBuilder{}.z(1).o(22).zto(2).otz(1).infer_changes().build()
    );

    pin_8.set(24, util::PinState::L);

    EXPECT_EQ( // t = 45
        pin_8.get_stats_at(46, 1),
        StatsBuilder{}.z(2).o(44).zto(3).otz(2).infer_changes().build()
    );
};

TEST_F(PinTest, Idle_to_Burst_to_Idle)
{
    util::Pin<8> pin_8{util::PinState::L, util::PinState::L};

    EXPECT_EQ( // t = 9
        pin_8.get_stats_at(9, 1),
        StatsBuilder{}.z(9).o(0).zto(0).otz(0).infer_changes().build()
    );

    // Burst length 5
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    
    EXPECT_EQ( // t = 10
        pin_8.get_stats_at(10, 1),
        StatsBuilder{}.z(10).o(0).zto(0).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 11 // H
        pin_8.get_stats_at(11, 1),
        StatsBuilder{}.z(10).o(1).zto(1).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 12 // H
        pin_8.get_stats_at(12, 1),
        StatsBuilder{}.z(10).o(2).zto(1).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 13 // H
        pin_8.get_stats_at(13, 1),
        StatsBuilder{}.z(10).o(3).zto(1).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 14 // H
        pin_8.get_stats_at(14, 1),
        StatsBuilder{}.z(10).o(4).zto(1).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 15 // H
        pin_8.get_stats_at(15, 1),
        StatsBuilder{}.z(10).o(5).zto(1).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 16
        pin_8.get_stats_at(16, 1),
        StatsBuilder{}.z(11).o(5).zto(1).otz(1).infer_changes().build()
    );
    EXPECT_EQ( // t = 24
        pin_8.get_stats_at(24, 1),
        StatsBuilder{}.z(19).o(5).zto(1).otz(1).infer_changes().build()
    );

    // Burst length 3
    pin_8.set(25, util::PinState::H);
    pin_8.set(25, util::PinState::L);
    pin_8.set(25, util::PinState::H);

    
    EXPECT_EQ( // t = 25
        pin_8.get_stats_at(25, 1),
        StatsBuilder{}.z(20).o(5).zto(1).otz(1).infer_changes().build()
    );
    EXPECT_EQ( // t = 26 // H
        pin_8.get_stats_at(26, 1),
        StatsBuilder{}.z(20).o(6).zto(2).otz(1).infer_changes().build()
    );
    EXPECT_EQ( // t = 27 // L
        pin_8.get_stats_at(27, 1),
        StatsBuilder{}.z(21).o(6).zto(2).otz(2).infer_changes().build()
    );
    EXPECT_EQ( // t = 28 // H
        pin_8.get_stats_at(28, 1),
        StatsBuilder{}.z(21).o(7).zto(3).otz(2).infer_changes().build()
    );
    EXPECT_EQ( // t = 29
        pin_8.get_stats_at(29, 1),
        StatsBuilder{}.z(22).o(7).zto(3).otz(3).infer_changes().build()
    );

    EXPECT_EQ( // t = 48
        pin_8.get_stats_at(48, 1),
        StatsBuilder{}.z(41).o(7).zto(3).otz(3).infer_changes().build()
    );

};

TEST_F(PinTest, SeamlessBursts)
{

    util::Pin<8> pin_8{util::PinState::L, util::PinState::L};

    EXPECT_EQ( // t = 9
        pin_8.get_stats_at(9, 1),
        StatsBuilder{}.z(9).o(0).zto(0).otz(0).infer_changes().build()
    );

    // Burst length 5
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    
    EXPECT_EQ( // t = 10
        pin_8.get_stats_at(10, 1),
        StatsBuilder{}.z(10).o(0).zto(0).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 11 // B1: H
        pin_8.get_stats_at(11, 1),
        StatsBuilder{}.z(10).o(1).zto(1).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 12 // B1: H
        pin_8.get_stats_at(12, 1),
        StatsBuilder{}.z(10).o(2).zto(1).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 13 // B1: H
        pin_8.get_stats_at(13, 1),
        StatsBuilder{}.z(10).o(3).zto(1).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 14 // B1: H
        pin_8.get_stats_at(14, 1),
        StatsBuilder{}.z(10).o(4).zto(1).otz(0).infer_changes().build()
    );

    // Burst length 3
    pin_8.set(15, util::PinState::H);
    pin_8.set(15, util::PinState::L);
    pin_8.set(15, util::PinState::H);

    EXPECT_EQ( // t = 15 // B1: H
        pin_8.get_stats_at(15, 1),
        StatsBuilder{}.z(10).o(5).zto(1).otz(0).infer_changes().build()
    );

    EXPECT_EQ( // t = 16 // B2: H
        pin_8.get_stats_at(16, 1),
        StatsBuilder{}.z(10).o(6).zto(1).otz(0).infer_changes().build()
    );
    EXPECT_EQ( // t = 17 // B2: L
        pin_8.get_stats_at(17, 1),
        StatsBuilder{}.z(11).o(6).zto(1).otz(1).infer_changes().build()
    );
    EXPECT_EQ( // t = 18 // B2: H
        pin_8.get_stats_at(18, 1),
        StatsBuilder{}.z(11).o(7).zto(2).otz(1).infer_changes().build()
    );
    EXPECT_EQ( // t = 19
        pin_8.get_stats_at(19, 1),
        StatsBuilder{}.z(12).o(7).zto(2).otz(2).infer_changes().build()
    );

    EXPECT_EQ( // t = 48
        pin_8.get_stats_at(48, 1),
        StatsBuilder{}.z(41).o(7).zto(2).otz(2).infer_changes().build()
    );
};

TEST_F(PinTest, InvalidBurst)
{
    util::Pin<8> pin_8{util::PinState::L, util::PinState::L};

    EXPECT_EQ( // t = 9
        pin_8.get_stats_at(9, 1),
        StatsBuilder{}.z(9).o(0).zto(0).otz(0).infer_changes().build()
    );

    // Burst length 9
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    pin_8.set(10, util::PinState::H);
    EXPECT_DEATH(pin_8.set(10, util::PinState::H), "");
};
