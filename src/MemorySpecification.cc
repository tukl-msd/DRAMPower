/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include "MemorySpecification.h"
#include "MemSpecParser.h"
#include "XMLParser.h"
#include <fstream>
#include <sstream>


using namespace std;
using namespace Data;

//Set variable values from XML
void MemorySpecification::processParameters(){
  setVarFromParam(&id, "memoryId");
  string memoryTypeString;
  setVarFromParam(&memoryTypeString, "memoryType");
  memoryType = getMemoryTypeFromName(memoryTypeString);
  }

//Get memSpec from XML
MemorySpecification MemorySpecification::getMemSpecFromXML(const string& id){

  MemSpecParser memSpecParser;
  cout << "* Parsing " << id << endl;
  XMLParser::parse(id, &memSpecParser);

  return memSpecParser.getMemorySpecification();
}
