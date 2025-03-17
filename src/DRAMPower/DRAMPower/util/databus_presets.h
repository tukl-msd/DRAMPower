#ifndef DRAMPOWER_UTIL_DATABUS_PRESETS
#define DRAMPOWER_UTIL_DATABUS_PRESETS

#include <DRAMPower/util/databus.h>
#include <optional>

namespace DRAMPower::util {

    using databus_64_t = DataBus<0, 64>;
    using databus_256_t = DataBus<0, 256>;
    using databus_1024_t = DataBus<0, 1024>;
    using databus_4096_t = DataBus<0, 4096>;

    using databus_preset_fallback_t = DataBus<64>;

    using databus_preset_sequence_t = DRAMUtils::util::type_sequence<
        util::databus_64_t,
        util::databus_256_t,
        util::databus_1024_t,
        util::databus_4096_t
    >;

    template <typename DataBus, typename Builder>
    static bool getDataBusPreset(
        const size_t width,
        DataBus &dataBus,
        Builder &builder
    ) {
        if (width <= 64) {
            dataBus = builder.template build<databus_64_t>();
            return true;
        } else if (width <= 256) {
            dataBus = builder.template build<databus_256_t>();
            return true;
        } else if (width <= 1024) {
            dataBus = builder.template build<databus_1024_t>();
            return true;
        } else if (width <= 4096) {
            dataBus = builder.template build<databus_4096_t>();
            return true;
        } 
        return false;
    }

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_DATABUS_PRESETS */
