/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#ifndef DATA_MEMSPEC_PARSER_H
#define DATA_MEMSPEC_PARSER_H

#include "XMLHandler.h"
#include "Parametrisable.h"
#include "MemorySpecification.h"

namespace Data {

  class MemSpecParser : public XMLHandler {

  public:

    MemSpecParser();

    void startElement(const std::string& name,
                      const XERCES_CPP_NAMESPACE_QUALIFIER Attributes& attrs);
    void endElement(const std::string& name);

    MemorySpecification getMemorySpecification();

  private:
    MemorySpecification memorySpecification;
    Parametrisable* parameterParent;
  };
}
#endif