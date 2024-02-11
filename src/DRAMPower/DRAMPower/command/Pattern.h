#ifndef DRAMPOWER_COMMAND_PATTERN_H
#define DRAMPOWER_COMMAND_PATTERN_H

#include <DRAMPower/command/Command.h>

#include <bitset>
#include <cassert>

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

enum class PatternEncoderLastBit
{
    L = 0,
    H = 1,
    LAST_BIT = 2
};

struct PatternEncoderSettings
{
    PatternEncoderLastBit V;
    PatternEncoderLastBit X;    
};

// TODO: Has to be standard specific
class PatternEncoder // Currently LPDDR4
{
private:
    PatternEncoderSettings settings;
public:
    PatternEncoder(PatternEncoderSettings settings)
        : settings(settings) {};
    uint64_t encode(const Command& cmd, const std::vector<pattern_descriptor::t>& pattern, const uint64_t lastpattern)
    {
        using namespace pattern_descriptor;

        std::bitset<64> bitset(0);
        std::bitset<32> bank_bits(cmd.targetCoordinate.bank);
        std::bitset<32> row_bits(cmd.targetCoordinate.row);
        std::bitset<32> column_bits(cmd.targetCoordinate.column);
        std::bitset<32> bank_group_bits(cmd.targetCoordinate.bankGroup);

        std::size_t n = pattern.size() - 1;

        assert(n < 64);

        for (const auto descriptor : pattern) {
            assert(n >= 0);

            switch (descriptor) {
            case H:
                bitset[n] = true;
                break;
            case L:
                bitset[n] = false;
                break;
            case V:
                switch(settings.V)
                {
                case PatternEncoderLastBit::L:
                    bitset[n] = false;
                    break;
                case PatternEncoderLastBit::H:
                    bitset[n] = true;
                    break;
                case PatternEncoderLastBit::LAST_BIT:
                    bitset[n] = (lastpattern >> n) & 1;
                    break;
                }
                break;
            case X:
                switch(settings.X)
                {
                case PatternEncoderLastBit::L:
                    bitset[n] = false;
                    break;
                case PatternEncoderLastBit::H:
                    bitset[n] = true;
                    break;
                case PatternEncoderLastBit::LAST_BIT:
                    bitset[n] = (lastpattern >> n) & 1;
                    break;
                }
                break;
            case AP:
                bitset[n] = false;
                break; // ToDo: Variabel machen
            case BL:
                bitset[n] = true;
                break; // ToDo: Variabel machen

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

            // Column bits
            case C0: // For writes the first 4 bits are always 0, see: Standard
                bitset[n] = false;
                break;
            case C1:
                bitset[n] = false;
                break;
            case C2:
                bitset[n] = false;
                break;
            case C3:
                bitset[n] = false;
                break;
            case C4:
                bitset[n] = column_bits[4];
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
                bitset[n] = column_bits[10];
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
        };

        return bitset.to_ullong();
    };
};

}

#endif /* DRAMPOWER_COMMAND_PATTERN_H */
