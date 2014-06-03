/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
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

XMLHandler::XMLHandler() {
}

XMLHandler::~XMLHandler() {
}

void XMLHandler::startElement(const XMLCh * const,
        const XMLCh * const,
        const XMLCh * const qname,
        const Attributes& attrs) {
    string name = transcode(qname);

    startElement(name, attrs);
}

void XMLHandler::endElement(const XMLCh * const,
        const XMLCh * const,
        const XMLCh * const qname) {
    string name = transcode(qname);

    endElement(name);
}

string XMLHandler::transcode(const XMLCh * const qname) const {
    char *buf = XMLString::transcode(qname);
    string name(buf);
    delete buf; //  In xerces 2.2.0: XMLString::release (&value);
    return name;
}

bool XMLHandler::hasValue(const string& id,
        const Attributes& attrs) const {
    XMLCh* attr = XMLString::transcode(id.c_str());
    const XMLCh* value = attrs.getValue(attr);
    XMLString::release(&attr);
    return value != NULL;
}

string XMLHandler::getValue(const string& id,
        const Attributes& attrs) const {
    XMLCh* attr = XMLString::transcode(id.c_str());
    const XMLCh* value = attrs.getValue(attr);
    XMLString::release(&attr);
    assert(value != NULL);
    return transcode(value);
}

void XMLHandler::error(const SAXParseException& e) {
    char* file = XMLString::transcode(e.getSystemId());
    char* message = XMLString::transcode(e.getMessage());

    cerr << "\nError at file " << file
            << ", line " << e.getLineNumber()
            << ", char " << e.getColumnNumber()
            << "\n  Message: " << message << endl;

    XMLString::release(&file);
    XMLString::release(&message);
}

void XMLHandler::fatalError(const SAXParseException& e) {
    char* file = XMLString::transcode(e.getSystemId());
    char* message = XMLString::transcode(e.getMessage());

    cerr << "\nFatal Error at file " << file
            << ", line " << e.getLineNumber()
            << ", char " << e.getColumnNumber()
            << "\n  Message: " << message << endl;

    XMLString::release(&file);
    XMLString::release(&message);
}

void XMLHandler::warning(const SAXParseException& e) {
    char* file = XMLString::transcode(e.getSystemId());
    char* message = XMLString::transcode(e.getMessage());

    cerr << "\nWarning at file " << file
            << ", line " << e.getLineNumber()
            << ", char " << e.getColumnNumber()
            << "\n  Message: " << message << endl;

    XMLString::release(&file);
    XMLString::release(&message);
}
