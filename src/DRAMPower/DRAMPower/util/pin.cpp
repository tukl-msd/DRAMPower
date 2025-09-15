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

PinState Pin::get(timestamp_t timestamp, std::size_t dataRate)
{
    timestamp_t virtual_time = timestamp * dataRate;
    assert(virtual_time >= m_last_set);
    if (virtual_time == m_last_set) {
        // Retrive lastState from pendingStats
        if (pending_stats.isPending()) {
            return pending_stats.getStats().fromstate;
        } else {
            return m_last_state; // No pending stats, return last state
        }
    }
    // Add stats before returning the state
    if (pending_stats.isPending() && pending_stats.getTimestamp() < virtual_time) {
        addPendingStats(virtual_time, m_stats);
        pending_stats.clear();
    }
    // m_last_state doesn't change
    count(virtual_time, m_stats);
    m_last_set = virtual_time;
    return m_last_state; // Return the last state
}

Pin::pin_stats_t Pin::get_stats_at(timestamp_t t, std::size_t dataRate) const
{
    timestamp_t virtual_time = t * dataRate;
    assert(virtual_time >= m_last_set);
    if (virtual_time == m_last_set) {
        return m_stats;
    }
    // virtual_time > m_last_set

    // Add stats from m_last_set to t
    auto stats = m_stats;
    addPendingStats(virtual_time, stats);
    count(virtual_time, stats);

    return stats;
}

void Pin::serialize(std::ostream &stream) const {
    stream.write(reinterpret_cast<const char *>(&m_last_state), sizeof(m_last_state));
    stream.write(reinterpret_cast<const char *>(&m_last_set), sizeof(m_last_set));
    pending_stats.serialize(stream);
    m_stats.serialize(stream);
}

void Pin::deserialize(std::istream &stream) {
    stream.read(reinterpret_cast<char *>(&m_last_state), sizeof(m_last_state));
    stream.read(reinterpret_cast<char *>(&m_last_set), sizeof(m_last_set));
    pending_stats.deserialize(stream);
    m_stats.deserialize(stream);
}

} // namespace DRAMPower::util
