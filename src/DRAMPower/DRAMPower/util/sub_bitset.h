#ifndef DRAMPOWER_UTIL_SUB_BITSET
#define DRAMPOWER_UTIL_SUB_BITSET

#include <DRAMPower/Types.h>

#include <bitset>
#include <cmath>
#include <cassert>
#include <bitset>
#include <stdexcept>

namespace DRAMPower::util
{

template <std::size_t max_width>
class sub_bitset
{
public:
    using buffer_t = std::bitset<max_width>;
private:
    std::size_t width = 0;
    buffer_t buffer;
    buffer_t mask;
public:
    explicit sub_bitset(std::size_t width)
    : width(width)
    , buffer(0)
    {
        assert(width <= max_width);
        if (width > max_width) {
            throw std::out_of_range("Width exceeds maximum width");
        }
        makeMask();
    }
    explicit sub_bitset(std::size_t width, uint64_t value)
        : width(width)
        , buffer(0)
    {
        assert(width <= max_width);
        if (width > max_width) {
            throw std::out_of_range("Width exceeds maximum width");
        }
        uint64_t accumulator = 1;
        for (std::size_t bit_index = 0; bit_index < width; ++bit_index) {
            buffer.set(bit_index, value & accumulator);
            accumulator <<= 1;
        }
        makeMask();
    }
    sub_bitset() = default; // width is 0
public:
    sub_bitset(const sub_bitset&) = default;
    sub_bitset(sub_bitset&&) noexcept = default;

    sub_bitset& operator=(const sub_bitset&) = default;
    sub_bitset& operator=(sub_bitset&&) noexcept = default;
public:
    inline void makeMask() {
        mask.reset();
        for (std::size_t i = 0; i < width; ++i) {
            mask.set(i);
        }
    }
    inline std::size_t size() const { return width; };
    inline std::size_t count() const {
        return buffer.count();
    }
public:
    inline void set(std::size_t n, bool value) {
        assert(n < width);
        if (n >= width) {
            throw std::out_of_range("Index out of range");
        }
        buffer.set(n, value);
    }
    inline void set() {
        buffer.set();
        buffer &= mask;
    }
    inline void reset() {
        buffer.reset();
    }
    inline void flip() {
        buffer.flip();
        buffer &= mask;
    }
    inline void flip(std::size_t n) {
        assert(n < width);
        if (n >= width) {
            throw std::out_of_range("Index out of range");
        }
        buffer.flip(n);
    }
public:
    inline auto operator[](std::size_t n) { 
        assert(n < width);
        if (n >= width) {
            throw std::out_of_range("Index out of range");
        }
        return buffer.test(n);
    }
    inline auto operator[](std::size_t n) const {
        assert(n < width);
        if (n >= width) {
            throw std::out_of_range("Index out of range");
        }
        return buffer.test(n);
    }
public:
    template <std::size_t max_width_rhs>
    bool operator==(const sub_bitset<max_width_rhs>& rhs) const {
        if (this->size() != rhs.size()) {
            return false;
        }
        return buffer == rhs.buffer;
    }
    template <std::size_t max_width_rhs>
    bool operator!=(const sub_bitset<max_width_rhs>& rhs) const {
        return !(*this == rhs);
    }
    bool operator==(unsigned long rhs) const {
        return *this == sub_bitset<max_width> { this->size(), rhs };
    }
    bool operator!=(unsigned long rhs) const {
        return !(*this == rhs);
    }
public: // bitset operations
    sub_bitset operator~() const {
        auto result = *this;
        result.buffer = ~buffer;
        result.buffer &= mask;
        return result;
    }
    template <std::size_t max_width_rhs>
    auto& operator^=(const sub_bitset<max_width_rhs>& rhs) {
        assert(this->size() == rhs.size());
        buffer ^= rhs.buffer;
        buffer &= mask;
        return *this;
    }
    template <std::size_t max_width_rhs>
    auto& operator&=(const sub_bitset<max_width_rhs>& rhs) {
        assert(this->size() == rhs.size());
        buffer &= rhs.buffer;
        return *this;
    }
    template <std::size_t max_width_rhs>
    auto& operator|=(const sub_bitset<max_width_rhs>& rhs) {
        assert(this->size() == rhs.size());
        buffer |= rhs.buffer;
        buffer &= mask;
        return *this;
    }
public:
};

template<std::size_t max_width>
inline sub_bitset<max_width> operator^(sub_bitset<max_width> lhs, const sub_bitset<max_width>& rhs) { return lhs ^= rhs; };
template<std::size_t max_width>
inline sub_bitset<max_width> operator&(sub_bitset<max_width> lhs, const sub_bitset<max_width>& rhs) { return lhs &= rhs; };
template<std::size_t max_width>
inline sub_bitset<max_width> operator|(sub_bitset<max_width> lhs, const sub_bitset<max_width>& rhs) { return lhs |= rhs; };

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_SUB_BITSET */
