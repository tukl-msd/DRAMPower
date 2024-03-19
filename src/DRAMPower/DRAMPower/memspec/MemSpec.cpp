/*
 * Copyright (c) 2015, University of Kaiserslautern
 * Copyright (c) 2012-2021, Fraunhofer IESE
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *    Lukas Steiner
 */

#include "MemSpec.h"


using namespace DRAMPower;
using json = nlohmann::json;

MemSpec::MemSpec(nlohmann::json &memspec)
{
    numberOfBanks = parseUint(memspec["memarchitecturespec"]["nbrOfBanks"],"nbrOfBanks");
    numberOfRows = (parseUint(memspec["memarchitecturespec"]["nbrOfRows"],"nbrOfRows"));
    numberOfColumns = (parseUint(memspec["memarchitecturespec"]["nbrOfColumns"],"nbrOfColumns"));
    numberOfDevices = parseUintWithDefaut(memspec["memarchitecturespec"]["nbrOfDevices"], "nbrOfDevices", 1); // default to 1 device
    burstLength = (parseUint(memspec["memarchitecturespec"]["burstLength"],"burstLength"));
    dataRate = (parseUint(memspec["memarchitecturespec"]["dataRate"],"dataRate"));
    bitWidth = (parseUint(memspec["memarchitecturespec"]["width"],"width"));
    memoryId = (parseString(memspec["memoryId"], "memoryId"));
    memoryType = (parseString(memspec["memoryType"], "memoryType"));
}


bool MemSpec::parseBool(json &obj, const std::string & name)
{
    if (!obj.empty()) {
        if (obj.is_boolean())
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': bool");
    }
    else
        throw std::invalid_argument("Query json: Parameter " + name + "' not found");
}

bool MemSpec::parseBoolWithDefault(json &obj, const std::string & name, bool def)
{
    if (!obj.empty()) {
        if (obj.is_boolean())
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': bool");
    }
    else
        return def;
}

uint64_t MemSpec::parseUint(json &obj, const std::string & name)
{
    if (!obj.empty()) {
        if (obj.is_number_unsigned())
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': unsigned int");
    }
    else

        throw std::invalid_argument("Query json: Parameter " + name + "' not found");
}

uint64_t MemSpec::parseUintWithDefaut(json &obj, const std::string & name, uint64_t default_value)
{
    if (!obj.empty()) {
        if (obj.is_number() && (obj > 0))
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': unsigned int");
    }
    else {
        //PRINTWARNING("Parameter " + name + " not found: parsed with zero.\n");
        return default_value;
    }
}

double MemSpec::parseUdouble(json &obj, const std::string & name)
{
    if (!obj.empty()) {
        if (obj.is_number() && (obj > 0))
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': positive double");
    }
    else

        throw std::invalid_argument("Query json: Parameter " + name + "' not found");
}

double MemSpec::parseUdoubleWithDefault(json &obj, const std::string & name)
{
    if (!obj.empty()) {
        if (obj.is_number() && (obj >= 0))
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': positive double");
    }
    else {
        //PRINTWARNING("Parameter " + name + " not found: parsed with zero.\n");
        return 0.0;
    }
}
std::string MemSpec::parseString(json &obj, const std::string & name)
{
    if (!obj.empty()) {
        if (obj.is_string())
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': string");
    }
    else
        throw std::invalid_argument("Query json: Parameter " + name + "' not found");
}

std::string MemSpec::parseStringWithDefault(json &obj, const std::string & name, const std::string & defaultString)
{
    if (!obj.empty()) {
        if (obj.is_string())
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': string");
    }
	else {
		return defaultString;
	}
}
