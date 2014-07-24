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

#ifndef DATA_XML_HANDLER_H
#define DATA_XML_HANDLER_H

#include <xercesc/sax2/DefaultHandler.hpp>

#include <string>

namespace Data {
class XMLHandler : public XERCES_CPP_NAMESPACE_QUALIFIER DefaultHandler {
 public:
  XMLHandler();

  virtual ~XMLHandler();

  void startElement(const XMLCh* const                               namespace_uri,
                    const XMLCh* const                               localname,
                    const XMLCh* const                               qname,
                    const XERCES_CPP_NAMESPACE_QUALIFIER Attributes& attrs);

  void endElement(const XMLCh* const namespace_uri,
                  const XMLCh* const localname,
                  const XMLCh* const qname);

 protected:
  virtual void startElement(const std::string& name,
                            const XERCES_CPP_NAMESPACE_QUALIFIER Attributes&
                            attrs) = 0;

  virtual void endElement(const std::string& name) = 0;

  bool hasValue(const std::string&                               id,
                const XERCES_CPP_NAMESPACE_QUALIFIER Attributes& attrs)
  const;

  std::string getValue(const std::string& id,
                       const XERCES_CPP_NAMESPACE_QUALIFIER Attributes&
                       attrs) const;

 private:
  std::string transcode(const XMLCh* const qname) const;

  void error(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e);

  void fatalError(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e);

  void warning(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e);
};
}

#endif // ifndef DATA_XML_HANDLER_H
