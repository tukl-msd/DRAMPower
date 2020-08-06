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
 #include "CliHandler.h"

using namespace std;
using namespace DRAMPower;

CliHandler::CliHandler(int _argc, char** _argv):
  argc(_argc),
  argv(_argv),
  io_term_active(false),
  mem_spec_path(""),
  cmd_trace_path(""),
  bank_wise_parms({-1,-1}),
  bank_wise_active(false),
  pasr_mode(-1),
  pasr_active(false){
}

CliHandler::~CliHandler(){
  delete(app);
}

void CliHandler::logo()
{
#define BLUETXT(s)  std::string(("\u001b[38;5;20m"+std::string((s))+"\033[0m"))
#define DBLUETXT(s) std::string(("\u001b[38;5;18m"+std::string((s))+"\033[0m"))
#define LBLUETXT(s) std::string(("\u001b[38;5;14m"+std::string((s))+"\033[0m"))
#define BLACKTXT(s)  std::string(("\u001b[38;5;232m"+std::string((s))+"\033[0m"))
#define BOLDTXT(s)   std::string(("\033[1;37m"+std::string((s))+"\033[0m"))
    cout << std::endl
         << BLACKTXT("■ ■ ")<< DBLUETXT("■  ")
         << BOLDTXT("DRAMPower, Copyright (c) 2020")
         << std::endl
         << BLACKTXT("■ ") << DBLUETXT("■ ") << BLUETXT("■  ")
         << "TU Eindhoven, TU Delft, TU Kaiserslautern"
         << std::endl
         << DBLUETXT("■ ") << BLUETXT("■ ") << LBLUETXT("■  " )
         << "Fraunhofer IESE"
         << std::endl
         << std::endl;
#undef GREENTXT
#undef DGREENTXT
#undef LGREENTXT
#undef BLACKTXT
#undef BOLDTXT
}


void CliHandler::parse_arguments(){
  
  app = new CLI::App("DRAMPower");

  try {
    app->add_flag(IO_TERM,
                  io_term_active,
                  "IO and Termination");

    app->add_flag_function(VERS,
                           [&](bool){
                             logo();
                             std::exit(EXIT_SUCCESS);
                           },
                           "Display DRAMPower version information");

    app->add_option(MEM_SPEC,
                    mem_spec_path,
                    "Memory specification file")
                    ->required()
                    ->check(CLI::ExistingFile); 
    app->add_option(CMD_TRACE,
                    cmd_trace_path,
                    "Commands trace file")
                    ->required()
                    ->check(CLI::ExistingFile);
    CLI::Option* bw_option { app->add_option(BANK_WISE,
                             bank_wise_parms,
                             "Bank-wise mode \nρ - ACT Standby bankwise power offset factor\nσ - Self-Refresh bankwise power offset factor" )
                             ->expected(2)
                             ->check(CLI::Range(0,100))
    };
    app->add_option(PASR_MODE,
                    pasr_mode, 
                    "Partial Array Self-Refresh mode" )
                    ->needs(bw_option)
                    ->check(CLI::Range(0,7));

    app->parse(argc, argv);

    bank_wise_active = !((bank_wise_parms.at(0) == -1) && (bank_wise_parms.at(1) = -1));
    pasr_active = !(pasr_mode == -1);
  } catch (const CLI::ParseError &e) {
    app->exit(e);
    std::exit(EXIT_FAILURE);
  } catch (const std::exception &e) {
    throw e;
  } catch (...) {
    throw std::runtime_error("Unknown error during arguments parsing!");
  }
}

bool CliHandler::get_io_term_active() const{
  return io_term_active;
}

const std::string& CliHandler::get_mem_spec_path() const{
  return mem_spec_path;
}

const std::string& CliHandler::get_cmd_trace_path() const{
  return cmd_trace_path;
}

bool CliHandler::get_bank_wise_active() const{
  return bank_wise_active;
}

int CliHandler::get_bank_wise_rho() const{
  try{
    return bank_wise_parms.at(0);
  } catch (const std::exception& e){
    throw std::runtime_error(e.what());
  }
  return false;
}

int CliHandler::get_bank_wise_sigma() const{
  try{
    return bank_wise_parms.at(1);
  } catch (const std::exception& e){
    throw std::runtime_error(e.what());
  }
  return false;
}

bool CliHandler::get_pasr_active() const{
  return pasr_active;
}

int CliHandler::get_pasr_mode() const{
  return pasr_mode;
}

void CliHandler::run_simulation(){
  MemorySpecification  memSpec(JSONParser::readJsonFromFile(get_mem_spec_path()));
  MemArchitectureSpec& memArchSpec = memSpec.memArchSpec;
  MemBankWiseParams memBwParams(get_bank_wise_rho(), 
                                get_bank_wise_sigma(),
                                get_pasr_active(),
                                get_pasr_mode(),
                                get_bank_wise_active(),
                                (unsigned)memArchSpec.nbrOfBanks);
  MemoryPowerModel mpm = MemoryPowerModel();
  TraceParser traceparser(memSpec);
    
  if ((memArchSpec.twoVoltageDomains) && (get_bank_wise_active())){
      cout << endl << "Bankwise simulation for Two-Voltage domain devices not supported." << endl;
      std::exit(EXIT_FAILURE);
  }  
  const clock_t begin_time = clock();  
  ifstream trace_file;
  trace_file.open(get_cmd_trace_path(), ifstream::in);  
  time_t start   = time(0);
  tm*    starttm = localtime(&start);
  cout << "* Analysis start time: " << asctime(starttm);
  cout << "* Analyzing the input trace" << endl;
  cout << "* Bankwise mode: ";
  
  if (get_pasr_active()) {
    cout << "enabled (power offset factors  ρ=" << get_bank_wise_rho() 
         << "% ,σ="<<get_bank_wise_sigma()<<"% )"<<endl;
  } else {
    cout << "disabled" << endl;
  }  
  cout << "* Partial Array Self-Refresh: ";
  if (get_pasr_active()){
      cout<<"enabled";
  }else{
       cout << "disabled" << endl;
  }
  // Calculates average power consumption and energy for the input memory
  // command trace
  const int CMD_ANALYSIS_WINDOW_SIZE = 1000000;
  traceparser.parseFile(memSpec, trace_file, CMD_ANALYSIS_WINDOW_SIZE);
  mpm.power_calc(memSpec, traceparser.counters,  get_io_term_active(), memBwParams);  
  mpm.power_print(memSpec,
                  get_io_term_active(),
                  traceparser.counters,
                  get_bank_wise_active());  
  time_t end   = time(0);
  tm*    endtm = localtime(&end);
  cout << "* Power Computation End time: " << asctime(endtm);
  cout << "* Total Simulation time: " 
       << float(clock() - begin_time) / CLOCKS_PER_SEC << " seconds" << endl;
}
