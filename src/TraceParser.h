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
 * Authors: Omar Naji
 *
 */

#ifndef TRACE_PARSER_H
#define TRACE_PARSER_H

#include <vector>
#include <string>
#include <fstream>

#include "MemCommand.h"
#include "MemorySpecification.h"
#include "CommandAnalysis.h"


class TraceParser {
 public:
  TraceParser(const DRAMPower::MemorySpecification& memSpec);
  // list of parsed commands
  std::vector<DRAMPower::MemCommand> cmd_list;

  // function for parsing one line of the trace
  DRAMPower::MemCommand parseLine(std::string line);

  DRAMPower::CommandAnalysis counters;
  // function for parsing the whole file.
  // use this function for small traces ( no out-of-memory issue )
  void parseFile(DRAMPower::MemorySpecification memSpec,
                 std::ifstream&      trace,
                 int                 window);
};

#endif // ifndef TRACE_PARSER_H
