#ifndef UTIL_HPP
#define UTIL_HPP


#include <stdint.h>
#include "csv.hpp"

namespace DRAMPower::CLI::util {

    // Util function to get the memory
    std::unique_ptr<uint8_t[]> hexStringToUint8Array(const csv::string_view data, size_t &size);

} // namespace DRAMPower::CLI::util


#endif