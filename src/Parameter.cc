/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include "Parameter.h"

#include <iomanip>
#include "Utils.h"

using namespace Data;
using namespace std;

Parameter::Parameter(const string& id, const string& type,
        const string& value) : id(id), type(type), value(value) {
}

string Parameter::getId() const {
    return id;
}

string Parameter::getType() const {
    return type;
}

int Parameter::getIntValue() const {
    return fromString<int>(value);
}

unsigned int Parameter::getUIntValue() const {
    bool isHex = value.size() > 1 && value[0] == '0' && value[1] == 'x';
    return fromString<unsigned int>(value, isHex ? std::hex : std::dec);
}

#ifdef _LP64

size_t Parameter::getSizeTValue() const {
    bool isHex = value.size() > 1 && value[0] == '0' && value[1] == 'x';
    return fromString<size_t > (value, isHex ? std::hex : std::dec);
}
#endif

double Parameter::getDoubleValue() const {
    return fromString<double>(value);
}

bool Parameter::getBoolValue() const {
    return fromString<bool > (value);
}

string Parameter::getValue() const {
    return value;
}

Parameter Data::HexParameter(const string& id, int value) {
    std::ostringstream ss;

    ss << "0x" << hex << setw(8) << setfill('0') << value;

    return Parameter(id, "int", ss.str());
}

Parameter Data::StringParameter(const string& id, const string& value) {
    return Parameter(id, "string", value);
}

ostream& Data::operator << (ostream& os, const Parameter& parameter) {
    os << "<parameter " <<
            "id=\"" << parameter.getId() << "\" " <<
            "type=\"" << parameter.getType() << "\" "
            "value=\"" << parameter.getValue() << "\" />";

    return os;
}
