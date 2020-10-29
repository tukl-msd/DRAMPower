/*
 * Copyright (c) 2019, University of Kaiserslautern
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *    Lukas Steiner
 *    Luiza Correa
 */

#include "MemSpecDDR4.h"

using json = nlohmann::json;


MemSpecDDR4::MemSpecDDR4(json &memspec,
                         const bool debug,
                         const bool writeToConsole,
                         const bool writeToFile,
                         const std::string &traceName)
    : MemSpec(memspec,debug,writeToConsole,writeToFile,traceName)
{
      numberOfDevicesOnDIMM = parseUint(memspec["memarchitecturespec"]["nbrOfDevicesOnDIMM"],"nbrOfDevicesOnDIMM");
      numberOfBankGroups = parseUint(memspec["memarchitecturespec"]["nbrOfBankGroups"],"nbrOfBankGroups");
      banksPerGroup = numberOfBanks / numberOfBankGroups;

      refreshMode            = (parseStringWithDefault(memspec["RefreshMode"],"RefreshMode","1X"));
      memTimingSpec.fCKMHz   = (parseUdouble(memspec["memtimingspec"]["clkMhz"], "clkMhz"));
      memTimingSpec.tCK      = (1000.0 / memTimingSpec.fCKMHz); //clock period in mili seconds
      memTimingSpec.tCKESR   = (parseUint(memspec["memtimingspec"]["CKESR"], "CKESR"));
      memTimingSpec.tCKE     = (parseUint(memspec["memtimingspec"]["CKE"], "CKE"));
      memTimingSpec.tPD      = memTimingSpec.tCKE;
      memTimingSpec.tDQSCK   = (parseUint(memspec["memtimingspec"]["DQSCK"], "DQSCK"));
      memTimingSpec.tRAS     = (parseUint(memspec["memtimingspec"]["RAS"], "RAS"));
      memTimingSpec.tRC      = (parseUint(memspec["memtimingspec"]["RC"], "RC"));
      memTimingSpec.tRCD     = (parseUint(memspec["memtimingspec"]["RCD"], "RCD"));
      memTimingSpec.tRL      = (parseUint(memspec["memtimingspec"]["RL"], "RL"));
      memTimingSpec.tRTP     = (parseUint(memspec["memtimingspec"]["RTP"], "RTP"));
      memTimingSpec.tWL      = (parseUint(memspec["memtimingspec"]["WL"], "WL"));
      memTimingSpec.tWR      = (parseUint(memspec["memtimingspec"]["WR"], "WR"));
      memTimingSpec.tXP      = (parseUint(memspec["memtimingspec"]["XP"], "XP"));
      memTimingSpec.tXS      = (parseUint(memspec["memtimingspec"]["XS"], "XS"));
      memTimingSpec.tCCD_S   = (parseUint(memspec["memtimingspec"]["CCD_S"], "CCD_S"));
      memTimingSpec.tCCD_L   = (parseUint(memspec["memtimingspec"]["CCD_L"], "CCD_L"));
      memTimingSpec.tRP      = (parseUint(memspec["memtimingspec"]["RP"], "RP"));
      memTimingSpec.tFAW     = (parseUint(memspec["memtimingspec"]["FAW"], "FAW"));
      if (refreshMode=="1X"){
          memTimingSpec.tRFC = (parseUint(memspec["memtimingspec"]["RFC"], "RFC"));
          memTimingSpec.tREFI    = (parseUint(memspec["memtimingspec"]["REFI"], "REFI"));
//          memTimingSpec.refreshtRP = memTimingSpec.tRP;
      }
      else if (refreshMode=="2X"){
          memTimingSpec.tRFC = (parseUint(memspec["memtimingspec"]["RFC2"], "RFC2"));
          memTimingSpec.tREFI    = (parseUint(memspec["memtimingspec"]["REFI"], "REFI")/2);
//          memTimingSpec.refreshtRP = static_cast<unsigned int>((memTimingSpec.tRP)*
//                               (parseUint(memspec["memtimingspec"]["RFC2"], "RFC2"))/
//                               (parseUint(memspec["memtimingspec"]["RFC"], "RFC")));
      }
      else if (refreshMode=="4X"){
          memTimingSpec.tRFC = (parseUint(memspec["memtimingspec"]["RFC4"], "RFC4"));
          memTimingSpec.tREFI    = (parseUint(memspec["memtimingspec"]["REFI"], "REFI")/4);
//          memTimingSpec.refreshtRP = static_cast<unsigned int>((memTimingSpec.tRP)*
//                               (parseUint(memspec["memtimingspec"]["RFC4"], "RFC4"))/
//                               (parseUint(memspec["memtimingspec"]["RFC"], "RFC")));
      }
      memTimingSpec.tRRD_S   = (parseUint(memspec["memtimingspec"]["RRD_S"], "RRD_S"));
      memTimingSpec.tRRD_L   = (parseUint(memspec["memtimingspec"]["RRD_L"], "RRD_L"));
      memTimingSpec.tWTR_S   = (parseUint(memspec["memtimingspec"]["WTR_S"], "WTR_S"));
      memTimingSpec.tWTR_L   = (parseUint(memspec["memtimingspec"]["WTR_L"], "WTR_L"));
      memTimingSpec.tAL      = (parseUint(memspec["memtimingspec"]["AL"], "AL"));
      memTimingSpec.tXPDLL   = (parseUint(memspec["memtimingspec"]["XPDLL"], "XPDLL"));
      memTimingSpec.tXSDLL   = (parseUint(memspec["memtimingspec"]["XSDLL"], "XSDLL"));
      memTimingSpec.tACTPDEN = (parseUint(memspec["memtimingspec"]["ACTPDEN"], "ACTPDEN"));
      memTimingSpec.tPRPDEN  = (parseUint(memspec["memtimingspec"]["PRPDEN"], "PRPDEN"));
      memTimingSpec.tREFPDEN = (parseUint(memspec["memtimingspec"]["REFPDEN"], "REFPDEN"));
      memTimingSpec.tRTRS    = (parseUint(memspec["memtimingspec"]["RTRS"], "RTRS"));

      prechargeOffsetRD      =  memTimingSpec.tAL + memTimingSpec.tRTP;
      prechargeOffsetWR      =  ((memArchSpec.burstLength)/(memArchSpec.dataRate)) + memTimingSpec.tWL + memTimingSpec.tWR;

      //Push back new subject created with default constructor.
      memPowerSpec.push_back(MemPowerSpec());

      memPowerSpec[0].iXX0      = (parseUdouble(memspec["mempowerspec"]["idd0"], "idd0"));
      memPowerSpec[0].iXX2N     = (parseUdouble(memspec["mempowerspec"]["idd2n"], "idd2n"));
      memPowerSpec[0].iXX3N     = (parseUdouble(memspec["mempowerspec"]["idd3n"], "idd3n"));
      memPowerSpec[0].iXX4R     = (parseUdouble(memspec["mempowerspec"]["idd4r"], "idd4r"));
      memPowerSpec[0].iXX4W     = (parseUdouble(memspec["mempowerspec"]["idd4w"], "idd4w"));
      memPowerSpec[0].iXX5      = (parseUdouble(memspec["mempowerspec"]["idd5B"], "idd5B"));
      memPowerSpec[0].iXX6      = (parseUdouble(memspec["mempowerspec"]["idd6"], "idd6"));
      memPowerSpec[0].vXX       = (parseUdouble(memspec["mempowerspec"]["vdd"], "vdd"));
      memPowerSpec[0].iXX2P     = (parseUdouble(memspec["mempowerspec"]["idd2p"], "idd2p"));
      memPowerSpec[0].iXX3P     = (parseUdouble(memspec["mempowerspec"]["idd3p"], "idd3p"));

      memPowerSpec.push_back(MemPowerSpec());

      memPowerSpec[1].iXX0      = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp0"], "ipp0"));
      memPowerSpec[1].iXX2N     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp2n"], "ipp2n"));
      memPowerSpec[1].iXX3N     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp3n"], "ipp3n"));
      memPowerSpec[1].iXX4R     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp4r"], "ipp4r"));
      memPowerSpec[1].iXX4W     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp4w"], "ipp4w"));
      memPowerSpec[1].iXX5      = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp5B"], "ipp5B"));
      memPowerSpec[1].iXX6      = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp6"], "ipp6"));
      memPowerSpec[1].vXX       = (parseUdoubleWithDefault(memspec["mempowerspec"]["vpp"], "vpp"));
      memPowerSpec[1].iXX2P     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp2p"], "ipp2p"));
      memPowerSpec[1].iXX3P     = (parseUdoubleWithDefault(memspec["mempowerspec"]["ipp3p"], "ipp3p"));

      //optional parameters
      memPowerSpec[0].capacitance = (parseUdoubleWithDefault(memspec["mempowerspec"]["capacitance"], "capacitance"));
      memPowerSpec[0].ioPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["ioPower"], "ioPower"));
      memPowerSpec[0].wrOdtPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["wrOdtPower"], "wrOdtPower"));
      memPowerSpec[0].termRdPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["termRdPower"], "termRdPower"));
      memPowerSpec[0].termWrPower = (parseUdoubleWithDefault(memspec["mempowerspec"]["termWrPower"], "termWrPower"));

      json bankWise = memspec["bankwisespec"];
      if (!bankWise.empty()){

          unsigned nbrofBanks = memArchSpec.numberOfBanks;

          bwParams.bwPowerFactRho = parseUint(memspec["bankwisespec"]["factRho"],"factRho");
          bwParams.bwPowerFactSigma = parseUint(memspec["bankwisespec"]["factSigma"],"factSigma");
          bwParams.flgPASR = parseBool(memspec["bankwisespec"]["hasPASR"],"hasPASR");
          if (bwParams.flgPASR){

              bwParams.pasrMode = parseUint(memspec["bankwisespec"]["pasrMode"],"pasrMode");
              ///////////////////////////////////////////////////////////
              // Activate banks for self refresh based on the PASR mode
              // ACTIVE     - X
              // NOT ACTIVE - 0
              ///////////////////////////////////////////////////////////
              switch(bwParams.pasrMode){

              case(BankWiseParams::pasrModes::PASR_0):{
                  // PASR MODE 0
                  // FULL ARRAY
                  // |X X X X |
                  // |X X X X |
                  bwParams.activeBanks.resize(nbrofBanks);
                  std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 0);
                  break;
              }
              case(BankWiseParams::pasrModes::PASR_1):{
                  // PASR MODE 1
                  // (1/2) ARRAY
                  // |X X X X |
                  // |0 0 0 0 |
                  bwParams.activeBanks.resize(nbrofBanks - 4);
                  std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 0);
                  break;
              }
              case(BankWiseParams::pasrModes::PASR_2):{
                  // PASR MODE 2
                  // (1/4) ARRAY
                  // |X X 0 0 |
                  // |0 0 0 0 |
                  bwParams.activeBanks.resize(nbrofBanks - 6);
                  std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 0);
                  break;
              }
              case(BankWiseParams::pasrModes::PASR_3):{
                  // PASR MODE 3
                  // (1/8) ARRAY
                  // |X 0 0 0 |
                  // |0 0 0 0 |
                  bwParams.activeBanks.resize(nbrofBanks - 7);
                  std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 0);
                  break;
              }
              case(BankWiseParams::pasrModes::PASR_4):{
                  // PASR MODE 4
                  // (3/4) ARRAY
                  // |0 0 X X |
                  // |X X X X |
                  bwParams.activeBanks.resize(nbrofBanks - 2);
                  std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 2);
                  break;
              }
              case(BankWiseParams::pasrModes::PASR_5):{
                  // PASR MODE 5
                  // (1/2) ARRAY
                  // |0 0 0 0 |
                  // |X X X X |
                  bwParams.activeBanks.resize(nbrofBanks - 4);
                  std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 4);
                  break;
              }
              case(BankWiseParams::pasrModes::PASR_6):{
                  // PASR MODE 6
                  // (1/4) ARRAY
                  // |0 0 0 0 |
                  // |0 0 X X |
                  bwParams.activeBanks.resize(nbrofBanks - 6);
                  std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 6);
                  break;
              }
              case(BankWiseParams::pasrModes::PASR_7):{
                  // PASR MODE 7
                  // (1/8) ARRAY
                  // |0 0 0 0 |
                  // |0 0 0 X |
                  bwParams.activeBanks.resize(nbrofBanks - 7);
                  std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 7);
                  break;
              }
              default:{
                  // PASR MODE 0
                  // FULL ARRAY
                  // |X X X X |
                  // |X X X X |
                  bwParams.activeBanks.resize(nbrofBanks);
                  std::iota(bwParams.activeBanks.begin(), bwParams.activeBanks.end(), 0);
                  break;
              }
            } //end switch
          } // end IF flgPASR
      } // end if !bankwise.empty()
      else{
         bwParams.bwPowerFactRho = 100;
         bwParams.bwPowerFactSigma = 100;
         bwParams.flgPASR = false;
      }
}

bool MemSpecDDR4::BankWiseParams::isBankActiveInPasr(const unsigned bankIdx) const
{
  return (std::find(activeBanks.begin(), activeBanks.end(), bankIdx)
      != activeBanks.end());
}

int64_t MemSpecDDR4::timeToCompletion(DRAMPower::MemCommand::cmds type)
{
  int64_t offset = 0;

  if (type == DRAMPower::MemCommand::RD) {
    offset = memTimingSpec.tRL +
                              memTimingSpec.tDQSCK + 1 + (memArchSpec.burstLength /
                                                         memArchSpec.dataRate);
  } else if (type == DRAMPower::MemCommand::WR) {
    offset = memTimingSpec.tWL +
                              (memArchSpec.burstLength / memArchSpec.dataRate) +
                              memTimingSpec.tWR;
  } else if (type == DRAMPower::MemCommand::ACT) {
    offset = memTimingSpec.tRCD;
  } else if ((type == DRAMPower::MemCommand::PRE) || (type == DRAMPower::MemCommand::PREA)) {
    offset = memTimingSpec.tRP;
  }
  return offset;
} // MemSpecDDR4::timeToCompletion

int64_t MemSpecDDR4::getExitSREFtime(){
    return memTimingSpec.tXSDLL - memTimingSpec.tRCD;
}

