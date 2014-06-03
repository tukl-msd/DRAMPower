/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#ifndef TOOLS_MEM_ARCHITECTURE_SPEC_H
#define TOOLS_MEM_ARCHITECTURE_SPEC_H

#include "Parametrisable.h"

namespace Data {

  class MemArchitectureSpec : public virtual Parametrisable {
  public:
    void processParameters();

    unsigned int burstLength;
    unsigned nbrOfBanks;
	unsigned nbrOfRanks;
    unsigned dataRate;
    unsigned nbrOfColumns;
    unsigned nbrOfRows;
    unsigned width;
    unsigned nbrOfBankGroups;
  };
}
#endif
