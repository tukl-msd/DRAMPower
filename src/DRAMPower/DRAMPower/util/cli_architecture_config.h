#ifndef DRAMPOWER_UTIL_CLI_ARCHITECTURE_CONFIG_H
#define DRAMPOWER_UTIL_CLI_ARCHITECTURE_CONFIG_H

#include <cstdint>

namespace DRAMPower::util {

struct CLIArchitectureConfig {
    uint64_t deviceCount;
    uint64_t rankCount;
    uint64_t bankCount;
};

};

#endif /* DRAMPOWER_UTIL_CLI_ARCHITECTURE_CONFIG_H */
