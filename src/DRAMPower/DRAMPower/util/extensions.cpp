#include <DRAMPower/util/extensions.h>
#include <DRAMPower/command/Command.h>

namespace DRAMPower::extensions {

using namespace DRAMPower::util;

bool DBI::enable(timestamp_t timestamp, bool enable) {
    bool result = true;
    // Dispatch callback if set
    if (m_callback) {
        result = (*m_callback)(timestamp, enable);
    }
    if (result) {
        // Update member
        m_enabled = enable;
    }
    return result;
}

bool DBI::isEnabled() const {
    return m_enabled;
}

void DBI::serialize(std::ostream& stream) const {
    stream.write(reinterpret_cast<const char*>(&m_enabled), sizeof(m_enabled));
}
void DBI::deserialize(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&m_enabled), sizeof(m_enabled));
}

} // namespace DRAMPower::extensions