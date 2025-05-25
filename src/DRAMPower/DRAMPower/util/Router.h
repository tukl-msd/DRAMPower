#ifndef DRAMPOWER_STANDARDS_DDR4_ROUTER_H
#define DRAMPOWER_STANDARDS_DDR4_ROUTER_H

#include <DRAMPower/command/Command.h>

#include <optional>
#include <vector>
#include <functional>
#include <cassert>
#include <cstddef>

namespace DRAMPower {

template <typename CommandEnum>
class Router {
// Public type definitions
public:
    using commandEnum_t = CommandEnum;
    using commandHandler_t = std::function<void(const Command&)>;
    using commandRouter_t = std::vector<commandHandler_t>;

// Constructors and assignment operators
public:
    Router() = delete; // No default constructor
    Router(const Router&) = default;
    Router& operator=(const Router&) = default;
    Router(Router&&) = default;
    Router& operator=(Router&&) = default;
    ~Router() = default;

    // constructor with default handler
    Router(std::optional<commandHandler_t> defaultHandler = std::nullopt)
        : m_router(static_cast<std::size_t>(commandEnum_t::COUNT), defaultHandler.value_or([](const Command&) {}))
    {}

// Public member functions
public:
    // Add a command handler to the router for the given command
    template <commandEnum_t cmd, typename Func>
    void routeCommand(Func&& func)
    {
        assert(m_router.size() > static_cast<std::size_t>(cmd));
        m_router[static_cast<std::size_t>(cmd)] = std::forward<Func>(func);
    }

    // Execute the command handler for the given command
    void executeCommand(const Command& command) const
    {
        assert(m_router.size() > static_cast<std::size_t>(command.type));
        m_router[static_cast<std::size_t>(command.type)](command);
    }
    std::size_t size() const
    {
        return m_router.size();
    }

// Private member variables
private:
    commandRouter_t m_router;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR4_ROUTER_H */
