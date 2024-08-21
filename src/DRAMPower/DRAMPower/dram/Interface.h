#ifndef DRAMPOWER_DDR_INTERFACE_H
#define DRAMPOWER_DDR_INTERFACE_H

#pragma once 

#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus.h>

#include <stdint.h>
#include <optional>


namespace DRAMPower {

enum class TogglingRateIdlePattern
{
    L = 0,
    H = 1,
    Z = 2,
};

struct ToggleRateDefinition
{
    double togglingRateRead;
    double togglingRateWrite;
    double dutyCycleRead;
    double dutyCycleWrite;
    TogglingRateIdlePattern idlePatternRead;
    TogglingRateIdlePattern idlePatternWrite;
};

class TogglingHandle
{

struct TogglingHandleLastBurst {
    uint64_t last_length;
    timestamp_t last_load;
};

private:
    uint64_t width = 0;
    uint64_t datarate = 0;
    double toggling_rate = 0; // [0, 1] allowed
    double duty_cycle = 0.0; // [0, 1] allowed
    std::optional<TogglingHandleLastBurst> last_burst = std::nullopt;
    bool enable = false;
    uint64_t count = 0;
    TogglingRateIdlePattern idlepattern = TogglingRateIdlePattern::Z;

public:
    TogglingHandle(const uint64_t width, const uint64_t datarate, const double toggling_rate, const double duty_cycle, const bool enabled = true)
        : width(width)
        , datarate(datarate)
        , toggling_rate(toggling_rate)
        , duty_cycle(duty_cycle)
        , enable(enabled)
    {
        assert(width > 0); // Check bounds
        assert(datarate > 0); // Check bounds
        assert(duty_cycle >= 0 && duty_cycle <= 1); // Check bounds
        assert(toggling_rate >= 0 && toggling_rate <= 1); // Check bounds
    }

    TogglingHandle() = default;
public:
// Getters and Setters
    void disable()
    {
        this->enable = false;
    }
    bool isEnabled() const
    {
        return this->enable;
    }
    double getTogglingRate() const
    {
        return this->toggling_rate;
    }
    double getDutyCycle() const
    {
        return this->duty_cycle;
    }
    uint64_t getWidth() const
    {
        return this->width;
    }
    void setWidth(const uint64_t width)
    {
        this->width = width;
    }
    uint64_t getDatarate() const
    {
        return this->datarate;
    }
    void setDataRate(const uint64_t datarate)
    {
        this->datarate = datarate;
    }
    void setTogglingRateAndDutyCycle(const double toggling_rate, const double duty_cycle, const TogglingRateIdlePattern idlepattern)
    {
        this->toggling_rate = toggling_rate;
        this->duty_cycle = duty_cycle;
        this->idlepattern = idlepattern;
        this->enable = true;
    }
    uint64_t getCount() const
    {
        return this->count;
    }
public:
    void incCountBurstLength(timestamp_t timestamp, uint64_t burstlength)
    {
        // Convert to bus timings
        timestamp_t virtual_timestamp = timestamp * this->datarate;
        assert(virtual_timestamp / this->datarate == timestamp); // No overflow
        assert(
            (this->last_burst && (virtual_timestamp >= (this->last_burst->last_length + this->last_burst->last_load)))
            || !this->last_burst
        );
        // Add last burst
        if (this->last_burst) {
            this->count += this->last_burst->last_length;
        }
        // Store burst in last_length and last_load if enabled
        if (this->enable) {
            // Set last_length and last_load to new burst_length
            this->last_burst = TogglingHandleLastBurst {
                burstlength, // last_length
                virtual_timestamp,   // last_load
            };
        } else {
            // Clear last_length and loast_load
            this->last_burst = std::nullopt;
        }
    }
    void incCountBitLength(timestamp_t timestamp, uint64_t bitlength)
    {
        assert(bitlength % this->width == 0);
        this->incCountBurstLength(timestamp, bitlength / this->width);
    }
    util::bus_stats_t get_stats(timestamp_t timestamp)
    {
        // Convert to bus timings
        timestamp_t virtual_timestamp = timestamp * this->datarate;
        assert(virtual_timestamp / this->datarate == timestamp); // No overflow

        util::bus_stats_t stats;
        // Check if last burst is finished
        if ((this->last_burst)
            && (virtual_timestamp < this->last_burst->last_length + this->last_burst->last_load)
        ) {
            // last burst not finished
            this->count += virtual_timestamp - this->last_burst->last_load;
        } else if (this->last_burst) {
            // last burst finished
            this->count += this->last_burst->last_length;
        }
        // Compute toggles
        stats.ones = this->count * this->duty_cycle;
        stats.zeroes = this->count * (1 - this->duty_cycle);
        stats.ones_to_zeroes = this->count * this->toggling_rate / 2; // Round down
        // Compute idle
        switch (this->idlepattern) {
            case TogglingRateIdlePattern::L:
                stats.zeroes += virtual_timestamp - this->count;
                break;
            case TogglingRateIdlePattern::H:
                stats.ones += virtual_timestamp - this->count;
                break;
            case TogglingRateIdlePattern::Z:
                // Nothing to do in high impedance mode
                break;
        }
        // Scale by bus width
        stats.ones *= this->width;
        stats.zeroes *= this->width;
        stats.ones_to_zeroes *= this->width;
        // Compute zeroes_to_ones and bit changes
        stats.zeroes_to_ones = stats.ones_to_zeroes;
        stats.bit_changes = stats.ones_to_zeroes + stats.zeroes_to_ones;
        return stats;
    }

};

struct interface_stats_t 
{
	util::bus_stats_t command_bus;
	util::bus_stats_t read_bus;
	util::bus_stats_t write_bus;
};

}

#endif /* DRAMPOWER_DDR_INTERFACE_H */
