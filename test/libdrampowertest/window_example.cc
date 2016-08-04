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

#include <string>
#include "libdrampower/LibDRAMPower.h"
#include "xmlparser/MemSpecParser.h"

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
        MemorySpecification memSpec(MemSpecParser::getMemSpecFromXML(filename));

        libDRAMPower test = libDRAMPower(memSpec, 0);
	int i = 0;
	unsigned long windowSize = 70;

        test.doCommand(MemCommand::ACT,0,25);
        test.doCommand(MemCommand::RD,0,35);
	test.doCommand(MemCommand::PRE,0,45);
        test.doCommand(MemCommand::ACT,4,55);
        test.doCommand(MemCommand::WR,4,65);
	test.calcWindowEnergy(++i*windowSize);
	std::cout << "Window " << i << " Total Energy:" << "\t" << test.getEnergy().window_energy << endl;
	std::cout << "Window " << i << " Average Power:" << "\t" << test.getPower().window_average_power << endl;
	test.doCommand(MemCommand::PRE,4,71);
        test.doCommand(MemCommand::ACT,0,80);
        test.doCommand(MemCommand::RDA,0,91);
        test.doCommand(MemCommand::ACT,2,100);
        test.doCommand(MemCommand::WRA,2,110);
        test.doCommand(MemCommand::ACT,5,114);
        test.doCommand(MemCommand::RD,5,120);
	test.doCommand(MemCommand::PRE,5,125);
        test.doCommand(MemCommand::ACT,0,130);	
	test.calcWindowEnergy(++i*windowSize);
        std::cout << "Window " << i << " Total Energy:" << "\t" << test.getEnergy().window_energy << endl;
	std::cout << "Window " << i << " Average Power:" << "\t" << test.getPower().window_average_power << endl;
        test.doCommand(MemCommand::RDA,0,141);
        test.doCommand(MemCommand::ACT,3,150);
        test.doCommand(MemCommand::WR,3,170);
	test.doCommand(MemCommand::PRE,3,180);
        test.doCommand(MemCommand::ACT,0,190);
        test.doCommand(MemCommand::RDA,0,209);
	test.calcWindowEnergy(++i*windowSize);
	std::cout << "Window " << i << " Total Energy:" << "\t" << test.getEnergy().window_energy << endl;
	std::cout << "Window " << i << " Average Power:" << "\t" << test.getPower().window_average_power << endl;
        test.doCommand(MemCommand::ACT,4,230);
	test.doCommand(MemCommand::WRA,4,240);
	test.doCommand(MemCommand::SREN,0,260);
	test.calcWindowEnergy(++i*windowSize);
	std::cout << "Window " << i << " Total Energy:" << "\t" << test.getEnergy().window_energy << endl;
	std::cout << "Window " << i << " Average Power:" << "\t" << test.getPower().window_average_power << endl;
	test.calcWindowEnergy(++i*windowSize);
	std::cout << "Window " << i << " Total Energy:" << "\t" << test.getEnergy().window_energy << endl;
	std::cout << "Window " << i << " Average Power:" << "\t" << test.getPower().window_average_power << endl;
	test.doCommand(MemCommand::SREX,0,360);
	test.doCommand(MemCommand::ACT,7,370);
        test.doCommand(MemCommand::WRA,7,380);
        test.doCommand(MemCommand::ACT,2,390);
        test.doCommand(MemCommand::RD,2,400);
	test.doCommand(MemCommand::PRE,2,410);
        test.calcEnergy();
	std::cout << "Window " << ++i << " Total Energy:" << "\t" << test.getEnergy().window_energy << endl;
	std::cout << "Window " <<  i << " Average Power:" << "\t" << test.getPower().window_average_power << endl;
	std::cout << "Total Energy:" << "\t" << test.getEnergy().total_energy << endl;
	std::cout << "Average Power:" << "\t" << test.getPower().average_power << endl;
}
