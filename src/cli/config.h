#ifndef CLI_CONFIG_H
#define CLI_CONFIG_H

#include <optional>

#include <DRAMUtils/util/json.h>
#include <DRAMUtils/util/json_utils.h>

#include <DRAMPower/dram/Interface.h>

struct CLIConfig {
    bool useToggleRate;
    std::optional<DRAMPower::ToggleRateDefinition> toggleRateConfig;
};

NLOHMANN_JSONIFY_ALL_THINGS(CLIConfig, useToggleRate, toggleRateConfig)

#endif /* CLI_CONFIG_H */
