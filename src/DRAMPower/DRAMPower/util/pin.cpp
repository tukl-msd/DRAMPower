#include <DRAMPower/util/pin.h>

namespace DRAMPower::util {

Pin::Pin(std::size_t dataRate, PinState state)
: m_dataRate(dataRate)
, m_last_state(state)
{}

Pin::pin_stats_t Pin::count(timestamp_t timestamp, PinState newState) {
    pin_stats_t stats;
    // Add Pending stats
    if (pending_stats.isPending()) {
        assert(pending_stats.getTimestamp() < timestamp);
        stats += pending_stats.getStats();
        pending_stats.clear();
    }
    // Add duration of lastState
    switch (m_last_state) {
        case PinState::L:
            stats.zeroes += (timestamp - m_last_set) * m_dataRate;
            break;
        case PinState::H:
            stats.ones += (timestamp - m_last_set) * m_dataRate;
            break;
        case PinState::Z:
            // Nothing to do
            break;
    }
    // Add pin change stats
    if (newState != m_last_state) {
        pin_stats_t stats;
        // L to H or H to L result in bit changes
        // X to Z or Z to X do not count
        if ((m_last_state == PinState::L && newState == PinState::H)) {
            stats.bit_changes++;
            stats.zeroes_to_ones++;
        } else if (m_last_state == PinState::H && newState == PinState::L) {
            stats.bit_changes++;
            stats.ones_to_zeroes++;
        }
        pending_stats.setPendingStats(timestamp, stats);
    }
    return stats;
}

void Pin::set(timestamp_t t, PinState state)
{
    assert(t > m_last_set);
    // count stats
    m_stats += count(t, state);
    // save new state
    m_last_set = t;
    m_last_state = state;
}

Pin::pin_stats_t Pin::get_stats_at(timestamp_t t)
{
    auto stats = m_stats;
    // Add stats from m_last_set to t
    if (t > m_last_set) {
        stats += count(t, m_last_state);
    };
    return stats;
}

} // namespace DRAMPower::util
