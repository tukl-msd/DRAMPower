/*
 * Copyright (c) 2012-2014, TU Delft
 * Copyright (c) 2012-2014, TU Eindhoven
 * Copyright (c) 2012-2014, TU Kaiserslautern
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
 * Authors: Karthik Chandrasekar
 *
 */

#include "xmlparser/MemSpecParser.h"

#include <iostream>

#include "xmlparser/XMLParser.h"

XERCES_CPP_NAMESPACE_USE

using namespace Data;
using namespace std;

MemSpecParser::MemSpecParser() :
  parameterParent(NULL)
{
}

void MemSpecParser::startElement(const string& name, const Attributes& attrs)
{
  if (name == "memspec") {
    parameterParent = &memorySpecification;
  } else if (name == "memarchitecturespec")   {
    parameterParent = &memorySpecification.memArchSpec;
  } else if (name == "memtimingspec")   {
    parameterParent = &memorySpecification.memTimingSpec;
  } else if (name == "mempowerspec")   {
    parameterParent = &memorySpecification.memPowerSpec;
  } else if (name == "parameter")   {
    Parameter parameter(getValue("id", attrs),
                        hasValue("type", attrs) ? getValue("type", attrs) : "uint",
                        getValue("value", attrs));
    assert(parameterParent != NULL);
    parameterParent->pushParameter(parameter);
  }
} // MemSpecParser::startElement

void MemSpecParser::endElement(const string& name)
{
  if (name == "memarchitecturespec") {
    memorySpecification.memArchSpec.processParameters();
    // Reset parameterParent
    parameterParent = &memorySpecification;
  } else if (name == "memtimingspec")   {
    memorySpecification.memTimingSpec.processParameters();
    // Reset parameterParent
    parameterParent = &memorySpecification;
  } else if (name == "mempowerspec")   {
    memorySpecification.memPowerSpec.processParameters();
    // Reset parameterParent
    parameterParent = &memorySpecification;
  } else if (name == "memspec")   {
    memorySpecification.processParameters();
  }
} // MemSpecParser::endElement

MemorySpecification MemSpecParser::getMemorySpecification()
{
  return memorySpecification;
}

// Get memSpec from XML
MemorySpecification MemSpecParser::getMemSpecFromXML(const string& id)
{
  MemSpecParser memSpecParser;

  cout << "* Parsing " << id << endl;
  XMLParser::parse(id, &memSpecParser);

  return memSpecParser.getMemorySpecification();
}
