#include <DRAMPower/util/pin.h>

namespace DRAMPower::util {

Pin::Pin(PinState state)
: m_last_state(state)
{}

void Pin::addPendingStats(timestamp_t t, PendingStats<PinPendingStats> pending_stats, pin_stats_t &stats) const {
    // add stats from last load stored in pending_stats
    if (pending_stats.isPending()) {
        assert(pending_stats.getTimestamp() < t);
        stats += getPinChangeStats(pending_stats.getStats().fromstate, pending_stats.getStats().newstate);
    }
}

void Pin::addPendingStats(timestamp_t t, timestamp_t pending_t, PinState to, PinState from, pin_stats_t &stats) const {

    auto pending_change = PendingStats<PinPendingStats>{};
    pending_change.setPendingStats(pending_t, {
        from, // from
        to    // to
    });
    addPendingStats(t, pending_change, stats);
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

void Pin::count(timestamp_t end, timestamp_t start, const PinState& state, pin_stats_t &stats) const {
    if (end <= start) {
        return;
    }
    // Add duration of lastState
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

void Pin::set_internal(timestamp_t t, PinState state, std::size_t dataRate, PendingStats<PinPendingStats> pending_stats, pin_stats_t &stats) const {
    // Check if change is needed
    if (m_last_state == state) {
        return;
    }
    timestamp_t virtual_time = t * dataRate;
    assert(virtual_time > m_last_set);
    // count stats
    addPendingStats(virtual_time, pending_stats, stats);
    count(virtual_time, m_last_set, m_last_state, stats);
}

void Pin::set(timestamp_t t, PinState state, std::size_t dataRate)
{
    timestamp_t virtual_time = t * dataRate;
    set_internal(t, state, dataRate, m_pending_stats, m_stats);
    m_pending_stats.clear();
    m_pending_stats.setPendingStats(virtual_time, {
        m_last_state, // From state
        state // New state
    });
    m_last_set = virtual_time;
    m_last_state = state;
}

PinState Pin::get(timestamp_t timestamp, std::size_t dataRate) const
{
    timestamp_t virtual_time = timestamp * dataRate;
    assert(virtual_time >= m_last_set);
    if (virtual_time == m_last_set) {
        // Retrive lastState from pendingStats
        if (m_pending_stats.isPending()) {
            return m_pending_stats.getStats().fromstate;
        } else {
            return m_last_state; // No pending stats, return last state
        }
    }
    // virtual_time > m_last_set
    return m_last_state; // Return the last state
}

Pin::pin_stats_t Pin::get_stats_at(timestamp_t t, std::size_t dataRate, std::optional<std::reference_wrapper<const PinTempChange>> optionalstats) const
{
    timestamp_t virtual_time = t * dataRate;
    assert(virtual_time >= m_last_set);
    if (virtual_time == m_last_set) {
        return m_stats;
    }
    // Add stats from m_last_set to t
    auto stats = m_stats;
    // virtual_time > m_last_set
    if (optionalstats && m_pending_stats.isPending()) {
        // optionalstats && pending_stats
        const auto& optstats_v = optionalstats->get();
        if (optstats_v.change_time <= m_pending_stats.getTimestamp()) {
            // Only use pending_stats
            addPendingStats(virtual_time, m_pending_stats, stats);
            count(virtual_time, m_last_set, m_last_state, stats);
        } else {
            // optstats_v.change_time > m_pending_stats.getTimestamp()
            // Add pendingstats, Count to optional change, Add optional change, Count to end
            addPendingStats(virtual_time, m_pending_stats, stats);
            count(optstats_v.change_time, m_last_set, m_pending_stats.getStats().newstate, stats);
            addPendingStats(virtual_time, optstats_v.change_time, optstats_v.state, m_pending_stats.getStats().newstate, stats);
            count(virtual_time, optstats_v.change_time, optstats_v.state, stats);
        }
    } else if (optionalstats/* && !m_pending_stats.isPending()*/) {
        // optionalstats
        // Count to optional change, Add optional change, Count to end
        const auto& optstats_v = optionalstats->get();
        count(optstats_v.change_time, m_last_set, m_last_state, stats);
        addPendingStats(virtual_time, optstats_v.change_time, optstats_v.state, m_last_state, stats);
        count(virtual_time, optstats_v.change_time, optstats_v.state, stats);
    } else {
        // pendingchange || !pending_stats
        // Add pendingstats, count to end
        addPendingStats(virtual_time, m_pending_stats, stats);
        count(virtual_time, m_last_set, m_last_state, stats);
    }

    return stats;
}

void Pin::serialize(std::ostream &stream) const {
    stream.write(reinterpret_cast<const char *>(&m_last_state), sizeof(m_last_state));
    stream.write(reinterpret_cast<const char *>(&m_last_set), sizeof(m_last_set));
    m_pending_stats.serialize(stream);
    m_stats.serialize(stream);
}

void Pin::deserialize(std::istream &stream) {
    stream.read(reinterpret_cast<char *>(&m_last_state), sizeof(m_last_state));
    stream.read(reinterpret_cast<char *>(&m_last_set), sizeof(m_last_set));
    m_pending_stats.deserialize(stream);
    m_stats.deserialize(stream);
}

} // namespace DRAMPower::util