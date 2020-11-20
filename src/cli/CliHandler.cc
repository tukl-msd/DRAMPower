/*
 * Copyright (c) 2012-2020, TU Delft
 * Copyright (c) 2012-2020, TU Eindhoven
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
 * Authors: Subash Kannoth
 *          Luiza Correa
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
    debug_file_active(false),
    debug_console_active(false)
{
}

CliHandler::~CliHandler()
{
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


void CliHandler::parse_arguments()
{
    app = new CLI::App("DRAMPower");

    try {
        app->add_flag(IO_TERM,
                      io_term_active,
                      "IO and Termination");

        app->add_flag(DEBUG_FILE,
                      debug_file_active,
                      "Generate debug file");

        app->add_flag(DEBUG_CONSOLE,
                      debug_console_active,
                      "Display debug messages on Console");

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

        app->parse(argc, argv);

    } catch (const CLI::ParseError &e) {
        app->exit(e);
        std::exit(EXIT_FAILURE);
    } catch (const std::exception &e) {
        throw e;
    } catch (...) {
        throw std::runtime_error("Unknown error during arguments parsing!");
    }
}

bool CliHandler::get_writeToConsole() const
{
    return debug_console_active;
}


bool CliHandler::get_writeToFile() const
{
    return debug_file_active;
}


bool CliHandler::get_io_term_active() const
{
    return io_term_active;
}


const std::string& CliHandler::get_mem_spec_path() const
{
    return mem_spec_path;
}

const std::string& CliHandler::get_cmd_trace_path() const
{
    return cmd_trace_path;
}

void CliHandler::loadMemSpec(const std::string &memspecUri)
{
    json doc = traceparser.parseJSON(memspecUri);
    json jMemSpec = doc["memspec"];

    std::string memoryType = jMemSpec["memoryType"];

    if (memoryType == "DDR3") {
        MemSpecDDR3 memSpecDDR3(jMemSpec);
        dramPower = new DRAMPowerDDR3(memSpecDDR3,get_io_term_active(),
                                     (get_writeToConsole() | get_writeToFile()),
                                     get_writeToConsole(),
                                     get_writeToFile(),
                                     "DebugFile");
    }
    else if (memoryType == "DDR4") {
        MemSpecDDR4 memSpecDDR4(jMemSpec);

        dramPower = new DRAMPowerDDR4(memSpecDDR4,get_io_term_active(),
                                     (get_writeToConsole() | get_writeToFile()),
                                     get_writeToConsole(),
                                     get_writeToFile(),
                                     "DebugFile");
    }
    else if (memoryType == "WIDEIO_SDR") {

        MemSpecWideIO memSpecWideIO(jMemSpec);

        dramPower = new DRAMPowerWideIO(memSpecWideIO, get_io_term_active(),
                                       (get_writeToConsole() | get_writeToFile()),
                                       get_writeToConsole(),
                                       get_writeToFile(),
                                       "DebugFile");
    }
    //    FUTURE WORK:
    //
    //    else if (memoryType == "LPDDR4")
    //        memSpec = new MemSpecLPDDR4(jMemSpec);
    //    else if (memoryType == "WIDEIO2")
    //        memSpec = new MemSpecWideIO2(jMemSpec);
    //    else if (memoryType == "HBM2")
    //        memSpec = new MemSpecHBM2(jMemSpec);
    //    else if (memoryType == "GDDR5")
    //        memSpec = new MemSpecGDDR5(jMemSpec);
    //    else if (memoryType == "GDDR5X")
    //        memSpec = new MemSpecGDDR5X(jMemSpec);
    //    else if (memoryType == "GDDR6")
    //        memSpec = new MemSpecGDDR6(jMemSpec);
    else
        throw std::runtime_error("Unsupported DRAM type!");
}


void CliHandler::run_simulation()
{
    traceparser = TraceParser();
    loadMemSpec(get_mem_spec_path());
    const clock_t begin_time = clock();
    ifstream trace_file;
    trace_file.open(get_cmd_trace_path(), ifstream::in);
    time_t start   = time(0);
    tm*    starttm = localtime(&start);
    cout << "* Analysis start time: " << asctime(starttm);
    cout << "* Analyzing the input trace" << endl;
    cout << "* Bankwise mode: ";

    // Calculates average power consumption and energy for the input memory
    // command trace
    cmd_list = traceparser.parseFile(trace_file);

    dramPower->cmdList = cmd_list;

    dramPower->calcEnergy();

    dramPower->powerPrint();

    time_t end   = time(0);
    tm*    endtm = localtime(&end);
    cout << "* Power Computation End time: " << asctime(endtm);
    cout << "* Total Simulation time: "
         << float(clock() - begin_time) / CLOCKS_PER_SEC << " seconds" << endl;
}
