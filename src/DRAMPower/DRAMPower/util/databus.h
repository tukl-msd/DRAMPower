#ifndef DRAMPOWER_UTIL_DATABUS
#define DRAMPOWER_UTIL_DATABUS

#include <cstddef>

#include <DRAMPower/util/bus.h>
#include <DRAMPower/dram/Interface.h>

#include <DRAMUtils/config/toggling_rate.h>

namespace DRAMPower::util {

class DataBus {

public:
    enum class BusType {
        Bus = 0,
        TogglingRate
    };

public:
    DataBus(size_t numberOfDevices, size_t width, size_t dataRate,
            util::Bus::BusIdlePatternSpec idlePattern, util::Bus::BusInitPatternSpec initPattern,
            DRAMUtils::Config::TogglingRateIdlePattern togglingRateIdlePattern = DRAMUtils::Config::TogglingRateIdlePattern::Z,
            const double togglingRate = 0.0, const double dutyCycle = 0.0,
            BusType busType = BusType::Bus
        )
        : busRead(width * numberOfDevices, dataRate, idlePattern, initPattern)
        , busWrite(width * numberOfDevices, dataRate, idlePattern, initPattern)
        , togglingHandleRead(width * numberOfDevices, dataRate, togglingRate, dutyCycle, togglingRateIdlePattern, false)
        , togglingHandleWrite(width * numberOfDevices, dataRate, togglingRate, dutyCycle, togglingRateIdlePattern, false)
        , busType(busType)
        , numberOfDevices(numberOfDevices)
        , width(width)
        , dataRate(dataRate)
    {
        switch(busType) {
            case BusType::Bus:
                busWrite.enable(0);
                busRead.enable(0);
                break;
            case BusType::TogglingRate:
                togglingHandleRead.enable(0);
                togglingHandleWrite.enable(0);
                break;
        }
    }

private:
    void load(util::Bus &bus, TogglingHandle &togglingHandle, timestamp_t timestamp, size_t n_bits, const uint8_t *data = nullptr) {
        switch(busType) {
            case BusType::Bus:
                if (nullptr == data || 0 == n_bits) {
                    // No data to load, skip burst
                    return;
                }
                bus.load(timestamp, data, n_bits);
                break;
            case BusType::TogglingRate:
                togglingHandle.incCountBitLength(timestamp, n_bits);
                break;
        }
    }

public:
    void loadWrite(timestamp_t timestamp, size_t n_bits, const uint8_t *data = nullptr) {
        load(busWrite, togglingHandleWrite, timestamp, n_bits, data);
    }
    void loadRead(timestamp_t timestamp, size_t n_bits, const uint8_t *data = nullptr) {
        load(busRead, togglingHandleRead, timestamp, n_bits, data);
    }

    void enableBus(timestamp_t timestamp) {
        busRead.enable(timestamp);
        busWrite.enable(timestamp);
        togglingHandleRead.disable(timestamp);
        togglingHandleWrite.disable(timestamp);
        busType = BusType::Bus;
    }

    void enableTogglingRate(timestamp_t timestamp) {
        busRead.disable(timestamp);
        busWrite.disable(timestamp);
        togglingHandleRead.enable(timestamp);
        togglingHandleWrite.enable(timestamp);
        busType = BusType::TogglingRate;
    }

    void setTogglingRateAndDutyCycle(DRAMUtils::Config::ToggleRateDefinition toggleratedefinition) {
        togglingHandleRead.setTogglingRateAndDutyCycle(toggleratedefinition.togglingRateRead, toggleratedefinition.dutyCycleRead, toggleratedefinition.idlePatternRead);
        togglingHandleWrite.setTogglingRateAndDutyCycle(toggleratedefinition.togglingRateWrite, toggleratedefinition.dutyCycleWrite, toggleratedefinition.idlePatternWrite);
    }

    timestamp_t lastBurst() {
        switch(busType) {
            case BusType::Bus:
                return std::max(busWrite.get_lastburst_timestamp(), busRead.get_lastburst_timestamp());
            case BusType::TogglingRate:
                return std::max(togglingHandleRead.get_lastburst_timestamp(), togglingHandleWrite.get_lastburst_timestamp());
        }
        assert(false);
        return 0;
    }

    bool isBus() {
        return BusType::Bus == busType;
    }

    bool isTogglingRate() {
        return BusType::TogglingRate == busType;
    }

    size_t getWidth() {
        return width;
    }

    size_t getDataRate() {
        return dataRate;
    }

    void get_stats(timestamp_t timestamp,
        util::bus_stats_t &busReadStats,
        util::bus_stats_t &busWriteStats,
        util::bus_stats_t &togglingHandleReadStats,
        util::bus_stats_t &togglingHandleWriteStats)
    {
        busReadStats = busRead.get_stats(timestamp);
        busWriteStats = busWrite.get_stats(timestamp);
        togglingHandleReadStats = togglingHandleRead.get_stats(timestamp);
        togglingHandleWriteStats = togglingHandleWrite.get_stats(timestamp);
    }

private:
    util::Bus busRead;
    util::Bus busWrite;
    TogglingHandle togglingHandleRead;
    TogglingHandle togglingHandleWrite;
    BusType busType;
    size_t numberOfDevices;
    size_t width;
    size_t dataRate;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_UTIL_DATABUS */
