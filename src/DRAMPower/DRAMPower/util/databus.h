#ifndef DRAMPOWER_UTIL_DATABUS
#define DRAMPOWER_UTIL_DATABUS

#include <cstddef>

#include <DRAMPower/util/bus.h>
#include <DRAMPower/dram/Interface.h>

#include <DRAMUtils/config/toggling_rate.h>

namespace DRAMPower::util {

template<std::size_t blocksize = 32>
class DataBus {

public:
    using Bus_t = util::Bus<blocksize>;
    using IdlePattern = typename Bus_t::BusIdlePatternSpec;
    using InitPattern = typename Bus_t::BusInitPatternSpec;

    enum class BusType {
        Bus = 0,
        TogglingRate
    };

public:
    DataBus(std::size_t numberOfDevices, std::size_t width, std::size_t dataRate,
        IdlePattern idlePattern, InitPattern initPattern,
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
        , dataRate(dataRate)
        , width(width)
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
    void load(Bus_t &bus, TogglingHandle &togglingHandle, timestamp_t timestamp, std::size_t n_bits, const uint8_t *data = nullptr) {
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
    void loadWrite(timestamp_t timestamp, std::size_t n_bits, const uint8_t *data = nullptr) {
        load(busWrite, togglingHandleWrite, timestamp, n_bits, data);
    }
    void loadRead(timestamp_t timestamp, std::size_t n_bits, const uint8_t *data = nullptr) {
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

    void setTogglingRateDefinition(DRAMUtils::Config::ToggleRateDefinition toggleratedefinition) {
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

    std::size_t getWidth() {
        return width;
    }

    std::size_t getNumberOfDevices() {
        return numberOfDevices;
    }

    std::size_t getCombinedBusWidth() {
        return width * numberOfDevices;
    }

    std::size_t getDataRate() {
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
    Bus_t busRead;
    Bus_t busWrite;
    TogglingHandle togglingHandleRead;
    TogglingHandle togglingHandleWrite;
    BusType busType;
    std::size_t numberOfDevices;
    std::size_t dataRate;
    std::size_t width;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_UTIL_DATABUS */
