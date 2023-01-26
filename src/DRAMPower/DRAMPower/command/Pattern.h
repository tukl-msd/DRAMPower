#ifndef DRAMPOWER_COMMAND_PATTERN_H
#define DRAMPOWER_COMMAND_PATTERN_H

#include <DRAMPower/command/Command.h>

#include <bitset>
#include <cassert>

namespace DRAMPower {
    namespace pattern_descriptor {
        enum t {
            H,L,V,X,
            A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14,A15,A16,A17,
            BG0,BG1,BG2,
            BA0,BA1,BA2,BA3,BA4,BA5,BA6,BA7,BA8,
            C0,C1,C2,C3,C4,C5,C6,C7,C8,C9,C10,C11,C12,C13,C14,C15,C16,
            R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,R13,R14,R15,R16,R17,R18,R19,R20,R21,R22,R23,
            CID0,CID1,CID2,CID3,
            AP,BL,
        };
    }


    class PatternEncoder {
    private:
    public:
        virtual uint64_t encode(const Command &cmd, const std::vector<pattern_descriptor::t> &pattern) const = 0;
    };

}

#endif /* DRAMPOWER_COMMAND_PATTERN_H */
