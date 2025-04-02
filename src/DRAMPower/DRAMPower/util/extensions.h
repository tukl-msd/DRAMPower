#ifndef DRAMPOWER_UTIL_EXTENSIONS
#define DRAMPOWER_UTIL_EXTENSIONS

#include <DRAMPower/command/Command.h>
#include <DRAMPower/util/extension_base.h>
#include <DRAMPower/util/databus_presets.h>
#include <DRAMPower/Types.h>

#include <functional>
#include <optional>

namespace DRAMPower::extensions {

class DRAMPowerExtensionBase {

};

class DRAMPowerExtensionDBI : public DRAMPowerExtensionBase {

// Public type definitions
public:
    using databus_preset_t = DRAMPower::util::databus_presets::databus_preset_t;
    // void(timestamp_t timestamp, bool previousState, bool newState)
    using interface_callback_t = std::function<void(timestamp_t, bool, bool)>;
    using timestamp_t = DRAMPower::timestamp_t;

// Constructors
public:
    explicit DRAMPowerExtensionDBI(databus_preset_t& dataBus, std::optional<interface_callback_t>&& callback = std::nullopt);

// Public member functions
public:
    void set(timestamp_t timestamp, bool dbi);
    bool get() const;

// Private member variables
private:
    bool m_dbi = false;
    databus_preset_t& m_dataBus;
    std::optional<interface_callback_t> m_callback;
};

} // namespace DRAMPower::extensions

#endif /* DRAMPOWER_UTIL_EXTENSIONS */
