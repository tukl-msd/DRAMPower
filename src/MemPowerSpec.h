/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include "Parametrisable.h"

namespace Data {

  class MemPowerSpec: public virtual Parametrisable{
  public:
    void processParameters();

    double idd0;
    double idd02;
    double idd2p0;
    double idd2p02;
    double idd2p1;
    double idd2p12;
    double idd2n;
    double idd2n2;
    double idd3p0;
    double idd3p02;
    double idd3p1;
    double idd3p12;
    double idd3n;
    double idd3n2;
    double idd4r;
    double idd4r2;
    double idd4w;
    double idd4w2;
    double idd5;
    double idd52;
    double idd6;
    double idd62;
    double vdd;
    double vdd2;
   };
}
