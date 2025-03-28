#include <DRAMPower/util/extensions.h>
#include <DRAMPower/command/Command.h>

namespace DRAMPower::extensions {

DRAMPowerExtensionDBI::DRAMPowerExtensionDBI(DRAMPower::util::databus_presets::databus_preset_t& dataBus)
: m_dataBus(dataBus) 
{
    m_dataBus.setDataBusInversion(m_dbi);
}

void DRAMPowerExtensionDBI::set(bool dbi) {
    m_dbi = dbi;
    m_dataBus.setDataBusInversion(dbi);
}
bool DRAMPowerExtensionDBI::get() const {
    return m_dbi;
}

} // namespace DRAMPower::extensions