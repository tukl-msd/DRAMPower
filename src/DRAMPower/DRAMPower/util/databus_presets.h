#ifndef DRAMPOWER_UTIL_DATABUS_PRESETS
#define DRAMPOWER_UTIL_DATABUS_PRESETS

#include <optional>

#include <DRAMPower/util/databus.h>
#include <DRAMPower/util/bus_extensions.h>
#include <DRAMUtils/util/types.h>

namespace DRAMPower::util::databus_presets {

    using databus_64_t = DataBus<64, bus_extensions::BusExtensionDBI>;
    using databus_256_t = DataBus<256, bus_extensions::BusExtensionDBI>;
    using databus_1024_t = DataBus<1024, bus_extensions::BusExtensionDBI>;
    using databus_4096_t = DataBus<4096, bus_extensions::BusExtensionDBI>;

    using databus_preset_sequence_t = DRAMUtils::util::type_sequence<
        databus_64_t,
        databus_256_t,
        databus_1024_t,
        databus_4096_t
    >;

    using databus_preset_t = util::DataBusContainerProxy<databus_preset_sequence_t>;

    inline databus_preset_t getDataBusPreset(
        const size_t width,
        DataBusConfig&& busConfig
    ) {
        if (width <= 64) {
            return databus_64_t(std::move(busConfig));
        } else if (width <= 256) {
            return databus_256_t(std::move(busConfig));
        } else if (width <= 1024) {
            return databus_1024_t(std::move(busConfig));
        } else if (width > 4096) {
            assert(false);
            throw std::runtime_error("Data bus width exceeds maximum.");
        }
        return databus_4096_t(std::move(busConfig));
    }

} // namespace DRAMPower::util::databus_presets

#endif /* DRAMPOWER_UTIL_DATABUS_PRESETS */
