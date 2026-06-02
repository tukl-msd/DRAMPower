#include "Pattern.h"
#include <cassert>
#include <bitset>
#include <variant>

namespace DRAMPower {

    uint64_t DefaultEncoder::encode(const TargetCoordinate& targetCoordinate, const std::vector<pattern_descriptor::t>& pattern, const PatternEncoderOverrides& settings, const uint64_t lastpattern, [[maybe_unused]] const std::monostate& extraData)
    {
        using namespace pattern_descriptor;

        std::bitset<64> bitset(0);
        std::bitset<32> bank_bits(targetCoordinate.bank);
        std::bitset<32> row_bits(targetCoordinate.row);
        std::bitset<32> column_bits(targetCoordinate.column);
        std::bitset<32> bank_group_bits(targetCoordinate.bankGroup);

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
            case BL:
            case CID0:
            case CID1:
            case CID2:
            case CID3:
                bitset[n] = patternApplyBitSpec<>(settings, descriptor, ((lastpattern >> n) & 1) == 1, true);
                break;
            case V:
            case X:
            case AP:
                bitset[n] = patternApplyBitSpec(settings, descriptor, ((lastpattern >> n) & 1) == 1, false);
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
            case BA4:
                bitset[n] = bank_bits[4];
                break;
            case BA5:
                bitset[n] = bank_bits[5];
                break;
            case BA6:
                bitset[n] = bank_bits[6];
                break;
            case BA7:
                bitset[n] = bank_bits[7];
                break;
            case BA8:
                bitset[n] = bank_bits[8];
                break;

            // BG bits
            case BG0:
                bitset[n] = bank_group_bits[0];
                break;
            case BG1:
                bitset[n] = bank_group_bits[1];
                break;
            case BG2:
                bitset[n] = bank_group_bits[2];
                break;

            case C0:
                bitset[n] = patternApplyBitSpec(settings, descriptor, ((lastpattern >> n) & 1) == 1, column_bits[0]);
                break;
            case C1:
                bitset[n] = patternApplyBitSpec(settings, descriptor, ((lastpattern >> n) & 1) == 1, column_bits[1]);
                break;
            case C2:
                bitset[n] = patternApplyBitSpec(settings, descriptor, ((lastpattern >> n) & 1) == 1, column_bits[2]);
                break;
            case C3:
                bitset[n] = patternApplyBitSpec(settings, descriptor, ((lastpattern >> n) & 1) == 1, column_bits[3]);
                break;
            case C4:
                bitset[n] = patternApplyBitSpec(settings, descriptor, ((lastpattern >> n) & 1) == 1, column_bits[4]);
                break;
            case C5:
                bitset[n] = column_bits[5];
                break;
            case C6:
                bitset[n] = column_bits[6];
                break;
            case C7:
                bitset[n] = column_bits[7];
                break;
            case C8:
                bitset[n] = column_bits[8];
                break;
            case C9:
                bitset[n] = column_bits[9];
                break;
            case C10:
                bitset[n] = patternApplyBitSpec(settings, descriptor, ((lastpattern >> n) & 1) == 1, column_bits[10]);
                break;
            case C11:
                bitset[n] = column_bits[C11];
                break;
            case C12:
                bitset[n] = column_bits[C12];
                break;
            case C13:
                bitset[n] = column_bits[C13];
                break;
            case C14:
                bitset[n] = column_bits[C14];
                break;
            case C15:
                bitset[n] = column_bits[C15];
                break;
            case C16:
                bitset[n] = column_bits[C16];
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
            case R15:
                bitset[n] = row_bits[15];
                break;
            case R16:
                bitset[n] = row_bits[16];
                break;
            case R17:
                bitset[n] = row_bits[17];
                break;

                default:
                break;
            }

            --n;
        }

        return bitset.to_ullong();
    }

} // namespace DRAMPower