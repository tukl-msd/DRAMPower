#ifndef DRAMPOWER_UTIL_IMPLICITCOMMANDHANDLER_H
#define DRAMPOWER_UTIL_IMPLICITCOMMANDHANDLER_H

#include <DRAMPower/Types.h>

#include <deque>
#include <functional>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <cstddef>

namespace DRAMPower {

namespace details {
    template <typename Queue, typename Func>
    void addImplicitCommand(Queue& queue, timestamp_t timestamp, Func&& func)
    {
        auto entry = std::make_pair(timestamp, std::forward<Func>(func));

        auto upper = std::upper_bound(queue.begin(), queue.end(),
            entry,
            [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

        queue.emplace(upper, entry);
    }

} // namespace details

template <typename CommandContext = void>
class ImplicitCommandHandler;

template<typename CommandContext>
class ImplicitCommandHandler {
// Public type definitions
public:
    using CommandContext_t = std::add_lvalue_reference_t<std::remove_reference_t<CommandContext>>;
    using implicitCommand_t = std::function<void(CommandContext_t)>;
    using implicitCommandListEntry_t = std::pair<timestamp_t, implicitCommand_t>;
    using implicitCommandList_t = std::deque<implicitCommandListEntry_t>;

    
// Public member functions
public:
    template <typename Func>
    void addImplicitCommand(timestamp_t timestamp, Func&& func)
    {
        details::addImplicitCommand(m_implicitCommandList, timestamp, std::forward<Func>(func));
    }

    void processImplicitCommandQueue(CommandContext_t context, timestamp_t timestamp, timestamp_t &last_command_time) {
        while (!m_implicitCommandList.empty() && m_implicitCommandList.front().first <= timestamp) {
            // Execute implicit command functor
            auto& [i_timestamp, i_implicitCommand] = m_implicitCommandList.front();
            i_implicitCommand(context);
            last_command_time = i_timestamp;
            m_implicitCommandList.pop_front();
        }
    }

    std::size_t implicitCommandCount() const {
        return m_implicitCommandList.size();
    }

// Private member variables
private:
    implicitCommandList_t m_implicitCommandList;
};

template<>
class ImplicitCommandHandler<void> {
// Public type definitions
public:
    using implicitCommand_t = std::function<void(void)>;
    using implicitCommandListEntry_t = std::pair<timestamp_t, implicitCommand_t>;
    using implicitCommandList_t = std::deque<implicitCommandListEntry_t>;

// Public member functions
public:
    template <typename Func>
    void addImplicitCommand(timestamp_t timestamp, Func&& func)
    {
        details::addImplicitCommand(m_implicitCommandList, timestamp, std::forward<Func>(func));
    }

    void processImplicitCommandQueue(timestamp_t timestamp, timestamp_t &last_command_time) {
        while (!m_implicitCommandList.empty() && m_implicitCommandList.front().first <= timestamp) {
            // Execute implicit command functor
            auto& [i_timestamp, i_implicitCommand] = m_implicitCommandList.front();
            i_implicitCommand();
            last_command_time = i_timestamp;
            m_implicitCommandList.pop_front();
        }
    }

    std::size_t implicitCommandCount() const {
        return m_implicitCommandList.size();
    }

// Private member variables
private:
    implicitCommandList_t m_implicitCommandList;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_UTIL_IMPLICITCOMMANDHANDLER_H */
