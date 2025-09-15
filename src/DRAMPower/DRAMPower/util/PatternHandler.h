#ifndef DRAMPOWER_STANDARDS_DDR4_PATTERNHANDLER_H
#define DRAMPOWER_STANDARDS_DDR4_PATTERNHANDLER_H

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

#include <vector>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <stdexcept>
#include <initializer_list>

namespace DRAMPower {

template <typename CommandEnum>
class PatternHandler : public util::Serialize, public util::Deserialize {
// Public type definitions+
public:
    using commandEnum_t = CommandEnum;
    using commandPattern_t = std::vector<pattern_descriptor::t>;
    using commandPatternMap_t = std::vector<commandPattern_t>;

// Constructors and assignment operators
public:
    PatternHandler() = delete; // No default constructor
    PatternHandler(const PatternHandler&) = default;
    PatternHandler& operator=(const PatternHandler&) = default;
    PatternHandler(PatternHandler&&) = default;
    PatternHandler& operator=(PatternHandler&&) = default;
    ~PatternHandler() = default;

    // Constructor with encoder overrides and initial patterns
    explicit PatternHandler(PatternEncoderOverrides encoderoverrides, uint64_t initPattern = 0)
        : m_encoder(encoderoverrides)
        , m_commandPatternMap(static_cast<std::size_t>(commandEnum_t::COUNT), commandPattern_t {})
        , m_lastPattern(initPattern)
    {}
    // Constructor with no encoder overrides and initial patterns
    explicit PatternHandler(uint64_t initPattern = 0)
        : m_lastPattern(initPattern)
        , m_commandPatternMap(static_cast<std::size_t>(commandEnum_t::COUNT), commandPattern_t {})
    {}

// Public member functions
public:
    PatternEncoder& getEncoder() { return m_encoder; }
    const PatternEncoder& getEncoder() const { return m_encoder; }

    template <commandEnum_t cmd_type>
    void registerPattern(const commandPattern_t &pattern)
    {
        assert(m_commandPatternMap.size() > static_cast<std::size_t>(cmd_type));
        m_commandPatternMap[static_cast<std::size_t>(cmd_type)] = pattern;
    }
    template <commandEnum_t cmd_type>
    void registerPattern(std::initializer_list<pattern_descriptor::t> pattern)
    {
        assert(m_commandPatternMap.size() > static_cast<std::size_t>(cmd_type));
        m_commandPatternMap[static_cast<std::size_t>(cmd_type)] = commandPattern_t(pattern);
    }

    const commandPattern_t& getPattern(CmdType cmd_type) const
    {
        assert(m_commandPatternMap.size() > static_cast<std::size_t>(cmd_type));
        return m_commandPatternMap[static_cast<std::size_t>(cmd_type)];
    }
    uint64_t getCommandPattern(const Command& cmd)
    {
        if (m_commandPatternMap[static_cast<std::size_t>(cmd.type)].empty()) {
            // No pattern registered for this command
            throw std::runtime_error("No pattern registered for this command");
        }
        const auto& pattern = m_commandPatternMap[static_cast<std::size_t>(cmd.type)];
        m_lastPattern = m_encoder.encode(cmd, pattern, m_lastPattern);
        return m_lastPattern;
    }

    uint64_t getCoordinatePattern(const TargetCoordinate& coordinate, const commandPattern_t& pattern)
    {
        if (pattern.empty()) {
            // No pattern provided
            throw std::runtime_error("No pattern provided for this coordinate");
        }
        m_lastPattern = m_encoder.encode(coordinate, pattern, m_lastPattern);
        return m_lastPattern;
    }

// Overrides
public:
    void serialize(std::ostream& stream) const override {
        m_encoder.serialize(stream);
    }
    void deserialize(std::istream& stream) override {
        m_encoder.deserialize(stream);
    }

// Private member variables
private:
    PatternEncoder m_encoder;
    commandPatternMap_t m_commandPatternMap;
    uint64_t m_lastPattern;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR4_PATTERNHANDLER_H */
