#ifndef DRAMPOWER_DDR_INTERFACE_H
#define DRAMPOWER_DDR_INTERFACE_H

#pragma once 

#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus.h>
#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

#include <DRAMUtils/config/toggling_rate.h>

#include <stdint.h>
#include <optional>


namespace DRAMPower {

class TogglingHandle : public util::Serialize, public util::Deserialize
{

struct TogglingHandleLastBurst  : public util::Serialize, public util::Deserialize {
    uint64_t last_length = 0;
    timestamp_t last_load = 0;
    bool handled = true;
    TogglingHandleLastBurst() = default;
    TogglingHandleLastBurst(uint64_t last_length, timestamp_t last_load, bool handled)
        : last_length(last_length), last_load(last_load), handled(handled) {}
    operator bool() const { return !this->handled; }
    void serialize(std::ostream &stream) const override;
    void deserialize(std::istream &stream) override;
};

private:
    uint64_t width = 0;
    uint64_t datarate = 1;
    double toggling_rate = 0; // [0, 1] allowed
    double duty_cycle = 0.0; // [0, 1] allowed
    TogglingHandleLastBurst last_burst;
    bool enableflag = false; // default disabled if default constructor is used
    uint64_t count = 0;
    timestamp_t disable_timestamp = 0;
    timestamp_t disable_time = 0;
    DRAMUtils::Config::TogglingRateIdlePattern idlepattern = DRAMUtils::Config::TogglingRateIdlePattern::Z;

public:
TogglingHandle(const uint64_t width, const uint64_t datarate, const double toggling_rate, const double duty_cycle, const bool enabled = true);
TogglingHandle(const uint64_t width, const uint64_t datarate, const double toggling_rate, const double duty_cycle, DRAMUtils::Config::TogglingRateIdlePattern idlepattern, const bool enabled = true);

    TogglingHandle() = default;
public:
// Getters and Setters
    double getTogglingRate() const;
    double getDutyCycle() const;
    bool isEnabled() const;
    uint64_t getWidth() const;
    uint64_t getDatarate() const;
    timestamp_t get_lastburst_timestamp(bool relative_to_clock = true) const;
    
    void setTogglingRateAndDutyCycle(const double toggling_rate, const double duty_cycle, const DRAMUtils::Config::TogglingRateIdlePattern idlepattern);
    void disable(timestamp_t timestamp);
    void enable(timestamp_t timestamp);
    void setWidth(const uint64_t width);
    void setDataRate(const uint64_t datarate);
    uint64_t getCount() const;
public:
    void incCountBurstLength(timestamp_t timestamp, uint64_t burstlength);
    void incCountBitLength(timestamp_t timestamp, uint64_t bitlength);
    util::bus_stats_t get_stats(timestamp_t timestamp) const;

// Overrides
    void serialize(std::ostream &stream) const override;
    void deserialize(std::istream &stream) override;

};

struct interface_stats_t 
{
    util::bus_stats_t command_bus;
    util::bus_stats_t read_bus;
    util::bus_stats_t write_bus;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_DDR_INTERFACE_H */
