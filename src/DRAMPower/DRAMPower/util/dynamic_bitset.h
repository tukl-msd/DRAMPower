#ifndef DRAMPOWER_UTIL_DYNAMIC_BITSET_H
#define DRAMPOWER_UTIL_DYNAMIC_BITSET_H

#include <DRAMPower/Types.h>

#include <optional>
#include <bitset>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cassert>
#include <bitset>
#include <type_traits>

namespace DRAMPower::util
{

template <std::size_t blocksize>
class dynamic_bitset
{
public:
    using buffer_element_t = std::bitset<blocksize>;
    using buffer_t = std::vector<buffer_element_t>;
private:
    std::size_t num_bits = 0;
    std::bitset<blocksize> lastelementmask;
    buffer_t buffer;
public:
    explicit dynamic_bitset(std::size_t num_bits)
    : num_bits(num_bits)
    , buffer(static_cast<std::size_t>((0 != num_bits % blocksize) ? ((num_bits / blocksize) + 1) : (num_bits / blocksize)), 0)
    {
        makeLastElementMask();
    }
    explicit dynamic_bitset(std::size_t num_bits, uint64_t value)
        : num_bits(num_bits)
        , buffer(static_cast<std::size_t>((0 != num_bits % blocksize) ? ((num_bits / blocksize) + 1) : (num_bits / blocksize)))
    {
        uint64_t accumulator = 1;
        for (std::size_t bit_index = 0; bit_index < num_bits; ++bit_index) {
            buffer[bit_index / blocksize][bit_index % blocksize] = value & accumulator;
            accumulator <<= 1;
        }
        lastelementmask = std::bitset<blocksize>(0);
        for (std::size_t i = 0; i < num_bits % blocksize; ++i) {
            lastelementmask.set(i);
        }
    }
    dynamic_bitset() = default;
public:
    dynamic_bitset(const dynamic_bitset&) = default;
    dynamic_bitset(dynamic_bitset&&) noexcept = default;

    dynamic_bitset& operator=(const dynamic_bitset&) = default;
    dynamic_bitset& operator=(dynamic_bitset&&) noexcept = default;
public:
    inline void makeLastElementMask() {
        lastelementmask.reset();
        for (std::size_t i = 0; i < num_bits % blocksize; ++i) {
            lastelementmask.set(i);
        }
    }
    inline std::size_t size() const { return num_bits; };
    std::size_t count() const {
        std::size_t n = 0;
        // Iterate over all block including the last block which can be smaller than the block_size
        std::size_t i_blocks = blocksize;
        for (auto &element : buffer) {
            if (i_blocks > num_bits) {
                n += (element & lastelementmask).count();
            } else {
                n += element.count();
            } 
            i_blocks += blocksize;
        }
        return n;
    }
public: // element manipulation
    void push_back(bool bit) {
        if (num_bits % blocksize == 0) {
            // New entry
            buffer.push_back(buffer_element_t{0});
        }
        buffer.back().set(num_bits % blocksize, bit);
        num_bits++;
        makeLastElementMask();
    }
    void set(std::size_t n, bool value) {
        buffer.at(n / blocksize).set(n % blocksize, value);
    }
    void set() {
        for (auto &element : buffer) {
            element.set();
        }
    }
    void reset() {
        for (auto &element : buffer) {
            element.reset();
        }
    }
    void flip() {
        for (auto &element : buffer) {
            element.flip();
        }
    }
    void flip(std::size_t n) {
        buffer.at(n / blocksize).flip(n % blocksize);
    }
    void clear() {
        buffer.clear();
        num_bits = 0;
    }
public: // element access
    // inline auto at(std::size_t n) -> decltype(buffer.at(n)) { return buffer.at(n); }
    // inline auto at(std::size_t n) -> decltype(buffer.at(n)) const { return buffer.at(n); }
public:
    inline auto operator[](std::size_t n) { return buffer.at(n / blocksize).test(n % blocksize); };
    inline auto operator[](std::size_t n) const { return buffer.at(n / blocksize).test(n % blocksize); };
public: // comparison operations
    template <std::size_t blocksize_rhs, std::enable_if_t<blocksize == blocksize_rhs, int> = 0>
    bool operator==(const dynamic_bitset<blocksize_rhs>& rhs) const {
        if (this->size() != rhs.size()) {
            return false;
        }
    
        for (std::size_t i = 0; i < this->buffer.size(); ++i) {
            if (buffer.at(i) != rhs.buffer.at(i)) {
                return false;
            }
        }
    
        return true;
    }
    template <std::size_t blocksize_rhs, std::enable_if_t<blocksize != blocksize_rhs, int> = 0>
    bool operator==(const dynamic_bitset<blocksize_rhs>& rhs) const {
        if (this->size() != rhs.size()) {
            return false;
        }
    
        for (std::size_t i = 0; i < this->size(); ++i) {
            if (this->operator[](i) != rhs[i]) {
                return false;
            }
        }
    
        return true;
    }
    template <std::size_t blocksize_rhs>
    bool operator!=(const dynamic_bitset<blocksize_rhs>& rhs) const {
        return !(*this == rhs);
    }
    bool operator==(unsigned long rhs) const {
        return *this == dynamic_bitset<blocksize> { this->size(), rhs };
    }
    bool operator!=(unsigned long rhs) const {
        return !(*this == rhs);
    }
public: // bitset operations
    dynamic_bitset operator~() const {
        auto bitset = *this;
    
        for (std::size_t i = 0; i < this->buffer.size(); ++i) {
            bitset.buffer.at(i) = ~buffer.at(i);
        };
    
        return bitset;
    }
    dynamic_bitset& operator^=(const dynamic_bitset& rhs) {
        assert(this->size() == rhs.size());
    
        for (std::size_t i = 0; i < this->buffer.size(); ++i) {
            buffer.at(i) = buffer.at(i) ^ rhs.buffer.at(i);
        };
    
        return *this;
    }
    dynamic_bitset& operator&=(const dynamic_bitset& rhs) {
        assert(this->size() == rhs.size());
    
        for (std::size_t i = 0; i < this->buffer.size(); ++i) {
            buffer.at(i) &= rhs.buffer.at(i);
        };
    
        return *this;
    }
    dynamic_bitset& operator|=(const dynamic_bitset& rhs) {
        assert(this->size() == rhs.size());
    
        for (std::size_t i = 0; i < this->buffer.size(); ++i) {
            buffer.at(i) |= rhs.buffer.at(i);
        };
    
        return *this;
    }
public:
};

template<std::size_t blocksize>
inline dynamic_bitset<blocksize> operator^(dynamic_bitset<blocksize> lhs, const dynamic_bitset<blocksize>& rhs) { return lhs ^= rhs; };
template<std::size_t blocksize>
inline dynamic_bitset<blocksize> operator&(dynamic_bitset<blocksize> lhs, const dynamic_bitset<blocksize>& rhs) { return lhs &= rhs; };
template<std::size_t blocksize>
inline dynamic_bitset<blocksize> operator|(dynamic_bitset<blocksize> lhs, const dynamic_bitset<blocksize>& rhs) { return lhs |= rhs; };

}

#endif /* DRAMPOWER_UTIL_DYNAMIC_BITSET_H */
