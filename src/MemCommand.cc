/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include "MemCommand.h"
#include "MemorySpecification.h"

using namespace Data;
using namespace std;

MemCommand::MemCommand():
  type(MemCommand::PRE),
  bank(0),
  timestamp(0) {
}

//typeBank initilization
MemCommand::MemCommand(MemCommand::cmds type,
                       unsigned bank, double timestamp):
  type(type),
  bank(bank),
  timestamp(timestamp){
}

void MemCommand::setType(MemCommand::cmds _type) {
  type = _type;
}

MemCommand::cmds MemCommand::getType() const {
    return type;
}


void MemCommand::setBank(unsigned _bank) {
  bank = _bank;
}

unsigned MemCommand::getBank() const {
    return bank;
}

//For auto-precharge with read or write - to calculate cycle of precharge
int MemCommand::getPrechargeOffset(const MemorySpecification& memSpec,
                                                          MemCommand::cmds type)
{
  int precharge_offset = 0;

  int BL(static_cast<int>(memSpec.memArchSpec.burstLength));
  int RTP(static_cast<int>(memSpec.memTimingSpec.RTP));
  int dataRate(static_cast<int>(memSpec.memArchSpec.dataRate));
  int AL(static_cast<int>(memSpec.memTimingSpec.AL));
  int WL(static_cast<int>(memSpec.memTimingSpec.WL));
  int WR(static_cast<int>(memSpec.memTimingSpec.WR));

  //Read with auto-precharge
  if(type == MemCommand::RDA)
  {
      if(memSpec.memoryType == MemorySpecification::DDR2)
        precharge_offset = AL + BL/dataRate + max(RTP, 2) - 2;
      else
        precharge_offset = RTP;
  }
  else if(type == MemCommand::WRA)//Write with auto-precharge
  {
        precharge_offset = WL + BL/dataRate + WR;
  }

  return precharge_offset;
}

void MemCommand::setTime(double _timestamp) {
  timestamp = _timestamp;
}

double MemCommand::getTime() const {
    return timestamp;
}
