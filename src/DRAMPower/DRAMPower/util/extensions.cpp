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
    m_dbi = m_dataBus.withExtension<databus_extensions::DataBusExtensionDBI>([this](const auto& ext) {
        return ext.getState();
    });
}

void DRAMPowerExtensionDBI::set(timestamp_t timestamp, bool dbi) {
    m_dataBus.withExtension<databus_extensions::DataBusExtensionDBI>([timestamp, dbi](auto& ext) {
        ext.setState(timestamp, dbi);
    });
    if (m_callback) {
        (*m_callback)(timestamp, m_dbi, dbi);
    }
    m_dbi = dbi;
}
bool DRAMPowerExtensionDBI::get() const {
    return m_dbi;
}

} // namespace DRAMPower::extensions