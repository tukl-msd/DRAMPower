#include <DRAMPower/util/pin.h>

namespace DRAMPower::util {

Pin::Pin(PinState state)
: m_last_state(state)
{}

void Pin::addPendingStats(timestamp_t t, pin_stats_t &stats) const {
    // add stats from last load stored in pending_stats
    if (pending_stats.isPending()) {
        assert(pending_stats.getTimestamp() < t);
        stats += getPinChangeStats(pending_stats.getStats().fromstate, pending_stats.getStats().newstate);
    }
}

Pin::pin_stats_t Pin::getPinChangeStats(const PinState &fromState, const PinState &newState) const {
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

void Pin::count(timestamp_t timestamp, pin_stats_t &stats) const {
    // Add duration of lastState
    switch (m_last_state) {
        case PinState::L:
            stats.zeroes += timestamp - m_last_set;
            break;
        case PinState::H:
            stats.ones += timestamp - m_last_set;
            break;
        case PinState::Z:
            // Nothing to do
            break;
    }
}

void Pin::set(timestamp_t t, PinState state, std::size_t dataRate)
{
    timestamp_t virtual_time = t * dataRate;
    assert(virtual_time > m_last_set);
    // count stats
    addPendingStats(virtual_time, m_stats);
    pending_stats.clear();
    count(virtual_time, m_stats);
    // store new state
    pending_stats.setPendingStats(virtual_time, {
        m_last_state, // From state
        state // New state
    });
    m_last_set = virtual_time;
    m_last_state = state;
}

Pin::pin_stats_t Pin::get_stats_at(timestamp_t t, std::size_t dataRate) const
{
    timestamp_t virtual_time = t * dataRate;
    assert(virtual_time >= m_last_set);
    if (virtual_time * dataRate == m_last_set) {
        return m_stats;
    }
    // virtual_time > m_last_set

    // Add stats from m_last_set to t
    auto stats = m_stats;
    addPendingStats(virtual_time, stats);
    count(virtual_time, stats);

    return stats;
}

} // namespace DRAMPower::util
