#ifndef UTIL_HPP
#define UTIL_HPP


#include <stdint.h>
#include "csv.hpp"

// Util calls for util functions
namespace CLIutil {
    // Util function to get the memory
    std::unique_ptr<uint8_t[]> hexStringToUint8Array(const csv::string_view data, size_t &size);
}

#endif