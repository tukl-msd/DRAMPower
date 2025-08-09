#ifndef DRAMPOWER_UTIL_CLOCK_H
#define DRAMPOWER_UTIL_CLOCK_H

#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus_types.h>
#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

#include <optional>
#include <cassert>
#include <cstddef>

namespace DRAMPower::util {

class Clock : public Serialize, public Deserialize {
public:
    using clock_stats_t = bus_stats_t;

private:
    std::optional<timestamp_t> last_start;
    clock_stats_t stats;
    std::size_t dataRate;

private:
    clock_stats_t count(timestamp_t duration) const
    {
        // __--__--__--__--__--__--__--__--__--__--__--__--
        // f(t)	= t / 2;

        clock_stats_t stats;
        stats.ones = duration * dataRate / 2;
        stats.zeroes = duration * dataRate / 2;
        stats.zeroes_to_ones = duration * dataRate / 2;
        stats.ones_to_zeroes = duration * dataRate / 2;
        stats.bit_changes = stats.zeroes_to_ones + stats.ones_to_zeroes;
        return stats;
    };

public:
    Clock(std::size_t _dataRate = 2, bool stopped = false)
        : dataRate(_dataRate)
    {
        if (stopped)
            last_start = std::nullopt;
        else
            last_start = 0;
    };

public:
    void stop(timestamp_t t)
    {
        assert(last_start.has_value());
        assert(*last_start < t);

        this->stats += count(t - *last_start);
        last_start.reset();
    };

    void start(timestamp_t t)
    {
        assert(!last_start.has_value());
        last_start = t;
    };

    clock_stats_t get_stats_at(timestamp_t t) const
    {
        auto stats = this->stats;

        if (last_start) {
            stats += count(t - *last_start);
        };

        return stats;
    };

    void serialize(std::ostream& stream) const override
    {
        stream.write(reinterpret_cast<const char*>(&dataRate), sizeof(dataRate));
        bool hasLastStart = last_start.has_value();
        stream.write(reinterpret_cast<const char*>(&hasLastStart), sizeof(hasLastStart));
        if (hasLastStart) {
            stream.write(reinterpret_cast<const char*>(&last_start), sizeof(last_start));
        }
        stats.serialize(stream);
    };

    void deserialize(std::istream& stream) override
    {
        stream.read(reinterpret_cast<char*>(&dataRate), sizeof(dataRate));
        bool hasLastStart = false;
        stream.read(reinterpret_cast<char*>(&hasLastStart), sizeof(hasLastStart));
        if (hasLastStart) {
            last_start = timestamp_t();
            stream.read(reinterpret_cast<char*>(&last_start), sizeof(last_start));
        } else {
            last_start.reset();
        }
        stats.deserialize(stream);
    };
};

};

#endif /* DRAMPOWER_UTIL_CLOCK_H */
