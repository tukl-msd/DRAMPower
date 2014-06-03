/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include "MemTimingSpec.h"

using namespace Data;

void MemTimingSpec::processParameters(){
  clkMhz = getParamValWithDefault("clkMhz", 0.0);
  RC = getParamValWithDefault("RC", 0);
  RCD = getParamValWithDefault("RCD", 0);
  CCD = getParamValWithDefault("CCD", 0);
  RRD = getParamValWithDefault("RRD", 0);
  WTR = getParamValWithDefault("WTR", 0);
  CCD_S = getParamValWithDefault("CCD_S", 0);
  CCD_L = getParamValWithDefault("CCD_L", 0);
  RRD_S = getParamValWithDefault("RRD_S", 0);
  RRD_L = getParamValWithDefault("RRD_L", 0);
  WTR_S = getParamValWithDefault("WTR_S", 0);
  WTR_L = getParamValWithDefault("WTR_L", 0);
  TAW = getParamValWithDefault("TAW", 0);
  FAW = getParamValWithDefault("FAW", 0);
  REFI = getParamValWithDefault("REFI", 0);
  RL = getParamValWithDefault("RL", 0);
  RP = getParamValWithDefault("RP", 0);
  RFC = getParamValWithDefault("RFC", 0);
  RAS = getParamValWithDefault("RAS", 0);
  WL = getParamValWithDefault("WL", 0);
  AL = getParamValWithDefault("AL", 0);
  DQSCK = getParamValWithDefault("DQSCK", 0);
  RTP = getParamValWithDefault("RTP", 0);
  WR = getParamValWithDefault("WR", 0);
  XP = getParamValWithDefault("XP", 0);
  XPDLL = getParamValWithDefault("XPDLL", 0);
  XS = getParamValWithDefault("XS", 0);
  XSDLL = getParamValWithDefault("XSDLL", 0);
  CKE = getParamValWithDefault("CKE", 0);
  CKESR = getParamValWithDefault("CKESR", 0);
  clkPeriod = 1000.0 / clkMhz;
}
