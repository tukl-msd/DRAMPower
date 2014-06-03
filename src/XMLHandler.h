/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
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

        void startElement(const XMLCh * const namespace_uri,
                const XMLCh * const localname,
                const XMLCh * const qname,
                const XERCES_CPP_NAMESPACE_QUALIFIER Attributes& attrs);

        void endElement(const XMLCh * const namespace_uri,
                const XMLCh * const localname,
                const XMLCh * const qname);

    protected:

        virtual void startElement(const std::string& name,
                const XERCES_CPP_NAMESPACE_QUALIFIER Attributes&
                attrs) = 0;

        virtual void endElement(const std::string& name) = 0;

        bool hasValue(const std::string& id,
                const XERCES_CPP_NAMESPACE_QUALIFIER Attributes& attrs)
        const;

        std::string getValue(const std::string& id,
                const XERCES_CPP_NAMESPACE_QUALIFIER Attributes&
                attrs) const;

    private:

        std::string transcode(const XMLCh * const qname) const;

        void error(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e);

        void fatalError(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e);

        void warning(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e);
    };
}

#endif
