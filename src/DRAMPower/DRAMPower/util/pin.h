#ifndef DRAMPOWER_UTIL_PINH
#define DRAMPOWER_UTIL_PINH

#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus_types.h>
#include <DRAMPower/util/pin_types.h>
#include <DRAMPower/util/pending_stats.h>
#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

#include <cassert>
#include <cstddef>
#include <optional>
#include <functional>

namespace DRAMPower::util {

struct PinPendingStats : public Serialize, public Deserialize {
    PinState fromstate;
    PinState newstate;

    PinPendingStats() = default;
    PinPendingStats(PinState from, PinState to) : fromstate(from), newstate(to) {}

    void serialize(std::ostream &stream) const override {
        stream.write(reinterpret_cast<const char *>(&fromstate), sizeof(fromstate));
        stream.write(reinterpret_cast<const char *>(&newstate), sizeof(newstate));
    }
    void deserialize(std::istream &stream) override {
        stream.read(reinterpret_cast<char *>(&fromstate), sizeof(fromstate));
        stream.read(reinterpret_cast<char *>(&newstate), sizeof(newstate));
    }
};

struct PinTempChange {
    timestamp_t change_time;
    PinState state;
};

class Pin : public Serialize, public Deserialize {
// Public type definitions
public:
    using pin_stats_t = bus_stats_t;

// Constructor
public:
    explicit Pin(PinState state);

// Private member functions
private:
    void addPendingStats(timestamp_t t, PendingStats<PinPendingStats> pending_stats, pin_stats_t &stats) const;
    void addPendingStats(timestamp_t t, timestamp_t pending_t, PinState to, PinState from, pin_stats_t &stats) const;
   [[nodiscard]] pin_stats_t getPinChangeStats(const PinState &fromState, const PinState &newState) const;
    void count(timestamp_t end, timestamp_t start, const PinState& state, pin_stats_t &stats) const;
    void set_internal(timestamp_t t, PinState state, std::size_t dataRate, PendingStats<PinPendingStats> pending_stats, pin_stats_t &stats) const;

// Public member functions
public:
    // The timestamp t is relative to the clock frequency
    void set(timestamp_t t, PinState state, std::size_t datarate = 1);
    // The timestamp t is relative to the clock frequency
    pin_stats_t get_stats_at(timestamp_t t, std::size_t dataRate = 1, std::optional<std::reference_wrapper<const PinTempChange>> = std::nullopt) const;

    PinState get(timestamp_t timestampp, std::size_t dataRate = 1) const;

// Overrides
public:
    void serialize(std::ostream &stream) const override;
    void deserialize(std::istream &stream) override;

// Private member variables
private:
    PendingStats<PinPendingStats> m_pending_stats;

    pin_stats_t m_stats;

    PinState m_last_state = PinState::Z;
    timestamp_t m_last_set = 0;

};

};

#endif /* DRAMPOWER_UTIL_PINH */