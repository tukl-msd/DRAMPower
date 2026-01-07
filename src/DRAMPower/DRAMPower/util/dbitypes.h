#ifndef DRAMPOWER_UTIL_DBI_TYPES_H
#define DRAMPOWER_UTIL_DBI_TYPES_H

#include <type_traits>
#include <limits>
#include <vector>
#include <array>
#include <bitset>

namespace DRAMPower::util::types {


// contiguous containers
// non contiguous container fallback
template <typename T>
struct is_contiguous_container : std::false_type {};
// std::vector specialization
template <typename T, typename Alloc>
struct is_contiguous_container<std::vector<T, Alloc>> : std::true_type {};
// std::array specialization
template <typename T, typename std::size_t N>
struct is_contiguous_container<std::array<T, N>> : std::true_type {};
// std::string specialization
template <typename CharT, typename Traits, typename Alloc>
struct is_contiguous_container<std::basic_string<CharT, Traits, Alloc>> : std::true_type {};
// value shortcut
template <typename T>
inline constexpr bool is_contiguous_container_v = is_contiguous_container<T>::value;

// is_bitset
// no bitset fallback
template<typename _Tp>
struct is_bitset : std::false_type{};
// std::bitset specialization
template <std::size_t N>
struct is_bitset<std::bitset<N>> : std::true_type {};
// value shortcut
template<typename _Tp>
inline constexpr bool is_bitset_v = is_bitset<_Tp>::value;

// digit_count
// std::numeric_limits<T> fallback
template <typename T>
struct digit_count {
    static constexpr std::size_t value = std::numeric_limits<T>::digits;
};
// std::bitset specialization
template <std::size_t N>
struct digit_count<std::bitset<N>> {
    static constexpr std::size_t value = N;
};
// value shortcut
template <typename T>
constexpr std::size_t digit_count_v = digit_count<T>::value;


} // namespace DRAMPower::util::types

#endif /* DRAMPOWER_UTIL_DBI_TYPES_H */
