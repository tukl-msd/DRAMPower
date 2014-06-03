/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include <iostream>

#include "Parametrisable.h"

namespace Data {

  class MemTimingSpec : public virtual Parametrisable {
  public:
    void processParameters();

    double clkMhz;
    unsigned RC;
    unsigned RCD;
    unsigned CCD;
    unsigned CCD_S;
    unsigned CCD_L;
    unsigned RRD;
    unsigned RRD_S;
    unsigned RRD_L;
    unsigned FAW;
    unsigned TAW;
    unsigned WTR;
    unsigned WTR_S;
    unsigned WTR_L;
    unsigned REFI;
    unsigned RL;
    unsigned RP;
    unsigned RFC;
    unsigned RAS;
    unsigned WL;
    unsigned AL;
    unsigned DQSCK;
    unsigned RTP;
    unsigned WR;
    unsigned XP;
    unsigned XPDLL;
    unsigned XS;
    unsigned XSDLL;
    unsigned CKE;
    unsigned CKESR;
    double clkPeriod;
  };

}