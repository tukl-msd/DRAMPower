#ifndef DRAMPOWER_UTIL_BUS_TYPES
#define DRAMPOWER_UTIL_BUS_TYPES

namespace DRAMPower::util {

enum class BusIdlePatternSpec
{
    L = 0,
    H = 1,
    Z = 2,
    LAST_PATTERN
};

enum class BusInitPatternSpec
{
    L = 0,
    H = 1,
    Z = 2,
};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_BUS_TYPES */
