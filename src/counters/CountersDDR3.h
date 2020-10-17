#ifndef COUNTERSDDR3_H
#define COUNTERSDDR3_H

#include <stdint.h>

#include <vector>
#include <iostream>
#include <deque>
#include <string>

#include "MemCommand.h"
#include "Utils.h"
#include "./memspec/MemSpecDDR3.h"
#include "Counters.h"

namespace DRAMPower {
class CountersDDR3 final : public Counters {
 public:

  CountersDDR3(MemSpecDDR3& memspec);

  MemSpecDDR3 memSpec;
  void getCommands(std::vector<MemCommand>& list,
                             bool lastupdate,
                             int64_t timestamp);


  // Handlers for commands that are getting processed

  void handleRef(    unsigned bank, int64_t timestamp);
  void handlePupAct( int64_t timestamp);
  void handlePupPre( int64_t timestamp);
  void handleSREx(   unsigned bank, int64_t timestamp);
  void handleNopEnd( int64_t timestamp);


  // To update idle period information whenever active cycles may be idle
  void idle_act_update(int64_t                     latest_read_cycle,
                       int64_t                     latest_write_cycle,
                       int64_t                     latest_act_cycle,
                       int64_t                     timestamp);

  // To update idle period information whenever precharged cycles may be idle
  void idle_pre_update(int64_t                     timestamp,
                       int64_t                     latest_pre_cycle);


};
}

#endif // COUNTERSDDR3_H