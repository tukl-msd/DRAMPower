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
#ifndef MEMSPEC_PARSER_H
#define MEMSPEC_PARSER_H
#include "jsonparser/JSONParser.h"
#include "MemArchitectureSpec.h"
#include "MemPowerSpec.h"
#include "MemTimingSpec.h"

//Root elements
static constexpr const char* MEM_ID        = "memoryId";
static constexpr const char* MEM_TYPE      = "memoryType";
static constexpr const char* MEM_TIME_SPEC = "memtimingspec";
static constexpr const char* MEM_POWR_SPEC = "mempowerspec";
static constexpr const char* MEM_ARCH_SPEC = "memarchitecturespec";
//MemTimingSpec
static constexpr const char* CLK_MHZ    = "memtimingspec/clkMhz";
static constexpr const char* RC         = "memtimingspec/RC";
static constexpr const char* RCD        = "memtimingspec/RCD";
static constexpr const char* CCD        = "memtimingspec/CCD";
static constexpr const char* CCD_S      = "memtimingspec/CCD_S";
static constexpr const char* CCD_L      = "memtimingspec/CCD_L";
static constexpr const char* RRD        = "memtimingspec/RRD";
static constexpr const char* RRD_S      = "memtimingspec/RRD_S";
static constexpr const char* RRD_L      = "memtimingspec/RRD_L";
static constexpr const char* FAW        = "memtimingspec/FAW";
static constexpr const char* TAW        = "memtimingspec/TAW";
static constexpr const char* WTR        = "memtimingspec/WTR";
static constexpr const char* WTR_S      = "memtimingspec/WTR_S";
static constexpr const char* WTR_L      = "memtimingspec/WTR_L";
static constexpr const char* REFI       = "memtimingspec/REFI";
static constexpr const char* RL         = "memtimingspec/RL";
static constexpr const char* RP         = "memtimingspec/RP";
static constexpr const char* RFC        = "memtimingspec/RFC";
static constexpr const char* REFB       = "memtimingspec/REFB";
static constexpr const char* RAS        = "memtimingspec/RAS";
static constexpr const char* WL         = "memtimingspec/WL";
static constexpr const char* AL         = "memtimingspec/AL";
static constexpr const char* DQSCK      = "memtimingspec/DQSCK";
static constexpr const char* RTP        = "memtimingspec/RTP";
static constexpr const char* WR         = "memtimingspec/WR";
static constexpr const char* XP         = "memtimingspec/XP";
static constexpr const char* XPDLL      = "memtimingspec/XPDLL";
static constexpr const char* XS         = "memtimingspec/XS";
static constexpr const char* XSDLL      = "memtimingspec/XSDLL";
static constexpr const char* CKE        = "memtimingspec/CKE";
static constexpr const char* CKESR      = "memtimingspec/CKESR";
static constexpr const char* CLK_PERIOD = "memtimingspec/clkPeriod";
//MemPowerSpec
static constexpr const char* IDD_01        = "mempowerspec/idd01";   
static constexpr const char* IDD_02        = "mempowerspec/idd02";
static constexpr const char* IDD_2P0       = "mempowerspec/idd2p0";
static constexpr const char* IDD_2P02      = "mempowerspec/idd2p02";
static constexpr const char* IDD_2P1       = "mempowerspec/idd2p1";
static constexpr const char* IDD_2P12      = "mempowerspec/idd2p12";
static constexpr const char* IDD_2N1       = "mempowerspec/idd2n1"; 
static constexpr const char* IDD_2N2       = "mempowerspec/idd2n2";
static constexpr const char* IDD_3P0       = "mempowerspec/idd3p0";
static constexpr const char* IDD_3P02      = "mempowerspec/idd3p02";
static constexpr const char* IDD_3P1       = "mempowerspec/idd3p1";
static constexpr const char* IDD_3P12      = "mempowerspec/idd3p12";
static constexpr const char* IDD_3N1       = "mempowerspec/idd3n1";
static constexpr const char* IDD_3N2       = "mempowerspec/idd3n2";
static constexpr const char* IDD_4R        = "mempowerspec/idd4r"; 
static constexpr const char* IDD_4R2       = "mempowerspec/idd4r2";
static constexpr const char* IDD_4W        = "mempowerspec/idd4w";
static constexpr const char* IDD_4W2       = "mempowerspec/idd4w2";
static constexpr const char* IDD_51        = "mempowerspec/idd51"; 
static constexpr const char* IDD_52        = "mempowerspec/idd52";
static constexpr const char* IDD_5B        = "mempowerspec/idd5B"; 
static constexpr const char* IDD_6         = "mempowerspec/idd61";
static constexpr const char* IDD_62        = "mempowerspec/idd62";
static constexpr const char* VDD1          = "mempowerspec/vdd1";   
static constexpr const char* VDD2          = "mempowerspec/vdd2";
static constexpr const char* CAPCITANCE    = "mempowerspec/capacitance";
static constexpr const char* IO_POWER      = "mempowerspec/ioPower";
static constexpr const char* WR_ODT_POWER  = "mempowerspec/wrOdtPower";
static constexpr const char* TERM_RD_POWER = "mempowerspec/termRdPower";
static constexpr const char* TERM_WR_POWER = "mempowerspec/termWrPower";
//MemArchitectureSpec
static constexpr const char* WIDTH                = "memarchitecturespec/width";
static constexpr const char* NBR_OF_BANKS         = "memarchitecturespec/nbrOfBanks";
static constexpr const char* NBR_OF_RANKS         = "memarchitecturespec/nbrOfRanks";
static constexpr const char* NBR_OF_BANK_GROUPS   = "memarchitecturespec/nbrOfBankGroups";
static constexpr const char* NBR_OF_COLUMNS       = "memarchitecturespec/nbrOfColumns";
static constexpr const char* NBR_OF_ROWS          = "memarchitecturespec/nbrOfRows";
static constexpr const char* DATA_RATE            = "memarchitecturespec/dataRate";
static constexpr const char* BURST_LENGTH         = "memarchitecturespec/burstLength";
static constexpr const char* DLL                  = "memarchitecturespec/dll";
static constexpr const char* TWO_VOLTAGE_DOMAINS  = "memarchitecturespec/twoVoltageDomains";
static constexpr const char* TERMINATION          = "memarchitecturespec/termination";

class MemSpecParser : public JSONParser{
public:
  const std::string& getMemoryId();
  const std::string& getMemoryType();
  const DRAMPower::MemArchitectureSpec& getMemArchitectureSpec();
  const DRAMPower::MemPowerSpec& getMemPowerSpec();
  const DRAMPower::MemTimingSpec& getMemTimingSpec();

  MemSpecParser(){}
  virtual ~MemSpecParser(){}
private:
  std::string memoryId;
  std::string memoryType;
  DRAMPower::MemArchitectureSpec memArchSpec;
  DRAMPower::MemPowerSpec memPowerSpec;
  DRAMPower::MemTimingSpec memTimingSpec;
};

#endif
