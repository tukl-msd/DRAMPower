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
 * Authors: Karthik Chandrasekar, Omar Naji, Subash Kannoth, Eder Zulian, Matthias Jung
 *
 */
#include <ctime>
#include <cmath>

#include <iostream>
#include <fstream>

#include "MemorySpecification.h"
#include "MemoryPowerModel.h"
#include "MemBankWiseParams.h"
#include "xmlparser/MemSpecParser.h"
#include "TraceParser.h"

using namespace Data;
using namespace std;

int error()
{
  cout << "Correct Usage: \n./drampower -m <memory spec (ID)> " <<
               "[-t] <transactions trace> " <<
               "[-c] <commands trace> " <<
               "[-i] <interleaving> " <<
               "[-g] <DDR4 bank group interleaving> " <<
               "[-s] <request size> " <<
               "[-r] " <<
               "[-p] <1 - Power-Down, 2 - Self-Refresh> " <<
               "[-b] <ρ - ACT Standby bankwise power offset factor (0-100), σ - Self-Refresh bankwise power offset factor (0-100)> "<<
               "[-pasr] <Partial Array Self-Refresh mode (0 - 7)> \n";
  return 1;
}

int main(int argc, char* argv[])
{
  int trans = 0, cmds = 0, memory = 0, size = 0, term = 0, power_down = 0;
  bool bankwiseMode = false;
  bool bankPASRact = false;
  unsigned bankwisePowerFactRho = 100;
  unsigned bankwisePowerFactSigma = 100;
  unsigned pasrMode = 0;

  char*    src_trans    = { 0 };
  char*    src_cmds     = { 0 };
  char*    src_memory   = { 0 };

  int interleaving = 1, grouping = 1, src_size = 1, burst = 1;

  for (int i = 1; i < argc; i++) {
    if (i + 1 != argc) {
      if (string(argv[i]) == "-t") {
        src_trans = argv[i + 1];
        trans     = 1;
      } else if (string(argv[i]) == "-c") {
        src_cmds = argv[i + 1];
        cmds     = 1;
      } else if (string(argv[i]) == "-m") {
        src_memory = argv[i + 1];
        memory     = 1;
      } else if (string(argv[i]) == "-i") {
        interleaving = atoi(argv[i + 1]);
      } else if (string(argv[i]) == "-g") {
        grouping = atoi(argv[i + 1]);
      } else if (string(argv[i]) == "-s") {
        src_size = atoi(argv[i + 1]);
        size     = 1;
      } else if (string(argv[i]) == "-p") {
        power_down = atoi(argv[i + 1]);
      } else if (string(argv[i]) == "-b") {
        string bwTuple = argv[i + 1];
        size_t idx;
        bankwiseMode = true;
        try{
            idx = bwTuple.find(",");
            bankwisePowerFactRho = static_cast<unsigned>(stoi(bwTuple.substr(0, idx)));
            bwTuple.erase(0, idx +  1);
            idx = bwTuple.find(",");
            bankwisePowerFactSigma = static_cast<unsigned>(stoi(bwTuple.substr(0, idx)));
        }catch(const std::exception& e) {
            return error();
        }
      } else if (string(argv[i]) == "-pasr") {
          pasrMode = static_cast<unsigned>(atoi(argv[i + 1]));
          bankPASRact = true;
      } else {
        if (string(argv[i]) == "-r") {
          term = 1;
        }
        continue;
      }
    } else {
      if (string(argv[i]) == "-r") {
        term = 1;
      }
      continue;
    }
  }

  if (bankwisePowerFactRho > 100) {
      cout << endl << "ACT Standby bankwise power offset factor ρ out of range." << endl;
      return error();
  }

  if (bankwisePowerFactSigma > 100) {
      cout << endl << "Self-Refresh bankwise power offset factor σ out of range." << endl;
      return error();
  }

  if (pasrMode > 7) {
      cout << endl << "Wrong PASR mode." << endl;
      return error();
  }

  if (memory == 0) {
    cout << endl << "No DRAM memory specified!" << endl;
    return error();
  }

  ifstream fout;
  if (trans) {
    fout.open(src_trans);
    if (fout.fail()) {
      cout << "Transactions trace file not found!" << endl;
      return error();
    }
  } else   {
    fout.open(src_cmds);
    if (fout.fail()) {
      cout << "Commands trace file not found!" << endl;
      return error();
    }
  }
  fout.close();

  // Replace the memory specification XML file with another in the same format
  // from the memspecs folder
  MemorySpecification  memSpec(MemSpecParser::getMemSpecFromXML(src_memory));

  MemArchitectureSpec& memArchSpec = memSpec.memArchSpec;
  MemBankWiseParams memBwParams(bankwisePowerFactRho, bankwisePowerFactSigma,
                                bankPASRact, pasrMode, bankwiseMode,
                                (unsigned)memArchSpec.nbrOfBanks);

  if ((memArchSpec.twoVoltageDomains) && (bankwiseMode)){
      cout << endl << "Bankwise simulation for Two-Voltage domain devices not supported." << endl;
      return error();
  }

  if (interleaving > memArchSpec.nbrOfBanks) {
    cout << "Interleaving > Number of Banks" << endl;
    return error();
  }

  if (grouping > memArchSpec.nbrOfBankGroups) {
    cout << "Grouping > Number of Bank Groups" << endl;
    return error();
  }

  if (power_down > 2) {
    cout << "Incorrect power-down option" << endl;
    return error();
  }

  if ((!bankwiseMode) && (bankPASRact)){
      cout<<endl<<"PASR cannot be activated when Bankwise mode is disabled."<<endl;
      return error();
  }

  int min_size = static_cast<int>(interleaving * grouping * memArchSpec.burstLength * memArchSpec.width / 8);

  if (size == 0) {
    src_size = min_size;
  } else {
    src_size = max(min_size, src_size);
  }

  burst = src_size / min_size;
  // transSize = BGI * BI * BC * BL.

  const clock_t begin_time = clock();

  ifstream trace_file;

  if (trans) {
    trace_file.open(src_trans, ifstream::in);
  } else if (cmds) {
    trace_file.open(src_cmds, ifstream::in);
  } else {
    cout << "No transaction or command trace file specified!" << endl;
    return error();
  }

  MemoryPowerModel mpm = MemoryPowerModel();

  time_t start   = time(0);
  tm*    starttm = localtime(&start);
  cout << "* Analysis start time: " << asctime(starttm);
  cout << "* Analyzing the input trace" << endl;
  cout << "* Bankwise mode: ";
  if (bankwiseMode) {
    cout << "enabled (power offset factors  ρ=" << bankwisePowerFactRho << "% ,σ="<<bankwisePowerFactSigma<<"% )"<<endl;
  } else {
    cout << "disabled" << endl;
  }
  cout << "* Partial Array Self-Refresh: ";
  if (bankPASRact){
      cout<<"enabled";
  }else{
       cout << "disabled" << endl;
  }
  // Calculates average power consumption and energy for the input memory
  // command trace
  const int CMD_ANALYSIS_WINDOW_SIZE = 1000000;
  TraceParser traceparser(memSpec);
  traceparser.parseFile(memSpec, trace_file, CMD_ANALYSIS_WINDOW_SIZE, grouping, interleaving, burst, power_down, trans);
  mpm.power_calc(memSpec, traceparser.counters, term, memBwParams);

  mpm.power_print(memSpec, term, traceparser.counters, bankwiseMode);
  time_t end   = time(0);
  tm*    endtm = localtime(&end);
  cout << "* Power Computation End time: " << asctime(endtm);

  cout << "* Total Simulation time: " << float(clock() - begin_time) /
    CLOCKS_PER_SEC << " seconds" << endl;

  return 0;
} // main
