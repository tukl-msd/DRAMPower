#include <DRAMPower/util/extensions.h>
#include <DRAMPower/command/Command.h>

namespace DRAMPower::extensions {

using namespace DRAMPower::util;

void DBI::enable(timestamp_t timestamp, bool enable) {
    // Dispatch callback if set
    if (m_callback) {
        (*m_callback)(timestamp, enable);
    }
    // Update member
    m_enabled = enable;
}

bool DBI::isEnabled() const {
    return m_enabled;
}

LPDDR6EfficiencyMode::LPDDR6EfficiencyMode(const bool enabled, interface_callback_t&& callback)
: m_callback(callback)
, m_enabled(enabled)
{}

void LPDDR6EfficiencyMode::enable(timestamp_t timestamp, bool enable) {
    // Dispatch callback
    if (m_callback) {
        m_callback(timestamp, enable);
    }
    // Update member
    m_enabled = enable;
}

bool LPDDR6EfficiencyMode::isEnabled() const {
    return m_enabled;
}

} // namespace DRAMPower::extensions