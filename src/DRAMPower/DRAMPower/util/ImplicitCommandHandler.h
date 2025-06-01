#ifndef DRAMPOWER_STANDARDS_DDR4_IMPLICITCOMMANDHANDLER_H
#define DRAMPOWER_STANDARDS_DDR4_IMPLICITCOMMANDHANDLER_H

#include <DRAMPower/Types.h>

#include <deque>
#include <functional>
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
    Inserter() = delete; // No default constructor
    Inserter(const Inserter&) = delete; // No copy constructor
    Inserter& operator=(const Inserter&) = delete; // No copy assignment operator

    // Constructor that takes a reference to the implicit command list
    explicit Inserter(implicitCommandList_t& implicitCommandList);

// Public constructors and assignment operators
public:
    Inserter(Inserter&&) = default;
    Inserter& operator=(Inserter&&) = delete;
    ~Inserter() = default;

// Public member functions
public:
    // Add an implicit command to the list
    template <typename Func>
    void addImplicitCommand(timestamp_t timestamp, Func&& func)
    {
        details::addImplicitCommand(m_implicitCommandList, timestamp, std::forward<Func>(func));
    }

    std::size_t implicitCommandCount() const;

// Private member variables
private:
    implicitCommandList_t& m_implicitCommandList;
};

// Helper type definitions
public:
    using Inserter_t = Inserter;

// Constructors and assignment operators
public:
    ImplicitCommandHandler() = default;
    ImplicitCommandHandler(const ImplicitCommandHandler&) = delete; // No copy constructor
    ImplicitCommandHandler& operator=(const ImplicitCommandHandler&) = delete; // No copy assignment operator
    ImplicitCommandHandler(ImplicitCommandHandler&&) = default;
    ImplicitCommandHandler& operator=(ImplicitCommandHandler&&) = default;
    ~ImplicitCommandHandler() = default;

// Inserter forwards
public:
    template <typename Func>
    void addImplicitCommand(timestamp_t timestamp, Func&& func)
    {
        details::addImplicitCommand(m_implicitCommandList, timestamp, std::forward<Func>(func));
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
    implicitCommandList_t m_implicitCommandList;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR4_IMPLICITCOMMANDHANDLER_H */
