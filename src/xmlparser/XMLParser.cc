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

#include "XMLParser.h"

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>

#include <iostream>

XERCES_CPP_NAMESPACE_USE

using namespace std;

void XMLParser::parse(const string& filename, DefaultHandler* handler)
{
  try
  {
    XMLPlatformUtils::Initialize();
  }

  catch (const XMLException& toCatch) {
    cerr << "Error in XML initialization";
    exit(1);
  }
  SAX2XMLReader* parser = XMLReaderFactory::createXMLReader();

  parser->setContentHandler(handler);
  parser->setErrorHandler(handler);

  parser->setFeature(XMLUni::fgSAX2CoreValidation, true);
  parser->setFeature(XMLUni::fgXercesDynamic, false);
  parser->setFeature(XMLUni::fgXercesSchema, true);

  try
  {
    parser->parse(filename.c_str());
  }

  catch (const XMLException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    cerr << "XMLException in " << toCatch.getSrcFile() << " at line " <<
      toCatch.getSrcLine() << ":\n" << message << "\n";
    XMLString::release(&message);
    exit(1);
  }

  catch (const SAXParseException& toCatch) {
    char* message = XMLString::transcode(toCatch.getMessage());
    cerr << "SAXParseException at line " <<
      toCatch.getLineNumber() << ", column " <<
      toCatch.getColumnNumber() << ":\n" << message << "\n";
    XMLString::release(&message);
    exit(1);
  }

  catch (const exception& e) {
    cerr << "Exception during XML parsing:" << endl
         << e.what() << endl;
    exit(1);
  }

  catch (...) {
    cerr << "XML Exception" << endl;
    exit(1);
  }
  XMLSize_t errorCount = parser->getErrorCount();

  delete parser;

  XMLPlatformUtils::Terminate();

  // See if we had any issues and act accordingly.
  if (errorCount != 0) {
    cerr << "XMLParser terminated with " << errorCount << " error(s)\n";
    exit(1);
  }
} // XMLParser::parse
