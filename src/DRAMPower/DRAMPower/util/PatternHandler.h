#ifndef DRAMPOWER_UTIL_PATTERNHANDLER_H
#define DRAMPOWER_UTIL_PATTERNHANDLER_H

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

#include <utility>
#include <variant>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <stdexcept>
#include <initializer_list>

namespace DRAMPower {

template <
    typename CommandEnum,
    typename pattern_t = pattern_descriptor::t,
    typename TargetCoordinate_t = TargetCoordinate,
    typename Encoder_t = DefaultEncoder,
    typename ExtraData_t = std::monostate
>
class PatternHandler : public util::Serialize, public util::Deserialize {
// Public type definitions+
public:
    using commandEnum_t = CommandEnum;
    using commandPattern_t = std::vector<pattern_t>;
    using commandPatternMap_t = std::vector<commandPattern_t>;
    using PatternEncoder_t = BasePatternEncoder<pattern_t, TargetCoordinate_t, Encoder_t, ExtraData_t>;

// Constructors and assignment operators
public:
    PatternHandler() = delete; // No default constructor
    PatternHandler(const PatternHandler&) = default;
    PatternHandler& operator=(const PatternHandler&) = default;
    PatternHandler(PatternHandler&&) = default;
    PatternHandler& operator=(PatternHandler&&) = default;
    ~PatternHandler() = default;

    // Constructor with encoder overrides and initial patterns
    template<typename... ExtraDataArgs>
    explicit PatternHandler(BasePatternEncoderOverrides<pattern_t> encoderoverrides, uint64_t initPattern = 0, ExtraDataArgs&&... extraDataArgs)
        : m_initPattern(initPattern)
        , m_encoder(encoderoverrides, std::forward<ExtraDataArgs>(extraDataArgs)...)
        , m_commandPatternMap(static_cast<std::size_t>(commandEnum_t::COUNT), commandPattern_t {})
        , m_lastPattern(initPattern)
    {}
    // Constructor with no encoder overrides and initial patterns
    explicit PatternHandler(uint64_t initPattern = 0)
        : m_initPattern(initPattern)
        , m_commandPatternMap(static_cast<std::size_t>(commandEnum_t::COUNT), commandPattern_t {})
        , m_lastPattern(initPattern)
    {}

// Public member functions
public:
    PatternEncoder_t& getEncoder() { return m_encoder; }
    const PatternEncoder_t& getEncoder() const { return m_encoder; }

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

    const commandPattern_t& getPattern(commandEnum_t cmd_type) const
    {
        assert(m_commandPatternMap.size() > static_cast<std::size_t>(cmd_type));
        return m_commandPatternMap[static_cast<std::size_t>(cmd_type)];
    }
    uint64_t getCommandPattern(commandEnum_t type, const TargetCoordinate_t& coordinate)
    {
        if (m_commandPatternMap[static_cast<std::size_t>(type)].empty()) {
            // No pattern registered for this command
            throw std::runtime_error("No pattern registered for this command");
        }
        const auto& pattern = m_commandPatternMap[static_cast<std::size_t>(type)];
        m_lastPattern = m_encoder.encode(coordinate, pattern, m_lastPattern);
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

    void reset() {
        m_lastPattern = m_initPattern;
        m_encoder.reset();
    }

// Overrides
public:
    void serialize(std::ostream& stream) const override {
        stream.write(reinterpret_cast<const char *>(&m_initPattern), sizeof(m_initPattern));
        m_encoder.serialize(stream);
    }
    void deserialize(std::istream& stream) override {
        stream.read(reinterpret_cast<char *>(&m_initPattern), sizeof(m_initPattern));
        m_encoder.deserialize(stream);
    }

// Private member variables
private:
    uint64_t m_initPattern;
    PatternEncoder_t m_encoder;
    commandPatternMap_t m_commandPatternMap;
    uint64_t m_lastPattern;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_UTIL_PATTERNHANDLER_H */
