#ifndef CLI_CONFIG_H
#define CLI_CONFIG_H

#include <DRAMUtils/util/json.h>
#include <DRAMUtils/util/json_utils.h>

#include <DRAMPower/simconfig/simconfig.h>

namespace DRAMPower::DRAMPowerCLI::config {

struct CLIConfig {
    DRAMPower::config::SimConfig simconfig;
};

NLOHMANN_JSONIFY_ALL_THINGS(CLIConfig, simconfig)

} // namespace DRAMPower::DRAMPowerCLI::config


#endif /* CLI_CONFIG_H */
