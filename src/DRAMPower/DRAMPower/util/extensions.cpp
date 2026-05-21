#include <DRAMPower/util/extensions.h>
#include <DRAMPower/command/Command.h>
#include <cstdint>
#include <tuple>

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

void MetaData::setMetaData(uint16_t metaData1, uint16_t metaData2) {
    if (m_set_callback) {
        (m_set_callback)(metaData1, metaData2);
        m_metadata = std::make_tuple(metaData1, metaData2);
    }
}

std::tuple<uint16_t, uint16_t> MetaData::getMetaData() const {
    if (m_get_callback) {
        return (m_get_callback)();
    }
    return m_metadata;
}

void MetaData::serialize(std::ostream& stream) const {
    auto[m1, m2] = m_metadata;
    stream.write(reinterpret_cast<const char*>(&m1), sizeof(m1));
    stream.write(reinterpret_cast<const char*>(&m2), sizeof(m2));
}
void MetaData::deserialize(std::istream& stream) {
    uint16_t m1 = 0;
    uint16_t m2 = 0;
    stream.read(reinterpret_cast<char*>(&m1), sizeof(m1));
    stream.read(reinterpret_cast<char*>(&m2), sizeof(m2));
    m_metadata = std::make_tuple(m1, m2);
}

} // namespace DRAMPower::extensions