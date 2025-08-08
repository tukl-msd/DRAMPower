#ifndef DRAMPOWER_COMMAND_PATTERN_H
#define DRAMPOWER_COMMAND_PATTERN_H

#include <DRAMPower/command/Command.h>

#include <unordered_map>
#include <vector>

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
        OPCODE, // Example: opcode 0x03 pattern: {H, L, OPCODE, OPCODE, OPCODE, OPCODE} result in 0b100011
    };
}

enum class PatternEncoderBitSpec
{
    L = 0,
    H = 1,
    LAST_BIT = 2,
    INVALID = -1
};

// struct for initializer list in PatternEncoderSettings
struct PatternEncoderSettingsEntry {
    pattern_descriptor::t descriptor;
    PatternEncoderBitSpec bitSpec;
};

// class to store pattern descriptor overrides
class PatternEncoderOverrides
{

private:
    std::unordered_map<pattern_descriptor::t, PatternEncoderBitSpec> settings;
public:
    // Constructor with initializer list for settings
    PatternEncoderOverrides() = default;
    PatternEncoderOverrides(std::initializer_list<PatternEncoderSettingsEntry> _settings);

public:
    void updateSettings(std::initializer_list<PatternEncoderSettingsEntry> _settings);
    std::unordered_map<pattern_descriptor::t, PatternEncoderBitSpec> getSettings();
    PatternEncoderBitSpec getSetting(pattern_descriptor::t descriptor);
    bool hasSetting(pattern_descriptor::t descriptor);
    bool removeSetting(pattern_descriptor::t descriptor);
};

class PatternEncoder // Currently LPDDR4
{
public:
    PatternEncoderOverrides settings;
    PatternEncoder(PatternEncoderOverrides settings);
private:
    inline bool applyBitSpec(
        PatternEncoderOverrides &spec,
        pattern_descriptor::t descriptor,
        bool LAST_BIT,
        bool default_bit
    );

public:
    void setOpcode(uint64_t opcode, uint16_t opcodeLength) {
        m_opcode = opcode;
        m_opcodeLength = opcodeLength;
    }
    uint64_t getOpcode() const {
        return m_opcode;
    }
    uint16_t getOpcodeLength() const {
        return m_opcodeLength;
    }

public:
    uint64_t encode(const Command& cmd, const std::vector<pattern_descriptor::t>& pattern, const uint64_t lastpattern);
    uint64_t encode(const TargetCoordinate& coordinate, const std::vector<pattern_descriptor::t>& pattern, const uint64_t lastpattern);

// Private member variables
private:
    uint64_t m_opcode = 0;
    uint16_t m_opcodeLength = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_COMMAND_PATTERN_H */
