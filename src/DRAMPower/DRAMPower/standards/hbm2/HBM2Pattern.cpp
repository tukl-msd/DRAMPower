#include "DRAMPower/standards/hbm2/HBM2Pattern.h"
#include "DRAMPower/command/Pattern.h"

#include <DRAMUtils/memspec/standards/MemSpecHBM2.h>

#include <bitset>


namespace DRAMPower {

void HBM2PatternExtraData::reset() {}

uint64_t HBM2Encoder::encode(const HBM2TargetCoordinate& targetCoordinate, const std::vector<pattern_descriptor_HBM2::t>& pattern, const BasePatternEncoderOverrides<pattern_descriptor_HBM2::t>&, const uint64_t, [[maybe_unused]] const HBM2PatternExtraData& extraData) {
    using namespace pattern_descriptor_HBM2;

    std::bitset<64> bitset(0);
    std::bitset<32> bank_bits(targetCoordinate.bank);
    std::bitset<32> row_bits(targetCoordinate.row);
    std::bitset<32> column_bits(targetCoordinate.column);
    std::bitset<32> pseudo_channel_bits(targetCoordinate.pseudoChannel);
    std::bitset<32> stack_bits(targetCoordinate.stack);

    std::size_t n = pattern.size() - 1;

    assert(n < 64);

    for (const auto descriptor : pattern) {
        // assert(n >= 0); // std::size_t is unsigned

        switch (descriptor) {
        case H:
            bitset[n] = true;
            break;
        case L:
            bitset[n] = false;
            break;

        // Command bits
        // when CA Parity mode is enabled, "X" defined by the command truth table shall be replaced to "V" (Valid)
        case X:
        case V:
            bitset[n] = true;
            break;
            
        // Target Coordinate bits
        // Bank bits
        case BA0:
            bitset[n] = bank_bits[0];
            break;
        case BA1:
            bitset[n] = bank_bits[1];
            break;
        case BA2:
            bitset[n] = bank_bits[2];
            break;
        case BA3:
            bitset[n] = bank_bits[3];
            break;
        // Column bits
        case C0:
            bitset[n] = true;
            break;
        case C1:
            bitset[n] = column_bits[1];
            break;
        case C2:
            bitset[n] = column_bits[2];
            break;
        case C3:
            bitset[n] = column_bits[3];
            break;
        case C4:
            bitset[n] = column_bits[4];
            break;
        case C5:
            bitset[n] = column_bits[5];
            break;
        // Row bits
        case R0:
            bitset[n] = row_bits[0];
            break;
        case R1:
            bitset[n] = row_bits[1];
            break;
        case R2:
            bitset[n] = row_bits[2];
            break;
        case R3:
            bitset[n] = row_bits[3];
            break;
        case R4:
            bitset[n] = row_bits[4];
            break;
        case R5:
            bitset[n] = row_bits[5];
            break;
        case R6:
            bitset[n] = row_bits[6];
            break;
        case R7:
            bitset[n] = row_bits[7];
            break;
        case R8:
            bitset[n] = row_bits[8];
            break;
        case R9:
            bitset[n] = row_bits[9];
            break;
        case R10:
            bitset[n] = row_bits[10];
            break;
        case R11:
            bitset[n] = row_bits[11];
            break;
        case R12:
            bitset[n] = row_bits[12];
            break;
        case R13:
            bitset[n] = row_bits[13];
            break;
        case R14:
            bitset[n] = row_bits[14];
            break;
        // Pseudo channel bits
        case PC0:
            bitset[n] = pseudo_channel_bits[0];
            break;
        // Stack bits
        case SID0:
            bitset[n] = stack_bits[0];
            break;
        case SID1:
            bitset[n] = stack_bits[1];
            break;
        case PAR:
            // Calculated after the bitset is computed
            bitset[n] = true;
            break;
        }
        --n;
    }
    return bitset.to_ullong();
}

} // namespace DRAMPower