/*
 * Copyright (c) 2012-2014, TU Delft
 * Copyright (c) 2012-2014, TU Eindhoven
 * Copyright (c) 2012-2014, TU Kaiserslautern
 * Copyright (c) 2012-2019, Fraunhofer IESE
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
 * Authors: Subash Kannoth
 *
 */
#ifndef CLI_HANDLER_HPP
#define CLI_HANLDER_HPP

#include <iostream>
#include <string>
#include <exception>
#include <fstream>
#include "MemorySpecification.h"
#include "MemoryPowerModel.h"
#include "MemBankWiseParams.h"
#include "xmlparser/MemSpecParser.h"
#include "TraceParser.h"
#include "common/libraries/cli11.h"
#include "common/version.hpp"

namespace DRAMPower {

constexpr static const char* MEM_SPEC("--mem_spec,-m");
constexpr static const char* IO_TERM("--io_term,-r");
constexpr static const char* CMD_TRACE("--command_trace,-c");
constexpr static const char* BANK_WISE("--bank_wise,-b");
constexpr static const char* PASR_MODE("--pasr,-s");

class CliHandler {
public:
  bool get_io_term_active() const;
  const std::string& get_mem_spec_path() const;
  const std::string& get_cmd_trace_path() const;
  bool get_bank_wise_active() const;
  int get_bank_wise_rho() const;
  int get_bank_wise_sigma() const;
  bool get_pasr_active() const;
  int get_pasr_mode() const ;

  void parse_arguments();
  void run_simulation();

  CliHandler(int _argc, char** _argv); 
  ~CliHandler();
private:
  CliHandler() {}
  CLI::App* app;

  int argc;
  char** argv;
  bool io_term_active;
  std::string mem_spec_path;
  std::string cmd_trace_path;
  std::vector<int> bank_wise_parms;
  bool bank_wise_active;
  int pasr_mode;
  bool pasr_active;
};

}
#endif