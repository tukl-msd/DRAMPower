#include <DRAMPower/util/extensions.h>
#include <DRAMPower/command/Command.h>
#include <cstdint>

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

void MetaData::setMetaData(uint16_t metaData) {
    if (m_set_callback) {
        (m_set_callback)(metaData);
        m_metadata = metaData;
    }
}

uint16_t MetaData::getMetaData() const {
    if (m_get_callback) {
        return (m_get_callback)();
    }
    return m_metadata;
}

void MetaData::serialize(std::ostream& stream) const {
    stream.write(reinterpret_cast<const char*>(&m_metadata), sizeof(m_metadata));
}
void MetaData::deserialize(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&m_metadata), sizeof(m_metadata));
}

} // namespace DRAMPower::extensions