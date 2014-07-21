/*
*Copyright (c) 2012, TU Delft, TU Eindhoven and TU Kaiserslautern 
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

#include <fstream>          
#include <algorithm>        
#include <sstream>          
                            
#include "CommandAnalysis.h"
#include "CmdScheduler.h"   
#include "TraceParser.h"
                            
using namespace Data;       
using namespace std;        

Data::MemCommand TraceParser::parseLine(std::string line) {
    
    MemCommand memcmd;
    istringstream linestream(line);                                                 
    string item;                                                                    
    double item_val;                                                                
    unsigned itemnum = 0;
    MemCommand::cmds type;                                                              
    while (getline(linestream, item, ',')) {                                        
        if (itemnum == 0) {                                                         
            stringstream timestamp(item);                                           
            timestamp >> item_val;                                                  
            memcmd.setTime(item_val);                                  
        } else if (itemnum == 1) {                                                  
            item_val = MemCommand::getTypeFromName(item);                           
            memcmd.setType(static_cast<MemCommand::cmds> (item_val));  
        } else if (itemnum == 2) {                                                  
            stringstream bank(item);                                                
            bank >> item_val;                                                       
            memcmd.setType(type);                                      
            memcmd.setBank(static_cast<unsigned>(item_val));           
        }                                                                           
        type = memcmd.getType();                                       
        itemnum++;                                                                  
    } 
    return memcmd;                                                                              
}

void TraceParser::parseFile (MemorySpecification memSpec, std::ifstream& trace, int grouping, 
                       int interleaving, int burst, int powerdown, int trans) {

    ifstream pwr_trace;
    if(trans){                                                                        
        cmdScheduler cmdsched;                                                        
        cmdsched.transTranslation(memSpec, trace, grouping, interleaving, burst, powerdown); 
        pwr_trace.open("commands.trace", ifstream::in);
        std::string line;
        while (getline(pwr_trace, line)) {
        MemCommand cmdline = parseLine(line);
        cmd_list.push_back(cmdline);
        }
        pwr_trace.close();                                
    }
    else {                                                                                 
        std::string line;
        while (getline(trace, line)) { 
            MemCommand cmdline = parseLine(line);
            cmd_list.push_back(cmdline);
        }
    }
    trace.close();
}
