#ifndef DRAMPOWER_DDR_INTERFACE_H
#define DRAMPOWER_DDR_INTERFACE_H

#pragma once 

#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus.h>

#include <stdint.h>
#include <stdint.h>

namespace DRAMPower {

struct ToggleRateDefinition
{
    double togglingRateRead;
    double togglingRateWrite;
    double dutyCycleRead;
    double dutyCycleWrite;
};

class TogglingHandle
{

private:
    double toggling_rate = 0; // 0 to 1 allowed
    double duty_cycle = 0.0; // 0 to 1 allowed
    bool enable = false;
    uint64_t count = 0;

public:
    TogglingHandle(const double toggling_rate, const double duty_cycle)
        : toggling_rate(toggling_rate)
        , duty_cycle(duty_cycle)
        , enable(true)
    {
        assert(duty_cycle >= 0 && duty_cycle <= 1);
        assert(toggling_rate >= 0); // TODO upper bound
    }
    TogglingHandle(const std::optional<double> toggling_rate, const std::optional<double> duty_cycle)
        : toggling_rate(toggling_rate.value_or(0))
        , duty_cycle(duty_cycle.value_or(0))
        , enable(toggling_rate.has_value() && duty_cycle.has_value())
    {
        if (duty_cycle.has_value())
            assert(duty_cycle.value() >= 0 && duty_cycle.value() <= 1);
        if (toggling_rate.has_value())
            assert(toggling_rate.value() >= 0); // TODO upper bound
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
    void setTogglingRateAndDutyCycle(const double toggling_rate, const double duty_cycle)
    {
        this->toggling_rate = toggling_rate;
        this->duty_cycle = duty_cycle;
        this->enable = true;
    }
    uint64_t getCount() const
    {
        return this->count;
    }
public:
    // Only increments count if enabled
    void incCount(uint64_t count)
    {
        if (this->enable)
            this->count += count;
    }
    util::bus_stats_t get_stats(timestamp_t)
    {
        // TODO if EOS in burst the whole burst is calculated
        util::bus_stats_t stats;
        stats.ones = this->count * this->duty_cycle;
        stats.zeroes = this->count * (1 - this->duty_cycle);
        stats.ones_to_zeroes = this->count * this->toggling_rate / 2; // Round down
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
