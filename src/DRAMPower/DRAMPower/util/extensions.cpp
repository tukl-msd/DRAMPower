#include <DRAMPower/util/extensions.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/util/bus_extensions.h>

namespace DRAMPower::extensions {

using namespace DRAMPower::util;

DBI::DBI(databus_preset_t& dataBus, std::optional<interface_callback_t>&& callback)
: m_dataBus(dataBus) 
, m_callback(std::move(callback))
{
    static_assert(dataBus.hasExtension<bus_extensions::BusExtensionDBI>(),
        "DRAMPowerExtensionDBI requires databus with BusExtensionDBI");
    if constexpr (!dataBus.hasExtension<bus_extensions::BusExtensionDBI>()) {
        throw std::runtime_error("DRAMPowerExtensionDBI requires databus with BusExtensionDBI");
    }
    // Ensure Extension and databus have the same state
    bool enable_write = m_dataBus.withExtensionWrite<bus_extensions::BusExtensionDBI>([&enable_write](const auto& ext) {
        return ext.isEnabled();
    });
    bool enable_read = m_dataBus.withExtensionRead<bus_extensions::BusExtensionDBI>([&enable_read](const auto& ext) {
        return ext.isEnabled();
    });
    assert(enable_write == enable_read);
    m_enabled = enable_write;
}

void DBI::enable(timestamp_t timestamp, bool enable) {
    // Update bus
    auto callback = [enable](auto& ext) {
        ext.enable(enable);
    };
    m_dataBus.withExtensionWrite<bus_extensions::BusExtensionDBI>(callback);
    m_dataBus.withExtensionRead<bus_extensions::BusExtensionDBI>(callback);
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

} // namespace DRAMPower::extensions