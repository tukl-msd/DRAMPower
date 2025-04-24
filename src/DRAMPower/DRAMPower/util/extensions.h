#ifndef DRAMPOWER_UTIL_EXTENSIONS
#define DRAMPOWER_UTIL_EXTENSIONS

#include <DRAMPower/command/Command.h>
#include <DRAMPower/util/extension_base.h>
#include <DRAMPower/util/databus_presets.h>
#include <DRAMPower/Types.h>

#include <functional>
#include <optional>

namespace DRAMPower::extensions {

class Base {

};

class DBI : public Base {

// Public type definitions
public:
    using databus_preset_t = DRAMPower::util::databus_presets::databus_preset_t;
    // void(const timestamp_t timestamp, const bool enable)
    using interface_callback_t = std::function<void(const timestamp_t, const bool)>;
    using timestamp_t = DRAMPower::timestamp_t;

// Constructors
public:
    explicit DBI(databus_preset_t& dataBus, std::optional<interface_callback_t>&& callback = std::nullopt);

// Public member functions
public:
    void enable(timestamp_t timestamp, bool enable);
    bool isEnabled() const;

// Private member variables
private:
    bool m_enabled = false;
    databus_preset_t& m_dataBus;
    std::optional<interface_callback_t> m_callback;
};


class LPDDR6EfficiencyMode : public Base {

// Public type definitions
public:
    using interface_callback_t = std::function<void(const timestamp_t, const bool enabled)>;

// Constructors
public:
    explicit LPDDR6EfficiencyMode(const bool enabled, interface_callback_t&& callback);

public:
    void enable(timestamp_t timestamp, bool enable);
    bool isEnabled() const;

// Private member variables
private:
    interface_callback_t m_callback;
    bool m_enabled = false;
};


} // namespace DRAMPower::extensions

#endif /* DRAMPOWER_UTIL_EXTENSIONS */
