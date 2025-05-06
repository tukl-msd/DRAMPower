#include <DRAMPower/util/pin.h>

namespace DRAMPower::util {

Pin::Pin(std::size_t dataRate, PinState state)
: m_dataRate(dataRate)
, m_last_state(state)
{}

void Pin::addPendingStats(timestamp_t timestamp, pin_stats_t &stats) const {
    if (pending_stats.isPending()) {
        assert(pending_stats.getTimestamp() < timestamp);
        stats += pending_stats.getStats();
    }
}

Pin::pin_stats_t Pin::getPinChangeStats(PinState &newState) const {
    pin_stats_t stats;
    // Add pin change stats
    if (newState != m_last_state) {
        // L to H or H to L result in bit changes
        // X to Z or Z to X do not count
        if ((m_last_state == PinState::L && newState == PinState::H)) {
            stats.bit_changes++;
            stats.zeroes_to_ones++;
        } else if (m_last_state == PinState::H && newState == PinState::L) {
            stats.bit_changes++;
            stats.ones_to_zeroes++;
        }
    }
    return stats;
}

void Pin::count(timestamp_t timestamp, pin_stats_t &stats, std::optional<std::size_t> datarate) const {
    // Add duration of lastState
    std::size_t i_dataRate = datarate.value_or(m_dataRate);
    switch (m_last_state) {
        case PinState::L:
            stats.zeroes += (timestamp - m_last_set) * i_dataRate;
            break;
        case PinState::H:
            stats.ones += (timestamp - m_last_set) * i_dataRate;
            break;
        case PinState::Z:
            // Nothing to do
            break;
    }
}

void Pin::set_with_datarate(timestamp_t t, PinState state)
{
    assert(t > m_last_set);
    // count stats
    addPendingStats(t, m_stats);
    count(t, m_stats);
    // store new state
    pending_stats.setPendingStats(t, getPinChangeStats(state));
    // save new state
    m_last_set = t;
    m_last_state = state;
}

void Pin::set(timestamp_t t, PinState state)
{
    timestamp_t virtual_time = t * m_dataRate;
    set_with_datarate(virtual_time, state);
}

Pin::pin_stats_t Pin::get_stats_at(timestamp_t t) const
{
    if (t <= m_last_set) {
        return m_stats;
    }

    // Add stats from m_last_set to t
    auto stats = m_stats;
    addPendingStats(t, stats);
    count(t, stats);

    return stats;
}

} // namespace DRAMPower::util
