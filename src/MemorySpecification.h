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

#ifndef TOOLS_MEMORY_SPECIFICATION_H
#define TOOLS_MEMORY_SPECIFICATION_H

#include <cassert>

#include "MemArchitectureSpec.h"
#include "MemTimingSpec.h"
#include "MemPowerSpec.h"
#include "Parametrisable.h"

namespace Data {
class MemorySpecification : public virtual Parametrisable {
 public:
  // Supported memory types
  enum MemoryType {
    DDR2,
    DDR3,
    DDR4,
    LPDDR,
    LPDDR2,
    LPDDR3,
    WIDEIO_SDR
  };

  static const unsigned int nMemoryTypes = 7;

  static std::string* getMemoryTypeStrings()
  {
    static std::string type_map[nMemoryTypes] = { "DDR2",   "DDR3",   "DDR4", "LPDDR",
                                                  "LPDDR2", "LPDDR3", "WIDEIO_SDR" };

    return type_map;
  }

  // To identify memory type from name
  static MemoryType getMemoryTypeFromName(const std::string& name)
  {
    std::string* typeStrings = getMemoryTypeStrings();

    for (size_t typeId = 0; typeId < nMemoryTypes; typeId++) {
      if (typeStrings[typeId] == name) {
        MemoryType memoryType = static_cast<MemoryType>(typeId);
        return memoryType;
      }
    }
    assert(false); // Unknown name.
  }

  std::string id;
  MemoryType  memoryType;

  MemArchitectureSpec memArchSpec;
  MemTimingSpec memTimingSpec;
  MemPowerSpec  memPowerSpec;

  void processParameters();

  static MemorySpecification getMemSpecFromXML(const std::string& id);
};
}
#endif // ifndef TOOLS_MEMORY_SPECIFICATION_H
