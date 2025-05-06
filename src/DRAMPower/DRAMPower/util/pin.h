#ifndef DRAMPOWER_UTIL_PINH
#define DRAMPOWER_UTIL_PINH

#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus_types.h>
#include <DRAMPower/util/pin_types.h>
#include <DRAMPower/util/pending_stats.h>

#include <optional>
#include <cassert>

namespace DRAMPower::util {

class Pin {
// Public type definitions
public:
    using pin_stats_t = bus_stats_t;

// Constructor
public:
    Pin(std::size_t dataRate = 1, PinState state = PinState::Z);

// Private member functions
private:
    void addPendingStats(timestamp_t timestamp, pin_stats_t &stats) const;
    pin_stats_t getPinChangeStats(PinState &newState) const;
    void count(timestamp_t timestamp, pin_stats_t &stats, std::optional<std::size_t> datarate = std::nullopt) const;

// Public member functions
public:
    // The timestamp t is relative to the clock frequency
    void set(timestamp_t t, PinState state);
    // The timestamp t respects the data rate of the pin
    void set_with_datarate(timestamp_t t, PinState state);
    // The timestamp t is relative to the clock frequency
    pin_stats_t get_stats_at(timestamp_t t) const;

// Private member variables
private:
    PendingStats pending_stats;

    pin_stats_t m_stats;
    std::size_t m_dataRate;

    PinState m_last_state = PinState::Z;
    timestamp_t m_last_set = 0;

};

};

#endif /* DRAMPOWER_UTIL_PINH */
