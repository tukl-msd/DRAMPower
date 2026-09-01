#ifndef DRAMPOWER_STANDARDS_HBM2_HBM2PATTERN_H
#define DRAMPOWER_STANDARDS_HBM2_HBM2PATTERN_H

#include "DRAMPower/command/Pattern.h"
#include "DRAMPower/standards/hbm2/HBM2Command.h"
#include <istream>
#include <ostream>
#include <vector>

namespace DRAMPower {

namespace pattern_descriptor_HBM2 {
    enum t {
        H, L,
        V, X,
        BA0, BA1, BA2, BA3,
        C0,
        C1,  C2,  C3,  C4,  C5,
        R0,  R1,  R2,  R3,  R4,  R5,  R6,  R7,  R8,  R9,  R10, R11, R12, R13, R14,
        PC0,
        SID0, SID1,
        PAR, // Parity for parity mode
    };
} // namespace pattern_descriptor

struct HBM2PatternExtraData {
    void serialize([[maybe_unused]] std::ostream& stream) const {}
    void deserialize([[maybe_unused]] std::istream& stream) {}
};

struct HBM2Encoder {
    static uint64_t encode(const HBM2TargetCoordinate& targetCoordinate, const std::vector<pattern_descriptor_HBM2::t>& pattern, const BasePatternEncoderOverrides<pattern_descriptor_HBM2::t>& settings, const uint64_t lastpattern, const HBM2PatternExtraData& extraData);
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_HBM2_HBM2PATTERN_H */
