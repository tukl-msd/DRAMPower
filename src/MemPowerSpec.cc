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
 * Authors: Karthik Chandrasekar, Subash Kannoth
 *
 */

#include "MemPowerSpec.h"

using namespace DRAMPower;

MemPowerSpec::MemPowerSpec() :
  idd01(0.0),
  idd02(0.0),
  idd2p0(0.0),
  idd2p02(0.0),
  idd2p1(0.0),
  idd2p12(0.0),
  idd2n1(0.0),
  idd2n2(0.0),
  idd3p0(0.0),
  idd3p02(0.0),
  idd3p1(0.0),
  idd3p12(0.0),
  idd3n1(0.0),
  idd3n2(0.0),
  idd4r(0.0),
  idd4r2(0.0),
  idd4w(0.0),
  idd4w2(0.0),
  idd51(0.0),
  idd52(0.0),
  idd5B(0.0),
  idd61(0.0),
  idd62(0.0),
  vdd1(0.0),
  vdd2(0.0),
  capacitance(0.0),
  ioPower(0.0),
  wrOdtPower(0.0),
  termRdPower(0.0),
  termWrPower(0.0)
{
}

