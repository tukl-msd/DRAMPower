#include <DRAMPower/util/pending_stats.h>

namespace DRAMPower::util
{

PendingStats::PendingStats()
: m_timestamp(0)
, m_stats()
, m_pending(false)
{}

void PendingStats::setPendingStats(timestamp_t timestamp, bus_stats_t stats)
{
    m_timestamp = timestamp;
    m_stats = stats;
    m_pending = true;
}

bool PendingStats::isPending() const
{
    return m_pending;
}

void PendingStats::clear()
{
    m_pending = false;
}

timestamp_t PendingStats::getTimestamp() const
{
    return m_timestamp;
}

bus_stats_t PendingStats::getStats() const
{
    return m_stats;
}


} // namespace DRAMPower::util