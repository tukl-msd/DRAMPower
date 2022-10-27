#include "binary_ops.h"

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

std::size_t BinaryOps::popcount(uint64_t n)
{
    return POPCOUNT(n);
}

std::size_t BinaryOps::zero_to_ones(uint64_t p, uint64_t q)
{
    return popcount(~p & q);
};

std::size_t BinaryOps::one_to_zeroes(uint64_t p, uint64_t q)
{
    return popcount(p & ~q);
};

std::size_t BinaryOps::bit_changes(uint64_t p, uint64_t q)
{
    return popcount(p ^ q);
};
};
