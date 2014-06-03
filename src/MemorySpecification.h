/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#ifndef TOOLS_MEMORY_SPECIFICATION_H
#define TOOLS_MEMORY_SPECIFICATION_H

#include "MemArchitectureSpec.h"
#include "MemTimingSpec.h"
#include "MemPowerSpec.h"
#include "Parametrisable.h"

namespace Data {
  class MemorySpecification : public virtual Parametrisable {
  public:

    //Supported memory types
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

    static std::string* getMemoryTypeStrings() {
        static std::string type_map[nMemoryTypes] = {"DDR2", "DDR3", "DDR4", "LPDDR",
												   "LPDDR2", "LPDDR3", "WIDEIO_SDR"};
        return type_map;
    }

    //To identify memory type from name
    static MemoryType getMemoryTypeFromName(const std::string& name){
      std::string* typeStrings = getMemoryTypeStrings();
      for(size_t typeId = 0; typeId < nMemoryTypes; typeId++){
        if(typeStrings[typeId] == name){
          MemoryType memoryType = static_cast<MemoryType>(typeId);
          return memoryType;
        }
      }
      assert(false);  // Unknown name.
    }

    std::string id;
    MemoryType memoryType;

    MemArchitectureSpec memArchSpec;
    MemTimingSpec memTimingSpec;
    MemPowerSpec memPowerSpec;

    void processParameters();

    static MemorySpecification getMemSpecFromXML(const std::string& id);
  };
}
#endif
