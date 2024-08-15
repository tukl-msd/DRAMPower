#ifndef CLI_CONFIG_H
#define CLI_CONFIG_H

#include <optional>

#include <DRAMUtils/util/json.h>
#include <DRAMUtils/util/json_utils.h>

struct ToggleRateConfig {
    double TogglingRateRead;
    double TogglingRateWrite;
    double DutyCycleRead;
    double DutyCycleWrite;
};
NLOHMANN_JSONIFY_ALL_THINGS(ToggleRateConfig, TogglingRateRead, TogglingRateWrite, DutyCycleRead, DutyCycleWrite)

struct CLIConfig {
    bool useToggleRate;
    std::optional<ToggleRateConfig> toggleRateConfig;
};

NLOHMANN_JSONIFY_ALL_THINGS(CLIConfig, useToggleRate, toggleRateConfig)

#endif /* CLI_CONFIG_H */
