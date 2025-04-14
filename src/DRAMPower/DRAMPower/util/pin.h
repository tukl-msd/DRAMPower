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
    pin_stats_t count(timestamp_t timestamp, PinState newState);

// Public member functions
public:
    void set(timestamp_t t, PinState state);
    pin_stats_t get_stats_at(timestamp_t t);

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
