/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include "MemSpecParser.h"

XERCES_CPP_NAMESPACE_USE

using namespace Data;
using namespace std;

MemSpecParser::MemSpecParser() :
  parameterParent(NULL) {
}

void MemSpecParser::startElement(const string& name, const Attributes& attrs) {
  if (name == "memspec") {
    parameterParent = &memorySpecification;
  }
  else if (name == "memarchitecturespec") {
    parameterParent = &memorySpecification.memArchSpec;
  }
  else if (name == "memtimingspec") {
    parameterParent = &memorySpecification.memTimingSpec;
  }
  else if (name == "mempowerspec") {
    parameterParent = &memorySpecification.memPowerSpec;
  }
  else if (name == "parameter") {
    Parameter parameter(getValue("id", attrs),
                        getValue("type", attrs),
                        getValue("value", attrs));
    assert(parameterParent != NULL);
    parameterParent->pushParameter(parameter);
  }
}

void MemSpecParser::endElement(const string& name) {
 if (name == "memarchitecturespec") {
    memorySpecification.memArchSpec.processParameters();
    // Reset parameterParent
    parameterParent = &memorySpecification;
  }
  else if (name == "memtimingspec") {
    memorySpecification.memTimingSpec.processParameters();
    // Reset parameterParent
    parameterParent = &memorySpecification;
  }
  else if (name == "mempowerspec") {
    memorySpecification.memPowerSpec.processParameters();
    // Reset parameterParent
    parameterParent = &memorySpecification;
  }
  else if (name == "memspec"){
    memorySpecification.processParameters();
  }
}

MemorySpecification MemSpecParser::getMemorySpecification(){
  return memorySpecification;
}
