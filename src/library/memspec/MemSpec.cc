/*
 * Copyright (c) 2015, University of Kaiserslautern
 * Copyright (c) 2012-2020, Fraunhofer IESE
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

using json = nlohmann::json;

MemSpec::MemSpec(nlohmann::json &memspec,
                 const bool debug,
                 const bool writeToConsole,
                 const bool writeToFile,
                 const std::string &traceName)
{
    setupDebugManager(debug, writeToConsole, writeToFile, traceName);
    numberOfBanks=parseUint(memspec["memarchitecturespec"]["nbrOfBanks"],"nbrOfBanks");
    numberOfRows = (parseUint(memspec["memarchitecturespec"]["nbrOfRows"],"nbrOfRows"));
    numberOfColumns = (parseUint(memspec["memarchitecturespec"]["nbrOfColumns"],"nbrOfColumns"));
    burstLength = (parseUint(memspec["memarchitecturespec"]["burstLength"],"burstLength"));
    dataRate = (parseUint(memspec["memarchitecturespec"]["dataRate"],"dataRate"));
    bitWidth = (parseUint(memspec["memarchitecturespec"]["width"],"width"));
    memoryId = (parseString(memspec["memoryId"], "memoryId"));
    memoryType = (parseString(memspec["memoryType"], "memoryType"));
}



//memArchSpec.numberOfChannels = parseUint(memspec["memarchitecturespec"]["nbrOfChannels"],"nbrOfChannels");
//memArchSpec.numberOfRanks=parseUint(memspec["memarchitecturespec"]["nbrOfRanks"],"nbrOfRanks");
//memArchSpec.numberOfDevicesOnDIMM = parseUint(memspec["memarchitecturespec"]["nbrOfDevicesOnDIMM"],"nbrOfDevicesOnDIMM");
//memArchSpec.numberOfBankGroups = parseUint(memspec["memarchitecturespec"]["nbrOfBankGroups"],"nbrOfBankGroups");
//memArchSpec.banksPerRank = memArchSpec.numberOfBanks/memArchSpec.numberOfRanks;
//memArchSpec.groupsPerRank = memArchSpec.numberOfBankGroups / memArchSpec.numberOfRanks;
//memArchSpec.banksPerGroup = memArchSpec.numberOfBanks / memArchSpec.numberOfBankGroups;



MemSpec::~MemSpec()
{
}

bool MemSpec::parseBool(json &obj, std::string name)
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

bool MemSpec::parseBoolWithDefault(json &obj, std::string name)
{
    if (!obj.empty()) {
        if (obj.is_boolean())
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': bool");
    }
    else
        return false;
}

unsigned MemSpec::parseUint(json &obj, std::string name)
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

unsigned MemSpec::parseUintWithDefaut(json &obj, std::string name)
{
    if (!obj.empty()) {
        if (obj.is_number() && (obj > 0))
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': unsigned int");
    }
    else {
        PRINTWARNING("Parameter " + name + " not found: parsed with zero.\n");
        return 0.0;
    }
}

double MemSpec::parseUdouble(json &obj, std::string name)
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

double MemSpec::parseUdoubleWithDefault(json &obj, std::string name)
{
    if (!obj.empty()) {
        if (obj.is_number() && (obj >= 0))
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': positive double");
    }
    else {
        PRINTWARNING("Parameter " + name + " not found: parsed with zero.\n");
        return 0.0;
    }
}
std::string MemSpec::parseString(json &obj, std::string name)
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

std::string MemSpec::parseStringWithDefault(json &obj, std::string name, std::string defaultString)
{
    if (!obj.empty()) {
        if (obj.is_string())
            return obj;
        else
            throw std::invalid_argument("Expected type for '" + name + "': string");
    }
    else
        PRINTWARNING("Parameter " + name + " not found: parsed with default.\n");
    return defaultString;
}


int64_t MemSpec::getExitSREFtime()
{
    throw std::invalid_argument("getExitSREFtime was not declared");
}

void MemSpec::setupDebugManager(    const bool debug __attribute__((unused)),
                                    const bool writeToConsole __attribute__((unused)),
                                    const bool writeToFile __attribute__((unused)),
                                    const std::string &traceName __attribute__((unused)))

{
#ifndef NDEBUG
    auto &dbg = DebugManager::getInstance();
    dbg.debug = debug;
    dbg.writeToConsole = writeToConsole;
    dbg.writeToFile = writeToFile;
    if (dbg.writeToFile && (traceName!=""))
        dbg.openDebugFile(traceName + ".txt");
#endif
}

