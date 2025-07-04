#ifndef DRAMPOWER_STANDARDS_DDR4_IMPLICITCOMMANDHANDLER_H
#define DRAMPOWER_STANDARDS_DDR4_IMPLICITCOMMANDHANDLER_H

#include <DRAMPower/Types.h>

#include <deque>
#include <functional>
#include <memory>
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

}

class ImplicitCommandHandler {
// Public type definitions
public:
    using implicitCommand_t = std::function<void(void)>;
    using implicitCommandListEntry_t = std::pair<timestamp_t, implicitCommand_t>;
    using implicitCommandList_t = std::deque<implicitCommandListEntry_t>;

// Helper classes
public:
class Inserter {
// Friend classes
    friend class ImplicitCommandHandler;

// Private constructors and assignment operators
private:
    // Constructor that takes a reference to the implicit command list
    explicit Inserter(const std::shared_ptr<implicitCommandList_t>& implicitCommandList);

// Public constructors and assignment operators
public:
    Inserter() = delete; // No default constructor
    Inserter(const Inserter&) = default; // copy constructor
    Inserter& operator=(const Inserter&) = default; // copy assignment operator

    Inserter(Inserter&&) = default; // move constructor
    Inserter& operator=(Inserter&&) = default; // move assignment operator

// Public member functions
public:
    // Add an implicit command to the list
    template <typename Func>
    bool addImplicitCommand(timestamp_t timestamp, Func&& func)
    {
        if (m_implicitCommandList.expired()) {
            return false; // No implicit command list available
        }
        details::addImplicitCommand(*m_implicitCommandList.lock(), timestamp, std::forward<Func>(func));
        return true; // Command added successfully
    }

    std::size_t implicitCommandCount() const;

// Private member variables
private:
    std::weak_ptr<implicitCommandList_t> m_implicitCommandList;
};

// Helper type definitions
public:
    using Inserter_t = Inserter;

// Constructors and assignment operators
public:
    ImplicitCommandHandler(const ImplicitCommandHandler&) = default; // copy constructor
    ImplicitCommandHandler& operator=(const ImplicitCommandHandler&) = default; // copy assignment operator
    ImplicitCommandHandler(ImplicitCommandHandler&&) = default; // move constructor
    ImplicitCommandHandler& operator=(ImplicitCommandHandler&&) = default; // move assignment operator
    ~ImplicitCommandHandler() = default;

    ImplicitCommandHandler();

// Inserter forwards
public:
    template <typename Func>
    void addImplicitCommand(timestamp_t timestamp, Func&& func)
    {
        details::addImplicitCommand(*m_implicitCommandList, timestamp, std::forward<Func>(func));
    }

// Public member functions
public:
    // Create a inserter for adding implicit commands which holds a reference to the implicit command list
    // The lifetime of the inserter must be managed by the caller
    Inserter_t createInserter();
    // Process implicit command queue
    void processImplicitCommandQueue(timestamp_t timestamp, timestamp_t &last_command_time);
    std::size_t implicitCommandCount() const;

// Private member variables
private:
    std::shared_ptr<implicitCommandList_t> m_implicitCommandList;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR4_IMPLICITCOMMANDHANDLER_H */
