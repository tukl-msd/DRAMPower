/*
 * Copyright (c) 2012-2014, TU Delft
 * Copyright (c) 2012-2014, TU Eindhoven
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
 * Authors: Karthik Chandrasekar,
 *          Matthias Jung,
 *          Omar Naji,
 *          Sven Goossens,
 *          Éder F. Zulian
 *          Subash Kannoth
 *          Felipe S. Prado
 *          Luiza Correa
 */
#ifndef COUNTERSDDR4_H
#define COUNTERSDDR4_H

#include <stdint.h>

#include <vector>
#include <iostream>
#include <deque>
#include <string>

#include "../MemCommand.h"
#include "../memspec/MemSpecDDR4.h"
#include "Counters.h"

namespace DRAMPower {
class CountersDDR4 final : public Counters {
 public:

  CountersDDR4(MemSpecDDR4& memspec);

  MemSpecDDR4 memSpec;
  void getCommands(std::vector<MemCommand>& list,
                                 bool lastupdate,
                     int64_t timestamp) override;


  // Handlers for commands that are getting processed
  void handleRef(    unsigned bank, int64_t timestamp) override;
  void handlePupAct( int64_t timestamp) override;
  void handlePupPre( int64_t timestamp) override;
  void handleSREx(   unsigned bank, int64_t timestamp) override;
  void handleNopEnd( int64_t timestamp) override;


  // To update idle period information whenever active cycles may be idle
  void idle_act_update(int64_t                     latest_read_cycle,
                       int64_t                     latest_write_cycle,
                       int64_t                     latest_act_cycle,
                       int64_t                     timestamp) override;

  // To update idle period information whenever precharged cycles may be idle
  void idle_pre_update(int64_t                     timestamp,
                       int64_t                     latest_pre_cycle) override;


};
}

#endif // COUNTERSDDR4_H
