#ifndef CLI_CONFIG_H
#define CLI_CONFIG_H

#include <DRAMUtils/util/json.h>
#include <DRAMUtils/util/json_utils.h>

#include <DRAMUtils/config/toggling_rate.h>

namespace DRAMPower::DRAMPowerCLI::config {

struct CLIConfig {
    DRAMUtils::Config::ToggleRateDefinition toggleRateConfig;
};

NLOHMANN_JSONIFY_ALL_THINGS(CLIConfig, toggleRateConfig)

} // namespace DRAMPower::DRAMPowerCLI::config


#endif /* CLI_CONFIG_H */
