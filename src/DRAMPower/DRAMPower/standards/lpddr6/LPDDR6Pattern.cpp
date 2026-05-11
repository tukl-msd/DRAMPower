#include "DRAMPower/standards/lpddr6/LPDDR6Pattern.h"
#include "DRAMPower/command/Pattern.h"

#include <bitset>


namespace DRAMPower {

uint64_t LPDDR6Encoder::encode(const LPDDR6TargetCoordinate& targetCoordinate, const std::vector<pattern_descriptor_LPDDR6::t>& pattern, const BasePatternEncoderOverrides<pattern_descriptor_LPDDR6::t>&, const uint64_t, const LPDDR6PatternExtraData& extraData) {
    using namespace pattern_descriptor_LPDDR6;

        std::bitset<64> bitset(0);
        // Bank is rank relative
        std::bitset<2> bank_bits(targetCoordinate.bank & 0x3);
        std::bitset<17> row_bits(targetCoordinate.row & 0x1FFFF);
        std::bitset<6> column_bits(targetCoordinate.column & 0x3F);
        std::bitset<2> bank_group_bits(targetCoordinate.bankGroup & 0x2);
        std::bitset<2> dbank_group_bits(targetCoordinate.dbankGroup & 0x2);
        bool sc_bit = (1 == targetCoordinate.subChannel);

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
                bitset[n] = false;
                break;
                
            // Burst length dependend
            case BL: // BL24 or BL48
                assert((24 == extraData.currentBurstLength || 48 == extraData.currentBurstLength) && "Invalid burstlength");
                bitset[n] = (24 == extraData.currentBurstLength);
                break;

            // Target Coordinate bits
            // Bank bits
            case BA0:
                bitset[n] = bank_bits[0];
                break;
            case BA1:
                bitset[n] = bank_bits[1];
                break;
            // Bankgroup bits
            case BG0:
                bitset[n] = bank_group_bits[0];
                break;
            case BG1:
                bitset[n] = bank_group_bits[1];
                break;
            case DBG0:
                bitset[n] = bank_group_bits[0];
                break;
            case DBG1:
                bitset[n] = bank_group_bits[0];
                break;
            // Column bits
            case C0:
                bitset[n] = column_bits[0];
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
            case R15:
                bitset[n] = row_bits[15];
                break;
            case R16:
                bitset[n] = row_bits[16];
                break;
            // Subchannel bit for efficiency mode
            case SC:
                bitset[n] = sc_bit; // TODO: normal mode
                break;
            case PAR:
                // Calculated after the bitset is computed
                break;
            }
            --n;
        }

        // Parity
        if (extraData.parity_check_mode && PAR == pattern[2]) {
            // When CA Parity is enabled, the Host 
            // generates an even bit parity based on the 1st and 2nd rising and falling edges of CA[3:0] 
            // signals, excluding the CA Parity bit
            bitset[pattern.size() - 3] = (1 == bitset.count() % 2);
        }

        return bitset.to_ullong();
}

} // namespace DRAMPower