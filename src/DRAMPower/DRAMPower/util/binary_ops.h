#ifndef DRAMPOWER_UTIL_BINARY_OPS_H
#define DRAMPOWER_UTIL_BINARY_OPS_H

#include <sstream>
#include <bitset>

#if defined(__GNUC__)
	#define POPCOUNT __builtin_popcountll
#elif defined(_MSC_VER)
	#include <nmmintrin.h>
	#if defined(_M_X64)
		#define POPCOUNT _mm_popcnt_u64
	#else
		#define POPCOUNT _mm_popcnt_u32
	#endif
#elif defined(__cpp_lib_bitops)
	#include <bit>
	#define POPCOUNT std::popcount
#else
	#include <bitset>
	inline std::size_t popcount_(uint64_t n) { return std::bitset<64>(n).count(); };
	#define POPCOUNT popcount_
#endif

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
    template <typename T>
    static std::size_t popcount(const T& bitset) {
        return POPCOUNT(bitset);
    }

    template <std::size_t N>
    static std::size_t popcount(const std::bitset<N>& bitset)
    {
        return bitset.count();
    }

    template <typename T>
    static std::size_t zero_to_ones(const T& p, const T& q)
    {
        return popcount(~p & q);
    }

    template <typename T>
    static std::size_t one_to_zeroes(const T& p, const T& q)
    {
        return popcount(p & ~q);
    }

    template <typename T>
    static std::size_t bit_changes(const T& p, const T& q)
    {
        return popcount(p ^ q);
    }
};

};

#endif /* DRAMPOWER_UTIL_BINARY_OPS_H */
