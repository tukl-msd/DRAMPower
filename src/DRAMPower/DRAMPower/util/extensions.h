#ifndef DRAMPOWER_UTIL_EXTENSIONS
#define DRAMPOWER_UTIL_EXTENSIONS

#include <DRAMPower/command/Command.h>
#include <DRAMPower/util/extension_base.h>
#include <DRAMPower/util/databus_presets.h>
#include <DRAMPower/util/bus_extensions.h>
#include <DRAMPower/Types.h>

#include <functional>
#include <optional>

namespace DRAMPower::extensions {

class Base {

};

class DBI : public Base {

// Public type definitions
public:
    using enable_callback_t = std::function<void(const timestamp_t, const bool)>;
    using timestamp_t = DRAMPower::timestamp_t;

// Constructors
public:
    template<typename Func>
    explicit DBI(Func&& callback, bool initstate)
    : m_enabled(initstate)
    , m_callback(std::move(callback))
    {}

// Public member functions
public:
    void enable(timestamp_t timestamp, bool enable);
    bool isEnabled() const;

// Private member variables
private:
    bool m_enabled = false;
    std::optional<enable_callback_t> m_callback;
};

} // namespace DRAMPower::extensions

#endif /* DRAMPOWER_UTIL_EXTENSIONS */
