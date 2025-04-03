#ifndef DRAMPOWER_UTIL_DATABUS_EXTENSIONS
#define DRAMPOWER_UTIL_DATABUS_EXTENSIONS

#include <cstddef>
#include <vector>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <functional>

namespace DRAMPower::util::databus_extensions {

enum class DataBusHook : uint64_t {
    None        = 0,
    onLoad      = 1 << 0
};
constexpr DataBusHook operator|(DataBusHook lhs, DataBusHook rhs) {
    return static_cast<DataBusHook>(static_cast<std::underlying_type_t<DataBusHook>>(lhs) |
        static_cast<std::underlying_type_t<DataBusHook>>(rhs));
}
constexpr DataBusHook operator&(DataBusHook lhs, DataBusHook rhs) {
    return static_cast<DataBusHook>(static_cast<std::underlying_type_t<DataBusHook>>(lhs) &
        static_cast<std::underlying_type_t<DataBusHook>>(rhs));
}
constexpr bool operator!=(DataBusHook lhs, size_t rhs) {
    return static_cast<std::underlying_type_t<DataBusHook>>(lhs) != rhs;
}

template <typename Parent>
class DataBusExtensionDBI {
public:
    using StateChangeCallback_t = std::function<void(bool)>;


public:
    explicit DataBusExtensionDBI(Parent *parent) : m_parent(parent) {}

    // supported hooks
    static constexpr DataBusHook getSupportedHooks() {
        return DataBusHook::onLoad;
    }

    void setState(timestamp_t timestamp, bool state) {
        // Case 1: timestamp < m_newTimestamp
        // Is not supported in DRAMPower. The timestamp is always increasing.
        // Case 2: timestamp == m_newTimestamp
        // just update the m_newState
        // Case 3: timestamp > m_newTimestamp
        // update the m_newState and m_newTimestamp
        if (!m_pendingState || (m_pendingState && timestamp <= m_newTimestamp)) {
            m_newState = state;
            m_newTimestamp = timestamp;
            m_pendingState = true;
        }
        if (m_stateChangeCallback) {
            m_stateChangeCallback(state);
        }
    }

    void setStateChangeCallback(StateChangeCallback_t&& callback) {
        m_stateChangeCallback = std::move(callback);
    }

    bool getState() const {
        return m_currentState;
    }

// Hook functions
    void onLoad(timestamp_t timestamp, std::size_t n_bits, const uint8_t *datain, uint8_t **dataout) {
        if (!datain || n_bits == 0) return;

        if (m_pendingState && timestamp >= m_newTimestamp) {
            m_currentState = m_newState;
            m_pendingState = false;
        }

        if (!m_currentState) return;

        size_t byteCount = (n_bits + 7) / 8;
        invertedData.resize(byteCount);

        for (std::size_t i = 0; i < byteCount; ++i) {
            invertedData[i] = ~datain[i];
        }
        *dataout = invertedData.data();
    }
private:
    Parent *m_parent;
    std::vector<uint8_t> invertedData;

    bool m_currentState = false;

    StateChangeCallback_t m_stateChangeCallback = nullptr;

    bool m_newState = false;
    bool m_pendingState = false;
    timestamp_t m_newTimestamp = 0;
};

} // namespace DRAMPower::util::databus_extensions

#endif /* DRAMPOWER_UTIL_DATABUS_EXTENSIONS */
