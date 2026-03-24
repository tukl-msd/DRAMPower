#ifndef DRAMPOWER_SIMCONFIG_SIMCONFIG_H
#define DRAMPOWER_SIMCONFIG_SIMCONFIG_H

#include "DRAMUtils/util/json_utils.h"
#include "DRAMUtils/config/toggling_rate.h"

namespace DRAMPower::config {

struct SimConfig {
// Public type definitions
public:
    using ToggleRateDefinition_t = DRAMUtils::Config::ToggleRateDefinition;

// Public Constructors
public:
    SimConfig() = default;

// Public members
public:
    std::optional<ToggleRateDefinition_t> toggleRateDefinition;
};
NLOHMANN_JSONIFY_ALL_THINGS(SimConfig, toggleRateDefinition);

} // DRAMPower::config

#endif /* DRAMPOWER_SIMCONFIG_SIMCONFIG_H */
