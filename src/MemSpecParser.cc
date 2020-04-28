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
#include "MemSpecParser.h"
#include <iostream>
#include <cassert>
#include <fstream>

using namespace DRAMPower;

const std::string& MemSpecParser::getMemoryId(){
  memoryId = getElementValWithDefault<std::string>(MEM_ID, "");
  return memoryId;
}

const std::string& MemSpecParser::getMemoryType(){
  memoryType = getElementValWithDefault<std::string>(MEM_TYPE, "");
  return memoryType;
}

const DRAMPower::MemArchitectureSpec& MemSpecParser::getMemArchitectureSpec() {
  auto hasTwoVoltageDomains = [&](const std::string& _mem_type) -> bool {
    return ((_mem_type == "LPDDR")     ||
            (_mem_type == "LPDDR2")    ||
            (_mem_type == "LPDDR3")    ||
            (_mem_type == "LPDDR4")    ||
            (_mem_type == "HBM2")      ||       
            (_mem_type == "WIDEIO_SDR")||
            (_mem_type == "DDR4" ));
  };
  auto hasTermination = [&](const std::string& _mem_type) -> bool {
    return ((_mem_type == "DDR2")   ||
            (_mem_type == "DDR3")   ||
            (_mem_type == "DDR4"));
  };
  auto hasDLL = [&](const std::string& _mem_type) -> bool {
    return ((_mem_type == "DDR2")   ||
            (_mem_type == "DDR3")   ||
            (_mem_type == "DDR4"));
  };

  validateElements({MEM_ARCH_SPEC,
                    BURST_LENGTH,
                    NBR_OF_BANKS,
                    NBR_OF_RANKS,
                    DATA_RATE,
                    NBR_OF_COLUMNS,
                    WIDTH,
                    MEM_TYPE}
                  );

  memArchSpec.burstLength       = getElementValWithDefault<int64_t>(BURST_LENGTH, 1);
  memArchSpec.nbrOfBanks        = getElementValWithDefault<int64_t>(NBR_OF_BANKS, 1);
  memArchSpec.nbrOfRanks        = getElementValWithDefault<int64_t>(NBR_OF_RANKS, 1);
  memArchSpec.dataRate          = getElementValWithDefault<int64_t>(DATA_RATE, 1);
  memArchSpec.nbrOfColumns      = getElementValWithDefault<int64_t>(NBR_OF_COLUMNS, 1);
  memArchSpec.nbrOfRows         = getElementValWithDefault<int64_t>(NBR_OF_ROWS, 1);
  memArchSpec.width             = getElementValWithDefault<int64_t>(WIDTH, 1);
  assert("memory width should be a multiple of 8" && (memArchSpec.width % 8) == 0);
  memArchSpec.nbrOfBankGroups   = getElementValWithDefault<int64_t>(NBR_OF_BANK_GROUPS, 1);
  memArchSpec.dll               = hasDLL(getElementValWithDefault<std::string>(MEM_TYPE, ""));
  memArchSpec.twoVoltageDomains = hasTwoVoltageDomains(getElementValWithDefault<std::string>(MEM_TYPE, ""));
  memArchSpec.termination       = hasTermination(getElementValWithDefault<std::string>(MEM_TYPE, ""));
  return memArchSpec;
}

const DRAMPower::MemPowerSpec& MemSpecParser::getMemPowerSpec() {

  validateElements({MEM_POWR_SPEC});

  memPowerSpec.idd01       = getElementValWithDefault<double>(IDD_01, 0.0);
  memPowerSpec.idd02       = getElementValWithDefault<double>(IDD_02, 0.0);
  memPowerSpec.idd2p0      = getElementValWithDefault<double>(IDD_2P0, 0.0);
  memPowerSpec.idd2p02     = getElementValWithDefault<double>(IDD_2P02, 0.0);
  memPowerSpec.idd2p1      = getElementValWithDefault<double>(IDD_2P1, 0.0);
  memPowerSpec.idd2p12     = getElementValWithDefault<double>(IDD_2P12, 0.0);
  memPowerSpec.idd2n1      = getElementValWithDefault<double>(IDD_2N1, 0.0);
  memPowerSpec.idd2n2      = getElementValWithDefault<double>(IDD_2N2, 0.0);
  memPowerSpec.idd3p0      = getElementValWithDefault<double>(IDD_3P0, 0.0);
  memPowerSpec.idd3p02     = getElementValWithDefault<double>(IDD_3P02, 0.0);
  memPowerSpec.idd3p1      = getElementValWithDefault<double>(IDD_3P1, 0.0);
  memPowerSpec.idd3p12     = getElementValWithDefault<double>(IDD_3P12, 0.0);
  memPowerSpec.idd3n1      = getElementValWithDefault<double>(IDD_3N1, 0.0);
  memPowerSpec.idd3n2      = getElementValWithDefault<double>(IDD_3N2, 0.0);
  memPowerSpec.idd4r       = getElementValWithDefault<double>(IDD_4R, 0.0);
  memPowerSpec.idd4r2      = getElementValWithDefault<double>(IDD_4R2, 0.0);
  memPowerSpec.idd4w       = getElementValWithDefault<double>(IDD_4W, 0.0);
  memPowerSpec.idd4w2      = getElementValWithDefault<double>(IDD_4W2, 0.0);
  memPowerSpec.idd51       = getElementValWithDefault<double>(IDD_51, 0.0);
  memPowerSpec.idd52       = getElementValWithDefault<double>(IDD_52, 0.0);
  memPowerSpec.idd5B       = getElementValWithDefault<double>(IDD_5B, 0.0);
  memPowerSpec.idd61       = getElementValWithDefault<double>(IDD_6, 0.0);
  memPowerSpec.idd62       = getElementValWithDefault<double>(IDD_62, 0.0);
  memPowerSpec.vdd1        = getElementValWithDefault<double>(VDD1, 0.0);
  memPowerSpec.vdd2        = getElementValWithDefault<double>(VDD2, 0.0);
  memPowerSpec.capacitance = getElementValWithDefault<double>(CAPCITANCE, 0.0);
  memPowerSpec.ioPower     = getElementValWithDefault<double>(IO_POWER, 0.0);
  memPowerSpec.wrOdtPower  = getElementValWithDefault<double>(WR_ODT_POWER, 0.0);
  memPowerSpec.termRdPower = getElementValWithDefault<double>(TERM_RD_POWER, 0.0);
  memPowerSpec.termWrPower = getElementValWithDefault<double>(TERM_WR_POWER, 0.0);
  return memPowerSpec;
}
const DRAMPower::MemTimingSpec& MemSpecParser::getMemTimingSpec() {

  validateElements({MEM_TIME_SPEC});
  
  memTimingSpec.clkMhz    = getElementValWithDefault<double>(CLK_MHZ, 0.0);
  memTimingSpec.RC        = getElementValWithDefault<int64_t>(RC, 0);
  memTimingSpec.RCD       = getElementValWithDefault<int64_t>(RCD, 0);
  memTimingSpec.CCD       = getElementValWithDefault<int64_t>(CCD, 0);    
  memTimingSpec.CCD_S     = getElementValWithDefault<int64_t>(CCD_S, 0);
  memTimingSpec.CCD_L     = getElementValWithDefault<int64_t>(CCD_L, 0);
  memTimingSpec.RRD       = getElementValWithDefault<int64_t>(RRD, 0);
  memTimingSpec.RRD_S     = getElementValWithDefault<int64_t>(RRD_S, 0);
  memTimingSpec.RRD_L     = getElementValWithDefault<int64_t>(RRD_L, 0);
  memTimingSpec.FAW       = getElementValWithDefault<int64_t>(FAW, 0);
  memTimingSpec.TAW       = getElementValWithDefault<int64_t>(TAW, 0);
  memTimingSpec.WTR       = getElementValWithDefault<int64_t>(WTR, 0);
  memTimingSpec.WTR_S     = getElementValWithDefault<int64_t>(WTR_S, 0);
  memTimingSpec.WTR_L     = getElementValWithDefault<int64_t>(WTR_L, 0);
  memTimingSpec.REFI      = getElementValWithDefault<int64_t>(REFI, 0);
  memTimingSpec.RL        = getElementValWithDefault<int64_t>(RL, 0);
  memTimingSpec.RP        = getElementValWithDefault<int64_t>(RP, 0);
  memTimingSpec.RFC       = getElementValWithDefault<int64_t>(RFC, 0);
  memTimingSpec.REFB      = getElementValWithDefault<int64_t>(REFB, 0);
  memTimingSpec.RAS       = getElementValWithDefault<int64_t>(RAS, 0);
  memTimingSpec.WL        = getElementValWithDefault<int64_t>(WL, 0);
  memTimingSpec.AL        = getElementValWithDefault<int64_t>(AL, 0);
  memTimingSpec.DQSCK     = getElementValWithDefault<int64_t>(DQSCK, 0);
  memTimingSpec.RTP       = getElementValWithDefault<int64_t>(RTP, 0);
  memTimingSpec.WR        = getElementValWithDefault<int64_t>(WR, 0);
  memTimingSpec.XP        = getElementValWithDefault<int64_t>(XP, 0);
  memTimingSpec.XPDLL     = getElementValWithDefault<int64_t>(XPDLL, 0);
  memTimingSpec.XS        = getElementValWithDefault<int64_t>(XS, 0);
  memTimingSpec.XSDLL     = getElementValWithDefault<int64_t>(XSDLL, 0);
  memTimingSpec.CKE       = getElementValWithDefault<int64_t>(CKE, 0);
  memTimingSpec.CKESR     = getElementValWithDefault<int64_t>(CKESR, 0);
  memTimingSpec.clkPeriod = 1000.0/ getElementValWithDefault<double>(CLK_MHZ, 0.0);
  return memTimingSpec;
}
