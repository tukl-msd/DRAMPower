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
 * Authors: Andreas Hansson
 *
 */

#include "XMLHandler.h"

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>

#include <iostream>

XERCES_CPP_NAMESPACE_USE

using namespace Data;
using namespace std;

XMLHandler::XMLHandler()
{
}

XMLHandler::~XMLHandler()
{
}

void XMLHandler::startElement(const XMLCh* const,
                              const XMLCh* const,
                              const XMLCh* const qname,
                              const Attributes&  attrs)
{
  string name = transcode(qname);

  startElement(name, attrs);
}

void XMLHandler::endElement(const XMLCh* const,
                            const XMLCh* const,
                            const XMLCh* const qname)
{
  string name = transcode(qname);

  endElement(name);
}

string XMLHandler::transcode(const XMLCh* const qname) const
{
  char*  buf = XMLString::transcode(qname);
  string name(buf);

  delete buf; //  In xerces 2.2.0: XMLString::release (&value);
  return name;
}

bool XMLHandler::hasValue(const string&     id,
                          const Attributes& attrs) const
{
  XMLCh* attr        = XMLString::transcode(id.c_str());
  const XMLCh* value = attrs.getValue(attr);
  XMLString::release(&attr);

  return value != NULL;
}

string XMLHandler::getValue(const string&     id,
                            const Attributes& attrs) const
{
  XMLCh* attr        = XMLString::transcode(id.c_str());
  const XMLCh* value = attrs.getValue(attr);
  XMLString::release(&attr);

  assert(value != NULL);
  return transcode(value);
}

void XMLHandler::error(const SAXParseException& e)
{
  char* file    = XMLString::transcode(e.getSystemId());
  char* message = XMLString::transcode(e.getMessage());

  cerr << "\nError at file " << file
       << ", line " << e.getLineNumber()
       << ", char " << e.getColumnNumber()
       << "\n  Message: " << message << endl;

  XMLString::release(&file);
  XMLString::release(&message);
}

void XMLHandler::fatalError(const SAXParseException& e)
{
  char* file    = XMLString::transcode(e.getSystemId());
  char* message = XMLString::transcode(e.getMessage());

  cerr << "\nFatal Error at file " << file
       << ", line " << e.getLineNumber()
       << ", char " << e.getColumnNumber()
       << "\n  Message: " << message << endl;

  XMLString::release(&file);
  XMLString::release(&message);
}

void XMLHandler::warning(const SAXParseException& e)
{
  char* file    = XMLString::transcode(e.getSystemId());
  char* message = XMLString::transcode(e.getMessage());

  cerr << "\nWarning at file " << file
       << ", line " << e.getLineNumber()
       << ", char " << e.getColumnNumber()
       << "\n  Message: " << message << endl;

  XMLString::release(&file);
  XMLString::release(&message);
}
