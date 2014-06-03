/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include "MemPowerSpec.h"

using namespace Data;

void MemPowerSpec:: processParameters(){
  idd0 = getParamValWithDefault("idd0", 0.0);
  idd02 = getParamValWithDefault("idd02", 0.0);
  idd2p0 = getParamValWithDefault("idd2p0", 0.0);
  idd2p02 = getParamValWithDefault("idd2p02", 0.0);
  idd2p1 = getParamValWithDefault("idd2p1", 0.0);
  idd2p12 = getParamValWithDefault("idd2p12", 0.0);
  idd2n = getParamValWithDefault("idd2n", 0.0);
  idd2n2 = getParamValWithDefault("idd2n2", 0.0);
  idd3p0 = getParamValWithDefault("idd3p0", 0.0);
  idd3p02 = getParamValWithDefault("idd3p02", 0.0);
  idd3p1 = getParamValWithDefault("idd3p1", 0.0);
  idd3p12 = getParamValWithDefault("idd3p12", 0.0);
  idd3n = getParamValWithDefault("idd3n", 0.0);
  idd3n2 = getParamValWithDefault("idd3n2", 0.0);
  idd4r = getParamValWithDefault("idd4r", 0.0);
  idd4r2 = getParamValWithDefault("idd4r2", 0.0);
  idd4w = getParamValWithDefault("idd4w", 0.0);
  idd4w2 = getParamValWithDefault("idd4w2", 0.0);
  idd5 = getParamValWithDefault("idd5", 0.0);
  idd52 = getParamValWithDefault("idd52", 0.0);
  idd6 = getParamValWithDefault("idd6", 0.0);
  idd62 = getParamValWithDefault("idd62", 0.0);
  vdd = getParamValWithDefault("vdd", 0.0);
  vdd2 = getParamValWithDefault("vdd2", 0.0);
}
