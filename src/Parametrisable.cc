/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include "Parametrisable.h"

#include <iostream>
#include <cstdlib>

using namespace Data;
using namespace std;

void Parametrisable::pushParameter(const Parameter& parameter) {
    parameters.push_back(parameter);
}

void Parametrisable::setParameter(const Parameter& parameter,
        unsigned int index) {
    unsigned int count = 0;
    vector<Parameter>::iterator p = parameters.begin();

    while (p != parameters.end() && !(p->getId() == parameter.getId() &&
            index == count)) {
        if (p->getId() == parameter.getId())
            ++count;
        ++p;
    }

    if (p == parameters.end()) {
        parameters.push_back(parameter);
    } else {
        p = parameters.erase(p);
        parameters.insert(p, parameter);
    }
}

bool Parametrisable::removeParameter(const string& id, unsigned int index) {
    unsigned int count = 0;
    for (vector<Parameter>::iterator p = parameters.begin();
            p != parameters.end(); ++p) {
        if (p->getId() == id && index == count++) {
            parameters.erase(p);
            return true;
        }
    }

    return false;
}

/**
 * Get a parameter with a specific id. Should there be a multiplicity,
 * then the index is used to determine which instance is returned, in
 * order traversal.
 */
Parameter Parametrisable::getParameter(const string& id,
        unsigned int index) const {
    unsigned int count = 0;
    for (vector<Parameter>::const_iterator p = parameters.begin();
            p != parameters.end(); ++p) {
        if (p->getId() == id && index == count++) {
            return *p;
        }
    }

    cerr << "Could not find parameter '" << id << "' (" << index << ")" << endl;
    cerr << "Stored parameters are: " << endl;
    for (vector<Parameter>::const_iterator p = parameters.begin();
            p != parameters.end(); ++p) {
        cerr << "   " << p->getId() << ": " << p->getValue() << endl;
    }
    exit(1);

    return Parameter("", "", "");
}

vector<Parameter> Parametrisable::getParameters() const {
    return parameters;
}

bool Parametrisable::hasParameter(const string& id, unsigned int index) const {
    unsigned int count = 0;
    for (vector<Parameter>::const_iterator p = parameters.begin();
            p != parameters.end(); ++p) {
        if (p->getId() == id && index == count++) {
            return true;
        }
    }

    return false;
}
