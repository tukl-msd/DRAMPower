#ifndef DRAMPOWER_UTIL_BINARY_OPS_H
#define DRAMPOWER_UTIL_BINARY_OPS_H

#include <algorithm>
#include <memory>
#include <vector>

#include <bitset>
#include <sstream>

namespace DRAMPower::util {

template <typename T>
inline std::string to_string(const T& bitset)
{
    std::stringstream ss;
    ss << '[';
    for (std::size_t i = bitset.size(); i > 0;) {
        ss << bitset[--i] ? '1' : '0';
    }
    ss << ']';

    return ss.str();
};

struct BinaryOps {
    static std::size_t popcount(uint64_t n);
    static std::size_t zero_to_ones(uint64_t p, uint64_t q);
    static std::size_t one_to_zeroes(uint64_t p, uint64_t q);
    static std::size_t bit_changes(uint64_t p, uint64_t q);

    // overloadsfor std::bitset
    template <typename T>
    static std::size_t popcount(const T& bitset)
    {
        return bitset.count();
    };

    template <typename T>
    static std::size_t zero_to_ones(const T& p, const T& q)
    {
        return popcount(~p & q);
    };

    template <typename T>
    static std::size_t one_to_zeroes(const T& p, const T& q)
    {
        return popcount(p & ~q);
    };

    template <typename T>
    static std::size_t bit_changes(const T& p, const T& q)
    {
        return popcount(p ^ q);
    };
};

};

#endif /* DRAMPOWER_UTIL_BINARY_OPS_H */
