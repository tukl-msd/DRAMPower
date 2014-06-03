/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#ifndef DATA_PARAMETER_H
#define DATA_PARAMETER_H

#include <string>
#include <ostream>

namespace Data {

    class Parameter {
    public:

        Parameter(const std::string& id, const std::string& type,
                const std::string& value);

        std::string getId() const;
        std::string getType() const;
        std::string getValue() const;

        int getIntValue() const;
        unsigned int getUIntValue() const;
        size_t getSizeTValue() const;
        double getDoubleValue() const;
        bool getBoolValue() const;

        operator int() const {
            return getIntValue();
        }

        operator unsigned int() const {
            return getUIntValue();
        }

#ifdef _LP64

operator size_t() const {
            return getSizeTValue();
        }
#endif

        operator double() const {
            return getDoubleValue();
        }

        operator bool() const {
            return getBoolValue();
        }

        operator std::string() const {
            return getValue();
        }

    private:

        std::string id;
        std::string type;
        std::string value;

    };

    Parameter HexParameter(const std::string& id, int value);

    Parameter StringParameter(const std::string& id, const std::string& value);

    std::ostream& operator<<(std::ostream& os,
            const Parameter& parameter);
}
#endif
