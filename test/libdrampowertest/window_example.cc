/*
 * Copyright (c) 2014, TU Delft, TU Eindhoven and TU Kaiserslautern
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
 * Authors: Matthias Jung, Omar Naji, Felipe S. Prado
 *
 */

// This example shows how the window feature of DRAMPower library
// can be used in simulators like gem5, DRAMSys, etc.

#include <iostream>
#include <string>
#include "libdrampower/LibDRAMPower.h"
#if USE_XERCES
    #include "xmlparser/MemSpecParser.h"
#endif


using namespace std;
using namespace Data;

int main(int argc, char* argv[])
{
    assert(argc == 2);
    //Setup of DRAMPower for your simulation
    string filename;
    //type path to memspec file
    filename = argv[1];
    //Parsing the Memspec specification of found in memspec folder
#if USE_XERCES
    MemorySpecification memSpec(MemSpecParser::getMemSpecFromXML(filename));
#else
    MemorySpecification memSpec;
#endif
    libDRAMPower test = libDRAMPower(memSpec, 0);

    int i = 0;
    unsigned long windowSize = 100;

    ios_base::fmtflags flags = cout.flags();
    streamsize precision = cout.precision();
    cout.precision(2);
    cout << fixed << endl;

    test.doCommand(MemCommand::ACT,0,10);
    test.doCommand(MemCommand::RD,0,20);
    test.doCommand(MemCommand::RD,0,30);
    test.doCommand(MemCommand::WR,0,40);
    test.doCommand(MemCommand::PRE,0,50);
    test.doCommand(MemCommand::ACT,4,60);
    test.doCommand(MemCommand::WRA,4,70);
    test.doCommand(MemCommand::PDN_F_PRE,0,90);
    //The window ended during a Precharged Power-down phase.
    test.calcWindowEnergy(++i*windowSize); //100 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::PUP_PRE,0,120);
    test.doCommand(MemCommand::ACT,0,130);
    test.doCommand(MemCommand::WR,0,140);
    test.doCommand(MemCommand::PRE,0,150);
    test.doCommand(MemCommand::PDN_S_PRE,0,180);
    test.doCommand(MemCommand::PUP_PRE,2,190);
    //The window is longer than an entire Precharged Power-down phase.
    test.calcWindowEnergy(++i*windowSize); //200 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::ACT,0,210);
    test.doCommand(MemCommand::RDA,0,220);
    test.doCommand(MemCommand::ACT,2,230);
    test.doCommand(MemCommand::WR,2,240);
    test.doCommand(MemCommand::RD,2,250);
    test.doCommand(MemCommand::RD,2,260);
    test.doCommand(MemCommand::ACT,5,270);
    test.doCommand(MemCommand::PDN_S_ACT,0,280);
    //The window ended during a Active Power-down phase.
    test.calcWindowEnergy(++i*windowSize); //300 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::PUP_ACT,0,310);
    test.doCommand(MemCommand::ACT,0,320);
    test.doCommand(MemCommand::RD,0,330);
    test.doCommand(MemCommand::PDN_F_ACT,0,340);
    test.doCommand(MemCommand::PUP_ACT,0,350);
    test.doCommand(MemCommand::RD,2,360);
    test.doCommand(MemCommand::PREA,0,390);
    test.doCommand(MemCommand::REF,0,400);
    //The window is longer than an entire Active Power-down phase.
    test.calcWindowEnergy(++i*windowSize); //400 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::PDN_S_PRE,0,430);
    test.doCommand(MemCommand::PUP_PRE,0,450);
    test.doCommand(MemCommand::SREN,0,490);
    //The window ended during a Self-refresh phase.
    test.calcWindowEnergy(++i*windowSize); //500 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    //Self-refresh cycles energy calculation (inside self-refresh)
    test.calcWindowEnergy(++i*windowSize); //600 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::SREX,0,610);
    test.doCommand(MemCommand::ACT,7,620);
    test.doCommand(MemCommand::WRA,7,630);
    test.doCommand(MemCommand::ACT,2,640);
    test.doCommand(MemCommand::RD,2,650);
    test.doCommand(MemCommand::PDN_S_ACT,0,660);
    test.doCommand(MemCommand::PUP_ACT,0,670);
    test.doCommand(MemCommand::PREA,0,680);
    test.doCommand(MemCommand::SREN,0,690);
    test.doCommand(MemCommand::SREX,0,700);
    //The window is longer than a self-refresh period and the self-refresh period is shorter than t.RFC - t.RP
    test.calcWindowEnergy(++i*windowSize); //700 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::ACT,2,720);
    test.doCommand(MemCommand::RD,2,730);
    test.doCommand(MemCommand::PRE,2,740);
    //Precharged cycles energy calculation
    test.calcWindowEnergy(++i*windowSize); //800 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::ACT,6,810);
    test.doCommand(MemCommand::PRE,6,900);
    //Active cycles energy calculation
    test.calcWindowEnergy(++i*windowSize); //900 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::REF,0,910);
    //Refresh cycles energy calculation
    test.calcWindowEnergy(++i*windowSize); //1000 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::ACT,4,1010);
    test.doCommand(MemCommand::WRA,4,1020);
    test.doCommand(MemCommand::SREN,0,1040);
    test.doCommand(MemCommand::SREX,0,1080);
    test.doCommand(MemCommand::ACT,6,1090);
    //The window is longer than a self-refresh period and the self-refresh period is longer than t.RFC
    test.calcWindowEnergy(++i*windowSize); //1100 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::PDN_F_ACT,0,1110);
    test.doCommand(MemCommand::PUP_ACT,0,1120);
    test.doCommand(MemCommand::PRE,6,1130);
    test.doCommand(MemCommand::PDN_F_PRE,0,1140);
    test.doCommand(MemCommand::PUP_PRE,0,1150);
    test.doCommand(MemCommand::ACT,5,1160);
    test.doCommand(MemCommand::WR,5,1170);
    test.doCommand(MemCommand::PREA,0,1180);
    test.doCommand(MemCommand::REF,0,1190);
    //The window ended in the middle of an auto-refresh.
    test.calcWindowEnergy(++i*windowSize); //1200 cycles
    test.doCommand(MemCommand::ACT,7,1220);
    test.doCommand(MemCommand::WRA,7,1230);
    test.doCommand(MemCommand::ACT,2,1240);
    test.doCommand(MemCommand::RD,2,1250);
    test.doCommand(MemCommand::PRE,2,1260);
    test.doCommand(MemCommand::SREN,0,1270);
    test.doCommand(MemCommand::SREX,0,1280);
    //The window is longer than a self-refresh period and the self-refresh period is shorter than t.RFC and longer than t.RFC - t.RP
    test.calcWindowEnergy(++i*windowSize); //1300 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::ACT,3,1320);
    test.doCommand(MemCommand::RD,3,1330);
    test.doCommand(MemCommand::WR,3,1340);
    test.doCommand(MemCommand::RDA,3,1360);
    test.doCommand(MemCommand::ACT,1,1390);
    test.doCommand(MemCommand::WRA,1,1400);
    //The command WRA will be divided into two commands (WR and PRE) and the PRE command will be included in the energy calculation of the next window
    test.calcWindowEnergy(++i*windowSize); //1400 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::ACT,7,1420);
    test.doCommand(MemCommand::RD,7,1430);
    test.doCommand(MemCommand::ACT,6,1440);
    test.doCommand(MemCommand::WR,6,1450);
    test.doCommand(MemCommand::ACT,0,1490);
    test.doCommand(MemCommand::RDA,0,1500);
    //The command RDA will be divided into two commands (RD and PRE) and the PRE command will be included in the energy calculation of the next window
    test.calcWindowEnergy(++i*windowSize); //1500 cycles
    test.calcEnergy();
    //Energy calculation ended in the middle of a window.
    std::cout << "Window " << ++i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " <<  i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    std::cout << "Total Trace Energy: "  <<  test.getEnergy().total_energy << " pJ" <<  endl;
    std::cout << "Average Power: " << test.getPower().average_power <<  " mW" <<  endl;

    cout.flags(flags);
    cout.precision(precision);
    
    return 0;
}


