#ifndef DRAMPOWER_UTIL_DATABUS_PRESETS
#define DRAMPOWER_UTIL_DATABUS_PRESETS

#include <optional>

#include <DRAMPower/util/databus.h>
#include <DRAMUtils/util/types.h>

namespace DRAMPower::util::databus_presets {

    using databus_64_t = DataBus<0, 64>;
    using databus_256_t = DataBus<0, 256>;
    using databus_1024_t = DataBus<0, 1024>;
    using databus_4096_t = DataBus<0, 4096>;

    using databus_preset_fallback_t = DataBus<64>;


    using databus_preset_sequence_t = DRAMUtils::util::type_sequence<
        databus_64_t,
        databus_256_t,
        databus_1024_t,
        databus_4096_t,
        databus_preset_fallback_t
    >;

    using databus_preset_t = util::DataBusContainerProxy<databus_preset_sequence_t>;

    template <typename Builder>
    static databus_preset_t getDataBusPreset(
        const size_t width,
        Builder&& builder,
        bool printWarning = false
    ) {
        if (width <= 64) {
            return builder.template build<databus_64_t>();
        } else if (width <= 256) {
            return builder.template build<databus_256_t>();
        } else if (width <= 1024) {
            return builder.template build<databus_1024_t>();
        } else if (width <= 4096) {
            return builder.template build<databus_4096_t>();
        }
        if (printWarning) {
            std::cerr << "[Warning] Data bus width " << width << " exceeds maximum. Using fallback with dynamic bitset. "
                      << "This may lead to performance issues." << std::endl;
        }
        return builder.template build<databus_preset_fallback_t>();
    }

} // namespace DRAMPower::util::databus_presets

#endif /* DRAMPOWER_UTIL_DATABUS_PRESETS */
