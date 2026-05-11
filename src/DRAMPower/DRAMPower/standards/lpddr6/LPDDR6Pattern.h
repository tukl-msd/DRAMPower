#ifndef DRAMPOWER_STANDARDS_LPDDR6_LPDDR6PATTERN_H
#define DRAMPOWER_STANDARDS_LPDDR6_LPDDR6PATTERN_H

#include "DRAMPower/command/Pattern.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Command.h"
#include <istream>
#include <ostream>
#include <vector>

namespace DRAMPower {

namespace pattern_descriptor_LPDDR6 {
    enum t {
        H, L,
        V, X,
        BG0, BG1,
        DBG0, DBG1,
        BA0, BA1,

        C0, // L for BL48 write, else C0
        C1,  C2,  C3,  C4,  C5,
        R0,  R1,  R2,  R3,  R4,  R5,  R6,  R7,  R8,  R9,  R10, R11, R12, R13, R14, R15, R16,
        BL, // H for BL24, L for BL48
        SC, // Sub Channel for efficiency mode

        PAR, // Parity for parity mode
    };
} // namespace pattern_descriptor

struct LPDDR6PatternExtraData {
    std::size_t currentBurstLength = 0;
    bool parity_check_mode = false;

    void serialize(std::ostream& stream) const {
        stream.write(reinterpret_cast<const char*>(&currentBurstLength), sizeof(currentBurstLength));
        stream.write(reinterpret_cast<const char*>(&parity_check_mode), sizeof(parity_check_mode));
    }
    void deserialize(std::istream& stream) {
        stream.read(reinterpret_cast<char*>(&currentBurstLength), sizeof(currentBurstLength));
        stream.read(reinterpret_cast<char*>(&parity_check_mode), sizeof(parity_check_mode));
    }
};

struct LPDDR6Encoder {
    static uint64_t encode(const LPDDR6TargetCoordinate& targetCoordinate, const std::vector<pattern_descriptor_LPDDR6::t>& pattern, const BasePatternEncoderOverrides<pattern_descriptor_LPDDR6::t>& settings, const uint64_t lastpattern, const LPDDR6PatternExtraData& extraData);
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR6_LPDDR6PATTERN_H */
