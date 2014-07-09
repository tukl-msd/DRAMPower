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
 * Authors: Matthias Jung, Omar Naji
 *
 */

#include"../src/LibDRAMPower.h"
int main(void)
{
		//Setup of DRAMPower for your simulation
		string filename;
		//type path to memspec file
		filename = "../memspecs/MICRON_1Gb_DDR2-1066_16bit_H.xml";
		libDRAMPower test;

		// During the simulation you can report activity
		// to DRAMPower with the doCommand(...) function:
		test.doCommand(MemCommand::ACT,0,35);
		test.doCommand(MemCommand::RDA,0,50);
		test.doCommand(MemCommand::ACT,4,51);
		test.doCommand(MemCommand::RDA,4,66);
		test.doCommand(MemCommand::ACT,0,86);
		test.doCommand(MemCommand::RDA,0,101);
		test.doCommand(MemCommand::ACT,2,102);
		test.doCommand(MemCommand::RDA,2,117);
		test.doCommand(MemCommand::ACT,5,119);
		test.doCommand(MemCommand::RDA,5,134);
		test.doCommand(MemCommand::ACT,0,137);
		test.doCommand(MemCommand::RDA,0,152);
		test.doCommand(MemCommand::ACT,3,159);
		test.doCommand(MemCommand::RDA,3,174);
		test.doCommand(MemCommand::ACT,0,195);
		test.doCommand(MemCommand::RDA,0,210);
		test.doCommand(MemCommand::ACT,4,232);
		test.doCommand(MemCommand::WRA,4,247);
		test.doCommand(MemCommand::PDN_F_ACT,3,248);

		// At the end of your simulation call the getEnergy(...)
		// function to print the power report
		test.getEnergy(filename);
		return 0;
}
