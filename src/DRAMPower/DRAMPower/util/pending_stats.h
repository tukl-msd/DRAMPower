#ifndef DRAMPOWER_UTIL_PENDING_STATS
#define DRAMPOWER_UTIL_PENDING_STATS

#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus_types.h>

namespace DRAMPower::util
{

class PendingStats
{
// Constructor, assignment operator and destructor
public:
    PendingStats();
    ~PendingStats() = default;

    PendingStats(const PendingStats&) = default;
    PendingStats& operator=(const PendingStats&) = default;
    PendingStats(PendingStats&&) = default;
    PendingStats& operator=(PendingStats&&) = default;

// Public member functions
public:
    void setPendingStats(timestamp_t timestamp, bus_stats_t stats);
    bool isPending() const;
    void clear();
    timestamp_t getTimestamp() const;
    bus_stats_t getStats() const;

// Private member variables
private:
    timestamp_t m_timestamp;
    bus_stats_t m_stats;
    bool        m_pending;
};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_PENDING_STATS */
