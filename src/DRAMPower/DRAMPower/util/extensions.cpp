#include <DRAMPower/util/extensions.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/util/databus_extensions.h>

namespace DRAMPower::extensions {

using namespace DRAMPower::util;

DRAMPowerExtensionDBI::DRAMPowerExtensionDBI(databus_preset_t& dataBus, std::optional<interface_callback_t>&& callback)
: m_dataBus(dataBus) 
, m_callback(std::move(callback))
{
    static_assert(dataBus.hasExtension<databus_extensions::DataBusExtensionDBI>(),
        "DRAMPowerExtensionDBI requires databus with DataBusExtensionDBI");
    if constexpr (!dataBus.hasExtension<databus_extensions::DataBusExtensionDBI>()) {
        throw std::runtime_error("DRAMPowerExtensionDBI requires databus with DataBusExtensionDBI");
    }
    // Ensure Extension and databus have the same state
    m_enabled = m_dataBus.withExtension<databus_extensions::DataBusExtensionDBI>([this](const auto& ext) {
        return ext.isEnabled();
    });
}

void DRAMPowerExtensionDBI::enable(timestamp_t timestamp, bool enable) {
    m_dataBus.withExtension<databus_extensions::DataBusExtensionDBI>([enable](auto& ext) {
        ext.enable(enable);
    });
    // Dispatch callback if set
    if (m_callback) {
        (*m_callback)(timestamp, enable);
    }
    // Update member
    m_enabled = enable;
}
bool DRAMPowerExtensionDBI::isEnabled() const {
    return m_enabled;
}

} // namespace DRAMPower::extensions