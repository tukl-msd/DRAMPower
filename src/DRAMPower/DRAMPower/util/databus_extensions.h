#ifndef DRAMPOWER_UTIL_DATABUS_EXTENSIONS
#define DRAMPOWER_UTIL_DATABUS_EXTENSIONS

#include <cstddef>
#include <vector>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace DRAMPower::util::databus_extensions {

enum class DataBusHook : uint64_t {
    None        = 0,
    OnLoad      = 1 << 0
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
    explicit DataBusExtensionDBI(Parent *parent) : parent(parent) {}

    // supported hooks
    static constexpr DataBusHook getSupportedHooks() {
        return DataBusHook::OnLoad;
    }

    void setState(bool state) {
        this->state = state;
    }

// Hook functions
    void onLoad(timestamp_t, std::size_t n_bits, const uint8_t *datain, uint8_t **dataout) {
        if (!datain || n_bits == 0) return;
        if (!state) return;

        size_t byteCount = (n_bits + 7) / 8;
        invertedData.resize(byteCount);

        for (std::size_t i = 0; i < byteCount; ++i) {
            invertedData[i] = ~datain[i];
        }
        *dataout = invertedData.data();
    }
private:
    Parent *parent;
    bool state = false;
    std::vector<uint8_t> invertedData;
};

} // namespace DRAMPower::util::databus_extensions

#endif /* DRAMPOWER_UTIL_DATABUS_EXTENSIONS */
