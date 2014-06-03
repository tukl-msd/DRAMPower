/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include "MemArchitectureSpec.h"

using namespace Data;

void MemArchitectureSpec::processParameters(){
  // Load all parameters in our member variables
  nbrOfBanks = getParamValWithDefault("nbrOfBanks", 1);
  nbrOfRanks = getParamValWithDefault("nbrOfRanks", 1); 
  nbrOfBankGroups = getParamValWithDefault("nbrOfBankGroups", 1);
  dataRate = getParamValWithDefault("dataRate", 1);
  burstLength = getParamValWithDefault("burstLength", 1);
  nbrOfColumns = getParamValWithDefault("nbrOfColumns", 1);
  nbrOfRows = getParamValWithDefault("nbrOfRows", 1);
  width = getParamValWithDefault("width", 1);
}
