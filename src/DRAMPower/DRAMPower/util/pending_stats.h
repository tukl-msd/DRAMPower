#ifndef DRAMPOWER_UTIL_PENDING_STATS
#define DRAMPOWER_UTIL_PENDING_STATS

#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus_types.h>

namespace DRAMPower::util
{

template <typename T = bus_stats_t>
class PendingStats
{
// Constructor, assignment operator and destructor
public:
    PendingStats()
    : m_timestamp(0)
    , m_stats()
    , m_pending(false)
    {}
    ~PendingStats() = default;

    PendingStats(const PendingStats&) = default;
    PendingStats& operator=(const PendingStats&) = default;
    PendingStats(PendingStats&&) = default;
    PendingStats& operator=(PendingStats&&) = default;

// Public member functions
public:
    void setPendingStats(timestamp_t timestamp, T stats)
    {
        m_timestamp = timestamp;
        m_stats = stats;
        m_pending = true;
    }
    
    bool isPending() const
    {
        return m_pending;
    }

    void clear()
    {
        m_pending = false;
    }
    
    timestamp_t getTimestamp() const
    {
        return m_timestamp;
    }
    
    T getStats() const
    {
        return m_stats;
    }

// Private member variables
private:
    timestamp_t m_timestamp;
    T           m_stats;
    bool        m_pending;
};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_PENDING_STATS */
