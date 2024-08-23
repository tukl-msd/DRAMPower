#ifndef DRAMPOWER_DDR_INTERFACE_H
#define DRAMPOWER_DDR_INTERFACE_H

#pragma once 

#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus.h>

#include <DRAMUtils/util/json.h>
#include <DRAMUtils/util/json_utils.h>

#include <stdint.h>
#include <optional>


namespace DRAMPower {

enum class TogglingRateIdlePattern
{
    L = 0,
    H = 1,
    Z = 2,
    Invalid = -1
};
NLOHMANN_JSON_SERIALIZE_ENUM(TogglingRateIdlePattern,
                             {{TogglingRateIdlePattern::Invalid, nullptr},
                              {TogglingRateIdlePattern::L, "L"},
                              {TogglingRateIdlePattern::H, "H"},
                              {TogglingRateIdlePattern::Z, "Z"}})

struct ToggleRateDefinition
{
    double togglingRateRead;
    double togglingRateWrite;
    double dutyCycleRead;
    double dutyCycleWrite;
    TogglingRateIdlePattern idlePatternRead;
    TogglingRateIdlePattern idlePatternWrite;
};
NLOHMANN_JSONIFY_ALL_THINGS(ToggleRateDefinition, togglingRateRead, togglingRateWrite, dutyCycleRead, dutyCycleWrite, idlePatternRead, idlePatternWrite)

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
    TogglingHandle(const uint64_t width, const uint64_t datarate, const double toggling_rate, const double duty_cycle, const bool enabled = true);

    TogglingHandle() = default;
public:
// Getters and Setters
    double getTogglingRate() const;
    double getDutyCycle() const;
    bool isEnabled() const;
    uint64_t getWidth() const;
    uint64_t getDatarate() const;
    
    void setTogglingRateAndDutyCycle(const double toggling_rate, const double duty_cycle, const TogglingRateIdlePattern idlepattern);
    void disable();
    void setWidth(const uint64_t width);
    void setDataRate(const uint64_t datarate);
    uint64_t getCount() const;
public:
    void incCountBurstLength(timestamp_t timestamp, uint64_t burstlength);
    void incCountBitLength(timestamp_t timestamp, uint64_t bitlength);
    util::bus_stats_t get_stats(timestamp_t timestamp);

};

struct interface_stats_t 
{
    util::bus_stats_t command_bus;
    util::bus_stats_t read_bus;
    util::bus_stats_t write_bus;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_DDR_INTERFACE_H */
