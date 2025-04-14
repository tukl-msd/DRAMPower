#ifndef DRAMPOWER_UTIL_DATABUS_EXTENSIONS
#define DRAMPOWER_UTIL_DATABUS_EXTENSIONS

#include <cstddef>
#include <vector>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <functional>

#include <DRAMPower/util/databus_types.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus_types.h>
#include <DRAMPower/util/binary_ops.h>

namespace DRAMPower::util::databus_extensions {

enum class DataBusHook : uint64_t {
    None        = 0,
    onInit      = 1 << 0,
    onLoad      = 1 << 1,
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

class DataBusExtensionDBI {
public:
    using IdlePattern_t = util::BusIdlePatternSpec;
    using InvertChangeCallback_t = std::function<void(timestamp_t, bool)>;

public:
    explicit DataBusExtensionDBI() = default;

    // supported hooks
    static constexpr DataBusHook getSupportedHooks() {
        return DataBusHook::onLoad
            | DataBusHook::onInit;
    }

// Public member functions
public:
    void setWidth(std::size_t width) {
        m_width = width;
    }

    void enable(bool enable) {
        m_enable = enable;
    }

    bool isEnabled() const {
        return m_enable;
    }

    void setIdlePattern(IdlePattern_t pattern) {
        m_idlePattern = pattern;
    }

    template <typename Func>
    void setChangeCallback(Func&& callback) {
        m_invertChangeCallback = std::forward<Func>(callback);
    }

// Hook functions
public:
    void onInit(std::size_t width) {
        m_width = width;
        m_lastInvert = false;
    }

    void onLoad(timestamp_t timestamp, util::DataBusMode mode, std::size_t n_bits, const uint8_t *data, bool &invert) {
        // TODO upper and lower dbi signal
        if (!data || n_bits == 0 || util::DataBusMode::Bus != mode || !m_enable) return;
        if (IdlePattern_t::Z == m_idlePattern) return;
        invert = false;

        // Count Transistions and check what is cheaper
        // 1. Invert the data
        // 2. Not invert the data

        uint64_t count_0 = 0;
        uint64_t count_1 = 0;
        size_t n_bytes = (n_bits + 7) / 8;
        for (size_t i = 0; i < n_bytes; ++i) {
            size_t ones_in_byte = util::BinaryOps::popcount(data[i]);
            count_1 += ones_in_byte;
            count_0 += 8 - ones_in_byte;
        }
        // Correct count_0, count_1 if the last byte is not full
        if (n_bits % 8 != 0) {
            uint8_t unused_bits = 8 - (n_bits % 8);
            uint8_t mask = (1u << (n_bits % 8)) - 1;
            uint8_t unused_portion = data[n_bytes - 1] & ~mask;
            size_t ones_in_unused = util::BinaryOps::popcount(unused_portion);
            count_1 -= ones_in_unused;
            count_0 -= unused_bits - ones_in_unused;
        }
        switch(m_idlePattern)
        {
            case IdlePattern_t::L:
                if (count_0 < count_1) {
                    invert = true;
                }
                break;
            case IdlePattern_t::H:
                if (count_1 < count_0) {
                    invert = true;
                }
                break;
            default:
                assert(false);
        }
        // Signal a change in the invert state
        if (nullptr != m_invertChangeCallback && invert != m_lastInvert) {
            m_invertChangeCallback(timestamp, invert);
        }
        m_lastInvert = invert;
    }
private:
    bool m_enable = false;
    bool m_lastInvert = false;
    std::size_t m_width = 0;

    InvertChangeCallback_t m_invertChangeCallback = nullptr;

    IdlePattern_t m_idlePattern = IdlePattern_t::Z;
};

} // namespace DRAMPower::util::databus_extensions

#endif /* DRAMPOWER_UTIL_DATABUS_EXTENSIONS */
