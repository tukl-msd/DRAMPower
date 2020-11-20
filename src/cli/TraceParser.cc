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
 * Authors: Omar Naji
 *          Luiza Correa
 */

#include "TraceParser.h"

using namespace DRAMPower;
using namespace std;


DRAMPower::MemCommand TraceParser::parseLine(std::string line)
{
    MemCommand memcmd(0, MemCommand::UNINITIALIZED, 0, 0);
    istringstream linestream(line);
    string item;
    int64_t item_val;
    unsigned itemnum = 0;

    while (getline(linestream, item, ',')) {
        if (itemnum == 0) {
            stringstream timestamp(item);
            timestamp >> item_val;
            memcmd.setTime(item_val);
        } else if (itemnum == 1) {
            item_val = MemCommand::getTypeFromName(item);
            memcmd.setType(static_cast<MemCommand::cmds>(item_val));
        } else if (itemnum == 2) {
            stringstream rank(item);
            rank >> item_val;
            memcmd.setRank(static_cast<unsigned>(item_val));
        }
        else if (itemnum == 3) {
            stringstream bank(item);
            bank >> item_val;
            memcmd.setBank(static_cast<unsigned>(item_val));
        }
        itemnum++;
    }
    return memcmd;
} // TraceParser::parseLine

std::vector<MemCommand> TraceParser::parseFile(std::ifstream& trace)
{
    ifstream pwr_trace;

    std::string line;
    while (getline(trace, line)) {
        MemCommand cmdline = parseLine(line);
        cmd_list.push_back(cmdline);
    }

    trace.close();
    return cmd_list;
} // TraceParser::parseFile



json TraceParser::parseJSON(const std::string &path) const
{
    json j;

    try {
        // parsing input with a syntax error
        j = json::parse(std::ifstream(path));
    }
    catch (json::parse_error& e) {
        // output exception information
        std::cout << "Error while trying to parse file: " << path << '\n'
                  << "message: " << e.what() << std::endl;
    }
    return j;
}