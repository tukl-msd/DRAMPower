#ifndef UTIL_HPP
#define UTIL_HPP


#include <stdint.h>
#include "csv.hpp"

// Util calls for util functions
namespace CLIutil {
    // Util function to get the memory
    inline bool hexCharToInt(char hexChar, uint8_t &result);
    bool hexStringToUint8Array(const csv::string_view data, std::unique_ptr<uint8_t[]> &arr, std::size_t &arraySize);
}

#endif