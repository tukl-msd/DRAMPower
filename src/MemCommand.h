/*
 * Copyright (c) 2012-2014, TU Delft
 * Copyright (c) 2012-2014, TU Eindhoven
 * Copyright (c) 2012-2014, TU Kaiserslautern
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

#include "MemorySpecification.h"

namespace Data {
class MemCommand {
 public:
  /*
   * ACTB - Activate (RGR)
   * ACT - Activate
   * RD - Read
   * WR - Write
   * PREB - Explicit (RGR Precharge per bank)
   * PRE - Explicit Precharge per bank
   * REF - Refresh all banks
   * REFB- Refresh a particular bank
   * END - To indicate end of trace
   * RDA - Read with auto-precharge
   * WRA - Write with auto-precharge
   * PREA - Precharge all banks
   * PDN_F_PRE - Precharge Power-down Entry command (Fast-Exit)
   * PDN_S_PRE - Precharge Power-down Entry command (Slow-Exit)
   * PDN_F_ACT - Active Power-down Entry command (Fast-Exit)
   * PDN_S_ACT - Active Power-down Entry command (Slow-Exit)
   * PUP_PRE - Precharge Power-down Exit
   * PUP_ACT - Active Power-down Exit
   * SREN - Self-Refresh Entry command
   * SREX - Self-refresh Exit
   * NOP - To indicate end of trace
   */

  enum cmds {
    ACTB      ,
    ACT       ,
    RD        ,
    WR        ,
    PREB      ,
    PRE       ,
    REF       ,
    REFB      ,
    END       ,
    RDA       ,
    WRA       ,
    PREA      ,
    PDN_F_PRE ,
    PDN_S_PRE ,
    PDN_F_ACT ,
    PDN_S_ACT ,
    PUP_PRE   ,
    PUP_ACT   ,
    SREN      ,
    SREX      ,
    NOP       ,
    UNINITIALIZED
  };

//  MemCommand();
  MemCommand(
    // Command Type
    MemCommand::cmds type = UNINITIALIZED,
    // Target Bank
    unsigned         bank = 0,
    // Command Issue Timestamp (in cc)
    int64_t          timestamp = 0L);

  // Get command type
  cmds getType() const;

  // Set command type
  void setType(MemCommand::cmds type);

  // Set target Bank
  void setBank(unsigned bank);

  // Get target Bank
  unsigned getBank() const;

  // Set timestamp
  void setTime(int64_t _timestamp);

  // Get timestamp
  int64_t getTimeInt64() const;

  cmds typeWithoutAutoPrechargeFlag() const;

  // To calculate precharge offset after read or write with auto-precharge
  int64_t getPrechargeOffset(const MemorySpecification& memSpec,
                         MemCommand::cmds           type) const;

  // To check for equivalence

  bool operator==(const MemCommand& other) const
  {
    if ((getType() == other.getType()) &&
        (getBank() == other.getBank())
        ) {
      return true;
    } else {
      return false;
    }
  }

  static const unsigned int nCommands = 22;

  static std::string* getCommandTypeStrings()
  {
    static std::string type_map[nCommands] = { "ACTB",
                                               "ACT",
                                               "RD",
                                               "WR",
                                               "PREB",
                                               "PRE",
                                               "REF",
                                               "REFB",
                                               "END",
                                               "RDA",
                                               "WRA",
                                               "PREA",
                                               "PDN_F_PRE",
                                               "PDN_S_PRE",
                                               "PDN_F_ACT",
                                               "PDN_S_ACT",
                                               "PUP_PRE",
                                               "PUP_ACT",
                                               "SREN",
                                               "SREX",
                                               "NOP",
                                               "UNINITIALIZED" };

    return type_map;
  }

  // To identify command type from name
  static cmds getTypeFromName(const std::string& name)
  {
    std::string* typeStrings = getCommandTypeStrings();

    for (size_t typeId = 0; typeId < nCommands; typeId++) {
      if (typeStrings[typeId] == name) {
        cmds commandType = static_cast<cmds>(typeId);
        return commandType;
      }
    }
    assert(false); // Unknown name.
    return NOP;  // For clang compilation
  }

 private:
  MemCommand::cmds type;
  unsigned bank;
  int64_t timestamp;
};
}
#endif // ifndef MEMCOMMAND_H
