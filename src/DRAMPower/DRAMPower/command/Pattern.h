#ifndef DRAMPOWER_COMMAND_PATTERN_H
#define DRAMPOWER_COMMAND_PATTERN_H

#include <DRAMPower/command/Command.h>
#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

#include <cassert>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>
#include <limits.h>

namespace DRAMPower {
namespace pattern_descriptor {
    enum t {
        H, L,
        V, X,
        A0,  A1,  A2,  A3,  A4,  A5,  A6,  A7,  A8,  A9,  A10, A11, A12, A13, A14, A15, A16, A17,
        BG0, BG1, BG2, BA0, BA1, BA2, BA3, BA4, BA5, BA6, BA7, BA8,
        C0,  C1,  C2,  C3,  C4,  C5,  C6,  C7,  C8,  C9,  C10, C11, C12, C13, C14, C15, C16,
        R0,  R1,  R2,  R3,  R4,  R5,  R6,  R7,  R8,  R9,  R10, R11, R12, R13, R14, R15, R16, R17, R18, R19, R20, R21, R22, R23,
        CID0, CID1, CID2, CID3,
        AP,
        BL,
    };
}

enum class PatternEncoderBitSpec
{
    L = 0,
    H = 1,
    LAST_BIT = 2,
    INVALID = -1
};

template<typename pattern_t = pattern_descriptor::t>
// struct for initializer list in PatternEncoderSettings
struct PatternEncoderSettingsEntry {
    pattern_t descriptor;
    PatternEncoderBitSpec bitSpec;
};

// class to store pattern descriptor overrides

template<typename pattern_t = pattern_descriptor::t>
class BasePatternEncoderOverrides
{

private:
    std::unordered_map<pattern_t, PatternEncoderBitSpec> settings;
public:
    // Constructor with initializer list for settings
    BasePatternEncoderOverrides() = default;
    BasePatternEncoderOverrides(std::initializer_list<PatternEncoderSettingsEntry<pattern_t>> _settings) {
        for (const auto &setting : _settings)
        {
            this->settings.emplace(setting.descriptor, setting.bitSpec);
        }
    }
public:
    void updateSettings(std::initializer_list<PatternEncoderSettingsEntry<pattern_t>> _settings) {
        // Update settings if descriptor is already present
        for (const auto &setting : _settings)
        {
            this->settings[setting.descriptor] = setting.bitSpec;
        }
    }
    PatternEncoderBitSpec getSetting(pattern_t descriptor) const {
        if (this->settings.find(descriptor) != this->settings.end())
        {
            return this->settings.at(descriptor);
        }
        return PatternEncoderBitSpec::INVALID;
    }
    const std::unordered_map<pattern_t, PatternEncoderBitSpec>& getSettings() const {
        return this->settings;
    }
    bool hasSetting(pattern_t descriptor) const {
        return this->settings.find(descriptor) != this->settings.end();
    }
    bool removeSetting(pattern_t descriptor) {
        return this->settings.erase(descriptor) > 0;
    }
};
using PatternEncoderOverrides = BasePatternEncoderOverrides<>;


struct DefaultEncoder {
    static uint64_t encode(const TargetCoordinate& targetCoordinate, const std::vector<pattern_descriptor::t>& pattern, const PatternEncoderOverrides& settings, const uint64_t lastpattern, const std::monostate& extraData);
};

template <typename pattern_t = pattern_descriptor::t>
inline bool patternApplyBitSpec(
    const BasePatternEncoderOverrides<pattern_t> &spec,
    pattern_t descriptor,
    bool LAST_BIT,
    bool default_bit
) {
    auto setting = spec.getSetting(descriptor);
    switch (setting)
    {
    case PatternEncoderBitSpec::L:
        return false;
    case PatternEncoderBitSpec::H:
        return true;
    case PatternEncoderBitSpec::LAST_BIT:
        return LAST_BIT;
    case PatternEncoderBitSpec::INVALID:
        return default_bit;
    default:
        assert(false);
        break;
    }
    return false;
}


template <
    typename pattern_t = pattern_descriptor::t,
    typename TargetCoordinate_t = TargetCoordinate,
    typename Encoder_t = DefaultEncoder,
    typename ExtraData_t = std::monostate
>
class BasePatternEncoder : public util::Serialize, public util::Deserialize // Currently LPDDR4
{
public:
    BasePatternEncoderOverrides<pattern_t> settings;
    template <typename... ExtraDataArgs>
    BasePatternEncoder(BasePatternEncoderOverrides<pattern_t> settings = {}, ExtraDataArgs&&... extraDataArgs)
        : settings(settings)
        , m_extraData(std::forward<ExtraDataArgs>(extraDataArgs)...)
    {}
private:

public:
    void setExtraData(const ExtraData_t& extraData) {
        m_extraData = extraData;
    }
    void setExtraData(ExtraData_t&& extraData) {
        m_extraData = extraData;
    }
    const ExtraData_t& getExtraData() const {
        return m_extraData;
    }
    ExtraData_t& getExtraData() {
        return m_extraData;
    }
    void reset() {
        if constexpr (!std::is_same_v<std::decay_t<ExtraData_t>, std::monostate>) {
            m_extraData.reset();
        }
    }

// Overrides
public:
    void serialize(std::ostream& stream) const override {
        if constexpr (!std::is_same_v<std::decay_t<ExtraData_t>, std::monostate>) {
            m_extraData.serialize(stream);
        }
        // settings
        const std::size_t settingsSize = settings.getSettings().size();
        stream.write(reinterpret_cast<const char*>(&settingsSize), sizeof(settingsSize));
        for (const auto& [descriptor, bitSpec] : settings.getSettings()) {
            stream.write(reinterpret_cast<const char*>(&descriptor), sizeof(descriptor));
            stream.write(reinterpret_cast<const char*>(&bitSpec), sizeof(bitSpec));
        }
    }
    void deserialize(std::istream& stream) override  {
        if constexpr (!std::is_same_v<std::decay_t<ExtraData_t>, std::monostate>) {
            m_extraData.deserialize(stream);
        }
        // settings
        std::size_t settingsSize = 0;
        stream.read(reinterpret_cast<char*>(&settingsSize), sizeof(settingsSize));
        settings = BasePatternEncoderOverrides<pattern_t>{};
        for (std::size_t i = 0; i < settingsSize; ++i) {
            pattern_t descriptor;
            PatternEncoderBitSpec bitSpec;
            stream.read(reinterpret_cast<char*>(&descriptor), sizeof(descriptor));
            stream.read(reinterpret_cast<char*>(&bitSpec), sizeof(bitSpec));
            settings.updateSettings({PatternEncoderSettingsEntry<pattern_t>{descriptor, bitSpec}});
        }
    }

public:
    uint64_t encode(const TargetCoordinate_t& coordinate, const std::vector<pattern_t>& pattern, const uint64_t lastpattern) {
        return Encoder_t::encode(coordinate, pattern, settings, lastpattern, m_extraData);
    }

// Private member variables
private:
    ExtraData_t m_extraData{};
};
using PatternEncoder = BasePatternEncoder<>;

} // namespace DRAMPower

#endif /* DRAMPOWER_COMMAND_PATTERN_H */
