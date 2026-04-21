#ifndef DRAMPOWER_UTIL_DATABUS_TYPES
#define DRAMPOWER_UTIL_DATABUS_TYPES

#include <cstddef>

#include <DRAMPower/util/bus_types.h>

#include <DRAMUtils/config/toggling_rate.h>


namespace DRAMPower::util {

enum class DataBusMode {
    Bus = 0,
    TogglingRate
};

struct DataBusConfig {
    std::size_t width;
    std::size_t dataRate;
    DRAMUtils::Config::ToggleRateDefinition toggleRateConf;
};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_DATABUS_TYPES */
