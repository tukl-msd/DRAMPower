#ifndef DRAMPOWER_UTIL_DATABUS_EXTENSIONS
#define DRAMPOWER_UTIL_DATABUS_EXTENSIONS

#include <cstddef>
#include <vector>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <functional>

#include <DRAMPower/util/databus_types.h>
#include <DRAMPower/util/bus_types.h>
#include <DRAMPower/util/binary_ops.h>

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
    friend Parent; // Parent can access private members
public:
    using StateChangeCallback_t = std::function<void(bool)>;
    using IdlePattern_t = util::BusIdlePatternSpec;

public:
    explicit DataBusExtensionDBI(Parent *parent) : m_parent(parent) {}

    // supported hooks
    static constexpr DataBusHook getSupportedHooks() {
        return DataBusHook::onLoad;
    }

    void enable(bool enable) {
        m_enable = enable;
    }

    bool isEnabled() const {
        return m_enable;
    }

// Private member functions
private:

    void setIdlePattern(IdlePattern_t pattern) {
        m_idlePattern = pattern;
    }

// Hook functions
    void onLoad(timestamp_t, util::DataBusMode mode, std::size_t n_bits, const uint8_t *data, bool &invert) {
        if (!data || n_bits == 0 || util::DataBusMode::Bus != mode || !m_enable) return;

        // Count Transistions and check what is cheaper
        // 1. Invert the data
        // 2. Not invert the data

        uint64_t count_0 = 0;
        uint64_t count_1 = 0;
        size_t n_bytes = (n_bits + 7) / 8;
        for (size_t i = 0; i < n_bytes; ++i) {
            count_0 += util::BinaryOps::popcount(static_cast<uint64_t>(data[i]));
            count_1 += util::BinaryOps::popcount(static_cast<uint64_t>(~data[i]));
        }
        // Correct count_0, count_1 if the last byte is not full
        if (n_bits % 8 != 0) {
            uint8_t mask = (1 << (n_bits % 8)) - 1;
            count_0 -= util::BinaryOps::popcount(static_cast<uint64_t>(data[n_bytes - 1] & ~mask));
            count_1 -= util::BinaryOps::popcount(static_cast<uint64_t>(~data[n_bytes - 1] & ~mask));
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
            case IdlePattern_t::Z:
                // No action needed
                break;
            default:
                assert(false);
        }
    }
private:
    Parent *m_parent;

    bool m_enable = false;

    IdlePattern_t m_idlePattern = IdlePattern_t::Z;
};

} // namespace DRAMPower::util::databus_extensions

#endif /* DRAMPOWER_UTIL_DATABUS_EXTENSIONS */
