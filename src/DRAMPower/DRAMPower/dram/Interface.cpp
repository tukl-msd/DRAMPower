#include "Interface.h"

namespace DRAMPower {

using namespace DRAMUtils::Config;

TogglingHandle::TogglingHandle(const uint64_t width, const uint64_t datarate, const double toggling_rate, const double duty_cycle, const bool enabled)
    : width(width)
    , datarate(datarate)
    , toggling_rate(toggling_rate)
    , duty_cycle(duty_cycle)
    , enableflag(enabled)
{
    assert(width > 0); // Check bounds
    assert(datarate > 0); // Check bounds
    assert(duty_cycle >= 0 && duty_cycle <= 1); // Check bounds
    assert(toggling_rate >= 0 && toggling_rate <= 1); // Check bounds
}

void TogglingHandle::disable(timestamp_t timestamp)
{
    if(!this->enableflag) {
        return;
    }
    this->enableflag = false;
    this->disable_timestamp = timestamp * this->datarate;
    if (this->last_burst) {
        if(timestamp * datarate >= this->last_burst->last_length + this->last_burst->last_load) {
            this->count += this->last_burst->last_length;
        } else {
            this->count += timestamp * datarate - this->last_burst->last_load;
        }
        this->last_burst = std::nullopt;
    }
}

void TogglingHandle::enable(timestamp_t timestamp)
{
    if (this->enableflag) {
        return;
    }
    this->disable_time += timestamp * this->datarate - this->disable_timestamp;
    this->enableflag = true;
}

bool TogglingHandle::isEnabled() const
{
    return this->enableflag;
}

double TogglingHandle::getTogglingRate() const
{
    return this->toggling_rate;
}

double TogglingHandle::getDutyCycle() const
{
    return this->duty_cycle;
}
uint64_t TogglingHandle::getWidth() const
{
    return this->width;
}
void TogglingHandle::setWidth(const uint64_t width)
{
    this->width = width;
}
uint64_t TogglingHandle::getDatarate() const
{
    return this->datarate;
}
void TogglingHandle::setDataRate(const uint64_t datarate)
{
    this->datarate = datarate;
}
void TogglingHandle::setTogglingRateAndDutyCycle(const double toggling_rate, const double duty_cycle, const TogglingRateIdlePattern idlepattern)
{
    this->toggling_rate = toggling_rate;
    this->duty_cycle = duty_cycle;
    this->idlepattern = idlepattern;
}
uint64_t TogglingHandle::getCount() const
{
    return this->count;
}

void TogglingHandle::incCountBurstLength(timestamp_t timestamp, uint64_t burstlength)
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
    if (this->enableflag) {
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
void TogglingHandle::incCountBitLength(timestamp_t timestamp, uint64_t bitlength)
{
    assert(bitlength % this->width == 0);
    this->incCountBurstLength(timestamp, bitlength / this->width);
}
util::bus_stats_t TogglingHandle::get_stats(timestamp_t timestamp)
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
            stats.zeroes += virtual_timestamp - this->disable_time - this->count;
            break;
        case TogglingRateIdlePattern::H:
            stats.ones += virtual_timestamp - this->disable_time - this->count;
            break;
        case TogglingRateIdlePattern::Invalid:
            assert(false); // Fallback to Z
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

} // namespace DRAMPower