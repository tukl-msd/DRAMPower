#ifndef DRAMPOWER_UTIL_PINH
#define DRAMPOWER_UTIL_PINH

#include <DRAMPower/util/burst_storage.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus_types.h>
#include <DRAMPower/util/pin_types.h>
#include <DRAMPower/util/pending_stats.h>

#include <cassert>
#include <cstddef>

namespace DRAMPower::util {

struct PinPendingStats {
    PinState fromstate;
    PinState newstate;

    PinPendingStats() = default;
    PinPendingStats(PinState from, PinState to) : fromstate(from), newstate(to) {}
};

struct PinTempChange {
    timestamp_t change_time;
    PinState state;
};

template<std::size_t max_burst_length>
class Pin {
// Public type definitions
public:
    using pin_stats_t = bus_stats_t;
    using burst_storage_t = util::burst_storage<util::BitsetContainer<max_burst_length>>;
    using burst_t = typename burst_storage_t::burst_t;

// Constructor
public:
    explicit Pin(PinState initstate, PinState idlestate)
        : m_last_state(initstate)
        , m_idle_state(idlestate)
    {}

// Private member functions
private:
    void addPendingStats(timestamp_t t, PendingStats<PinPendingStats> pending_stats, pin_stats_t &stats) const {
        // add stats from last load stored in pending_stats
        if (pending_stats.isPending() && pending_stats.getTimestamp() < t) {
            stats += getPinChangeStats(pending_stats.getStats().fromstate, pending_stats.getStats().newstate);
        }
    }

    void addPendingStats(timestamp_t t, timestamp_t pending_t, PinState to, PinState from, pin_stats_t &stats) const {
        auto pending_change = PendingStats<PinPendingStats>{};
        pending_change.setPendingStats(pending_t, {
            from, // from
            to    // to
        });
        addPendingStats(t, pending_change, stats);
    }

   [[nodiscard]] pin_stats_t getPinChangeStats(const PinState &fromState, const PinState &newState) const {
        pin_stats_t stats;
        // Add pin change stats
        if (newState != fromState) {
            // L to H or H to L result in bit changes
            // X to Z or Z to X are not counted
            if (fromState == PinState::L && newState == PinState::H) {
                stats.bit_changes++;
                stats.zeroes_to_ones++;
            } else if (fromState == PinState::H && newState == PinState::L) {
                stats.bit_changes++;
                stats.ones_to_zeroes++;
            }
        }
        return stats;
    }

    void count(timestamp_t end, timestamp_t start, const PinState& state, pin_stats_t &stats) const {
        if (end <= start) {
            return;
        }

        // Burst Storage
        if (m_burst_storage.endTime() > start) { // TODO
            stats += m_burst_storage.count(start, end);
            // Adjust start for idle counting
            start = m_burst_storage.endTime();
            // Transitions last burst to idle_pattern
            if (end > start) {
                burst_t lastburst = m_burst_storage.get_burst(0);
                stats += getPinChangeStats(lastburst ? PinState::H : PinState::L, state);
            }
        }

        // Add duration of state
        if (end > start) {
            switch (state) {
                case PinState::L:
                    stats.zeroes += end - start;
                    break;
                case PinState::H:
                    stats.ones += end - start;
                    break;
                case PinState::Z:
                    // Nothing to do
                    break;
            }
        }
    }
    void set_internal(timestamp_t t, std::size_t dataRate, PendingStats<PinPendingStats> pending_stats, pin_stats_t &stats) const {
        // Check if change is needed
        // if (m_last_state == state) {
        //     return;
        // }
        timestamp_t virtual_time = t * dataRate;
        assert(virtual_time >= m_last_set);

        // Count init transition
        if (m_init_load && (0 < virtual_time)) {
            stats += getPinChangeStats(m_last_state, m_idle_state);
        }

        // count stats
        addPendingStats(virtual_time, pending_stats, stats);
        count(virtual_time, m_last_set, m_idle_state, stats);
    }

// Public member functions
public:
    // The timestamp t is relative to the clock frequency
    void set(timestamp_t load_time, PinState state, std::size_t dataRate = 1) {
        timestamp_t virtual_load_time  = load_time * dataRate;

        // Count stats to virtual_load_time
        set_internal(virtual_load_time, dataRate, m_pending_stats, m_stats);
        if (m_pending_stats.isPending() && m_pending_stats.getTimestamp() < virtual_load_time) {
            m_pending_stats.clear();
        }


        if ((m_last_set != virtual_load_time) || m_init_load) {
            // New Burst
            const PinState& laststate =  [this, virtual_load_time](){
                if ((virtual_load_time == m_last_set + m_burst_storage.size())
                    || (m_init_load && (0 == virtual_load_time))) {
                    // seamless or first load
                    return m_last_state;
                }
                return m_idle_state;
            }();
            m_pending_stats.setPendingStats(virtual_load_time, {
                laststate, // From state
                state // New state
            });
        }
        m_burst_storage.push_back(virtual_load_time, PinState::H == state);

        m_last_set = virtual_load_time;
        m_last_state = state;
        m_init_load = false;
    }

    // The timestamp t is relative to the clock frequency
    [[nodiscard]] pin_stats_t get_stats_at(timestamp_t t, std::size_t dataRate = 1) const {
        timestamp_t virtual_time = t * dataRate;
        assert(virtual_time >= m_last_set);
        if (virtual_time == m_last_set) {
            return m_stats;
        }
        // virtual_time > m_last_set

    
        // Add stats from m_last_set to t
        auto stats = m_stats;

        // Init stats
        if (m_init_load && (0 < virtual_time)) {
            stats += getPinChangeStats(m_last_state, m_idle_state);
        }

        addPendingStats(virtual_time, m_pending_stats, stats);
        count(virtual_time, m_last_set, m_idle_state, stats);
        return stats;
    }

// Private member variables
private:
    PendingStats<PinPendingStats> m_pending_stats;

    pin_stats_t m_stats;

    PinState m_last_state = PinState::Z;
    PinState m_idle_state = PinState::L;

    bool m_init_load = true;

    timestamp_t m_last_set = 0;
    burst_storage_t m_burst_storage{};

};

};

#endif /* DRAMPOWER_UTIL_PINH */