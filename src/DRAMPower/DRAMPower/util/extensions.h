#ifndef DRAMPOWER_UTIL_EXTENSIONS
#define DRAMPOWER_UTIL_EXTENSIONS

#include <DRAMPower/command/Command.h>
#include <DRAMPower/util/extension_base.h>
#include <DRAMPower/util/databus_presets.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

#include <functional>
#include <optional>

namespace DRAMPower::extensions {

class Base : public util::Serialize, public util::Deserialize {
protected:
    // Protected constructor to prevent instantiation of Base class
    Base() = default;

    Base(const Base&) = default;
    Base(Base&&) = default;
    Base& operator=(const Base&) = default;
    Base& operator=(Base&&) = default;

public:
    virtual ~Base() = default;
};

class DBI : public Base {

// Public type definitions
public:
    using enable_callback_t = std::function<bool(const timestamp_t, const bool)>;
    using timestamp_t = DRAMPower::timestamp_t;

// Constructors
public:
    template<typename Func>
    explicit DBI(Func&& callback, bool initstate)
    : m_enabled(initstate)
    , m_callback(std::forward<Func>(callback))
    {}

// Public member functions
public:
    bool enable(timestamp_t timestamp, bool enable);
    bool isEnabled() const;

// Overrides
public:
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;

// Private member variables
private:
    bool m_enabled = false;
    std::optional<enable_callback_t> m_callback;
};

} // namespace DRAMPower::extensions

#endif /* DRAMPOWER_UTIL_EXTENSIONS */
