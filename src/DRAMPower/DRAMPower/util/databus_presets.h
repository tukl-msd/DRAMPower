#ifndef DRAMPOWER_UTIL_DATABUS_PRESETS
#define DRAMPOWER_UTIL_DATABUS_PRESETS

#include <optional>

#include <DRAMPower/util/databus.h>
#include <DRAMUtils/util/types.h>

namespace DRAMPower::util::databus_presets {

    template <typename... Extensions>
    using databus_64_t = DataBus<64, Extensions...>;
    template <typename... Extensions>
    using databus_256_t = DataBus<256, Extensions...>;
    template <typename... Extensions>
    using databus_1024_t = DataBus<1024, Extensions...>;
    template <typename... Extensions>
    using databus_4096_t = DataBus<4096, Extensions...>;

    template <typename... Extensions>
    using databus_preset_sequence_t = DRAMUtils::util::type_sequence<
        databus_64_t<Extensions...>,
        databus_256_t<Extensions...>,
        databus_1024_t<Extensions...>,
        databus_4096_t<Extensions...>
    >;

    template <typename... Extensions>
    using databus_preset_t = util::DataBusContainerProxy<databus_preset_sequence_t<Extensions...>>;

    template <typename... Extensions>
    inline databus_preset_t<Extensions...> getDataBusPreset(
        const size_t width,
        DataBusConfig&& busConfig
    ) {
        if (width <= 64) {
            return databus_64_t<Extensions...>(std::move(busConfig));
        } else if (width <= 256) {
            return databus_256_t<Extensions...>(std::move(busConfig));
        } else if (width <= 1024) {
            return databus_1024_t<Extensions...>(std::move(busConfig));
        } else if (width > 4096) {
            assert(false);
            throw std::runtime_error("Data bus width exceeds maximum.");
        }
        return databus_4096_t<Extensions...>(std::move(busConfig));
    }

} // namespace DRAMPower::util::databus_presets

#endif /* DRAMPOWER_UTIL_DATABUS_PRESETS */
