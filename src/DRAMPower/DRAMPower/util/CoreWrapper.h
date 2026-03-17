#ifndef DRAMPOWER_STANDARDS_DDR4_COREWRAPPER_H
#define DRAMPOWER_STANDARDS_DDR4_COREWRAPPER_H

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/data/stats.h"
#include "DRAMPower/util/Deserialize.h"
#include "DRAMPower/util/ImplicitCommandHandler.h"
#include "DRAMPower/util/Serialize.h"
#include <utility>


namespace DRAMPower {

template<typename Core>
class CoreWrapper : public util::Serialize, public util::Deserialize {
// Public type definitions
public:
    using ImplicitCommandHandler_t = ImplicitCommandHandler;
    using implicitCommandInserter_t = typename ImplicitCommandHandler::Inserter_t;

    // Forward constructor
    template<typename... Args>
    CoreWrapper(Args&&... args)
        : m_implicitCommandHandler{}
        , m_core(m_implicitCommandHandler.createInserter(), std::forward<Args>(args)...)
    {}

// Public member functions
public:
    void processImplicitCommandQueue(timestamp_t timestamp, timestamp_t& last_command_time)
    {
        m_implicitCommandHandler.processImplicitCommandQueue(timestamp, last_command_time);
    }

    void doCommand(const Command& cmd) {
        processImplicitCommandQueue(cmd.timestamp, m_last_command_time);
        m_last_command_time = std::max(cmd.timestamp, m_last_command_time);
        m_core.doCommand(cmd);
    }

    void getWindowStats(timestamp_t timestamp, SimulationStats& stats) {
        processImplicitCommandQueue(timestamp, m_last_command_time);
        m_core.getWindowStats(timestamp, stats);
    }

    SimulationStats getWindowStats(timestamp_t timestamp) {
        SimulationStats stats;
        getWindowStats(timestamp, stats);
        return stats;
    }

    std::size_t implicitCommandCount() const {
        return m_implicitCommandHandler.implicitCommandCount();
    }

    bool isSerializable() const {
        return 0 == implicitCommandCount();
    }

    timestamp_t getLastCommandTime() const
    {
        return m_last_command_time;
    }

    Core& getCore() {
        return m_core;
    }
    const Core& getCore() const {
        return m_core;
    }

    void serialize(std::ostream& stream) const {
        stream.write(reinterpret_cast<const char*>(&m_last_command_time), sizeof(m_last_command_time));
        m_core.serialize(stream);
    }

    void deserialize(std::istream& stream) {
        stream.read(reinterpret_cast<char*>(&m_last_command_time), sizeof(m_last_command_time));
        m_core.deserialize(stream);
    }

// Private member variables
private:
    ImplicitCommandHandler_t m_implicitCommandHandler;
    Core m_core;
    timestamp_t m_last_command_time = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR4_COREWRAPPER_H */
