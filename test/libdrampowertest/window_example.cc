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

    int64_t i = 0;
    int64_t windowSize = 200;

    ios_base::fmtflags flags = cout.flags();
    streamsize precision = cout.precision();
    cout.precision(2);
    cout << fixed << endl;

    test.doCommand(MemCommand::ACT,0,20);
    test.doCommand(MemCommand::RD,0,40);
    test.doCommand(MemCommand::RD,0,60);
    test.doCommand(MemCommand::WR,0,80);
    test.doCommand(MemCommand::PRE,0,100);
    test.doCommand(MemCommand::ACT,4,120);
    test.doCommand(MemCommand::WRA,4,140);
    test.doCommand(MemCommand::PDN_F_PRE,0,180);
    //The window ended during a Precharged Fast-exit Power-down phase.
    test.calcWindowEnergy(++i*windowSize); //200 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::PUP_PRE,0,240);
    test.doCommand(MemCommand::ACT,0,260);
    test.doCommand(MemCommand::WR,0,280);
    test.doCommand(MemCommand::PRE,0,300);
    test.doCommand(MemCommand::PDN_S_PRE,0,360);
    test.doCommand(MemCommand::PUP_PRE,2,380);
    //The window is longer than an entire Precharged Slow-exit Power-down phase.
    test.calcWindowEnergy(++i*windowSize); //400 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::ACT,0,420);
    test.doCommand(MemCommand::RDA,0,440);
    test.doCommand(MemCommand::ACT,2,460);
    test.doCommand(MemCommand::WR,2,480);
    test.doCommand(MemCommand::RD,2,500);
    test.doCommand(MemCommand::RD,2,520);
    test.doCommand(MemCommand::ACT,5,540);
    test.doCommand(MemCommand::PDN_S_ACT,0,560);
    //The window ended during a Active Slow-exit Power-down phase.
    test.calcWindowEnergy(++i*windowSize); //600 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::PUP_ACT,0,620);
    test.doCommand(MemCommand::ACT,0,640);
    test.doCommand(MemCommand::RD,0,660);
    test.doCommand(MemCommand::PDN_F_ACT,0,680);
    test.doCommand(MemCommand::PUP_ACT,0,700);
    test.doCommand(MemCommand::RD,2,720);
    test.doCommand(MemCommand::PREA,0,760);
    test.doCommand(MemCommand::REF,0,800);
    //The window is longer than an entire Active Fast-exit Power-down phase.
    test.calcWindowEnergy(++i*windowSize); //800 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::PDN_S_PRE,0,880);
    test.doCommand(MemCommand::PUP_PRE,0,900);
    test.doCommand(MemCommand::SREN,0,980);
    //The window ended during a Self-refresh phase and after a number of self-refresh cycles less than tRFC - tRP
    test.calcWindowEnergy(++i*windowSize); //1000 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    //Self-refresh cycles energy calculation (inside self-refresh)    
    test.calcWindowEnergy(++i*windowSize); //1200 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::SREX,0,1205);
    test.doCommand(MemCommand::ACT,7,1280);
    test.doCommand(MemCommand::WR,7,1300);
    test.doCommand(MemCommand::ACT,2,1320);
    test.doCommand(MemCommand::RD,2,1340);
    test.doCommand(MemCommand::PREA,0,1360);
    test.doCommand(MemCommand::SREN,0,1380);
    test.doCommand(MemCommand::SREX,0,1400);
    //The window is longer than a self-refresh period and the self-refresh period is shorter than tRFC - tRP
    test.calcWindowEnergy(++i*windowSize); //1400 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::ACT,2,1480);
    test.doCommand(MemCommand::RD,2,1500);
    test.doCommand(MemCommand::PRE,2,1520);
    //Precharged cycles energy calculation
    test.calcWindowEnergy(++i*windowSize); //1600 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::ACT,6,1620);
    test.doCommand(MemCommand::PRE,6,1800);
    //Active cycles energy calculation
    test.calcWindowEnergy(++i*windowSize); //1800 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::REF,0,1820);
    //Auto-refresh cycles energy calculation
    test.calcWindowEnergy(++i*windowSize); //2000 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::SREN,0,2020);
    test.doCommand(MemCommand::SREX,0,2100);
    test.doCommand(MemCommand::ACT,6,2180);
    //The window is longer than a self-refresh period and the self-refresh period is longer than tRFC
    test.calcWindowEnergy(++i*windowSize); //2200 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::PDN_S_ACT,0,2220);
    test.doCommand(MemCommand::PUP_ACT,0,2240);
    test.doCommand(MemCommand::PRE,6,2260);
    test.doCommand(MemCommand::PDN_F_PRE,0,2280);
    test.doCommand(MemCommand::PUP_PRE,0,2300);
    test.doCommand(MemCommand::ACT,5,2320);
    test.doCommand(MemCommand::WR,5,2340);
    test.doCommand(MemCommand::PREA,0,2360);
    test.doCommand(MemCommand::REF,0,2380);
    //The window ended in the middle of an auto-refresh.
    test.calcWindowEnergy(++i*windowSize); //2400 cycles
    test.doCommand(MemCommand::ACT,2,2460);
    test.doCommand(MemCommand::RD,2,2480);
    test.doCommand(MemCommand::PRE,2,2500);
    test.doCommand(MemCommand::SREN,0,2515);
    test.doCommand(MemCommand::SREX,0,2580);
    //The window is longer than a self-refresh period and the self-refresh period is shorter than tRFC and longer than tRFC - tRP
    test.calcWindowEnergy(++i*windowSize); //2600 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::ACT,3,2660);
    test.doCommand(MemCommand::RD,3,2680);
    test.doCommand(MemCommand::WR,3,2700);
    test.doCommand(MemCommand::RDA,3,2720);
    test.doCommand(MemCommand::ACT,1,2780);
    test.doCommand(MemCommand::WRA,1,2800);
    //The command WRA will be divided into two commands (WR and PRE) and the PRE command will be included in the energy calculation of the next window
    test.calcWindowEnergy(++i*windowSize); //2800 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::ACT,7,2840);
    test.doCommand(MemCommand::RD,7,2860);
    test.doCommand(MemCommand::ACT,6,2880);
    test.doCommand(MemCommand::WR,6,2900);
    test.doCommand(MemCommand::ACT,0,2980);
    test.doCommand(MemCommand::RDA,0,3000);
    //The command RDA will be divided into two commands (RD and PRE) and the PRE command will be included in the energy calculation of the next window
    test.calcWindowEnergy(++i*windowSize); //3000 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::PRE,6,3040);
    test.doCommand(MemCommand::PDN_F_ACT,0,3180);
    //The window ended during a Active Fast-exit Power-dowx phase.
    test.calcWindowEnergy(++i*windowSize); //3200 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::PUP_ACT,0,3220);
    test.doCommand(MemCommand::PRE,7,3240);
    test.doCommand(MemCommand::PDN_S_PRE,0,3380);
    //The window ended during a Precharged Slow-exit Power-down phase.
    test.calcWindowEnergy(++i*windowSize); //3400 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " << i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::PUP_PRE,0,3420);
    test.doCommand(MemCommand::SREN,0,3535);
    //The window ended during a Self-refresh phase and after a number of self-refresh cycles less than tRFC and greater than tRFC - tRP
    test.calcWindowEnergy(++i*windowSize); //3600 cycles
    std::cout << "Window " << i << " Total Energy: " << test.getEnergy().window_energy << " pJ" <<  endl;
    std::cout << "Window " <<  i << " Average Power: " << test.getPower().window_average_power << " mW" << endl << endl;
    test.doCommand(MemCommand::SREX,0,3610);
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


