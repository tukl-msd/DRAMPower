/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include "XMLParser.h"

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>

#include <iostream>

XERCES_CPP_NAMESPACE_USE

using namespace std;

void XMLParser::parse(const string& filename, DefaultHandler* handler) {
    try
    {
        XMLPlatformUtils::Initialize();
    }

    catch(const XMLException & toCatch) {
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

    catch(const XMLException & toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cerr << "XMLException in " << toCatch.getSrcFile() << " at line " <<
                toCatch.getSrcLine() << ":\n" << message << "\n";
        XMLString::release(&message);
        exit(1);
    }

    catch(const SAXParseException & toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cerr << "SAXParseException at line " <<
                toCatch.getLineNumber() << ", column " << 
				toCatch.getColumnNumber() << ":\n" << message << "\n";
        XMLString::release(&message);
        exit(1);
    }

    catch(const exception & e) {
        cerr << "Exception during XML parsing:" << endl
                << e.what() << endl;
        exit(1);
    }

    catch(...) {
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
}
