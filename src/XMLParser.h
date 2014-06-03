/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#ifndef XML_PARSER_H
#define XML_PARSER_H

#include <string>

#include <xercesc/sax2/DefaultHandler.hpp>

class XMLParser {
private:

    /**
     * Protect people from themselves. This class should never be
     * instantiated, nor copied.
     */
    XMLParser();
    XMLParser(const XMLParser&);
    XMLParser& operator=(const XMLParser&);

public:
    /**
     * General parsing function that uses the specified handler on the
     * file in question.
     */
    static void parse(const std::string& filename,
            XERCES_CPP_NAMESPACE_QUALIFIER DefaultHandler* handler);

};
#endif
