/*
 * Copyright (c) 2012-2020, TU Delft
 * Copyright (c) 2012-2020, TU Eindhoven
 * Copyright (c) 2012-2020, TU Kaiserslautern
 * Copyright (c) 2012-2020, Fraunhofer IESE
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Karthik Chandrasekar
 *
 */

#ifndef MEMCOMMAND_H
#define MEMCOMMAND_H

#include <stdint.h>
#include <cassert>
#include <string>


namespace DRAMPower {
class MemCommand {
public:
    /*
   * 1. ACT - Activate
   * 2. RD - Read
   * 3. WR - Write
   * 4. PRE - Explicit Precharge per bank
   * 5. REF - Refresh all banks
   * 6  REFB- Refresh a particular bank
   * 7. END - To indicate end of trace
   * 8. RDA - Read with auto-precharge
   * 9. WRA - Write with auto-precharge
   * 10. PREA - Precharge all banks
   * 11. PDN_F_PRE - Precharge Power-down Entry command (Fast-Exit)
   * 12. PDN_S_PRE - Precharge Power-down Entry command (Slow-Exit)
   * 13. PDN_F_ACT - Active Power-down Entry command (Fast-Exit)
   * 14. PDN_S_ACT - Active Power-down Entry command (Slow-Exit)
   * 15. PUP_PRE - Precharge Power-down Exit
   * 16. PUP_ACT - Active Power-down Exit
   * 17. SREN - Self-Refresh Entry command
   * 18. SREX - Self-refresh Exit
   * 19. NOP - To indicate end of trace
   */

    enum cmds {
        ACT       = 0,
        RD        = 1,
        WR        = 2,
        PRE       = 3,
        REF       = 4,
        REFB      = 5,
        END       = 6,
        RDA       = 7,
        WRA       = 8,
        PREA      = 9,
        PDN_F_PRE = 10,
        PDN_S_PRE = 11,
        PDN_F_ACT = 12,
        PDN_S_ACT = 13,
        PUP_PRE   = 14,
        PUP_ACT   = 15,
        SREN      = 16,
        SREX      = 17,
        NOP       = 18,
        UNINITIALIZED = 19
    };

    static std::string* getCommandTypeStrings();

    //  MemCommand();
    MemCommand(// Command Issue Timestamp (in cc)
               int64_t           timestamp = 0L,
               // Command Type
               MemCommand::cmds type = UNINITIALIZED,
               //Command Rank
               unsigned          rank = 0,
               // Target Bank
               unsigned          bank = 0);



    // Set command type
    void setType(MemCommand::cmds _type);
    // Set target Bank
    void setBank(unsigned _bank);
    // Set timestamp
    void setTime(int64_t _timestamp);
    // Set target Rank
    void setRank(unsigned _rank);

    // Get command type
    cmds getType() const;
    // Get target Bank
    unsigned getBank() const;
    // Get timestamp
    int64_t getTimeInt64() const;
    // Get target Rank
    unsigned getRank() const;

    cmds typeWithoutAutoPrechargeFlag() const;

    bool operator==(const MemCommand& other) const;

    static const unsigned int nCommands = 20;

    static cmds getTypeFromName(const std::string& name);

private:
    int64_t timestamp;
    MemCommand::cmds type;
    unsigned rank;
    unsigned bank;
};
}
#endif // ifndef MEMCOMMAND_H