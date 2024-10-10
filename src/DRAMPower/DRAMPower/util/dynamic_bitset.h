#ifndef DRAMPOWER_UTIL_DYNAMIC_BITSET_H
#define DRAMPOWER_UTIL_DYNAMIC_BITSET_H

#include <DRAMPower/Types.h>

#include <optional>
#include <bitset>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cassert>

namespace DRAMPower::util
{

class dynamic_bitset
{
public:
    using buffer_t = std::vector<bool>;
private:
    buffer_t buffer;
public:
    explicit dynamic_bitset();
    explicit dynamic_bitset(std::size_t num_bits);
    explicit dynamic_bitset(std::size_t num_bits, uint64_t value = 0);
public:
    dynamic_bitset(const dynamic_bitset&) = default;
    dynamic_bitset(dynamic_bitset&&) noexcept = default;

    dynamic_bitset& operator=(const dynamic_bitset&) = default;
    dynamic_bitset& operator=(dynamic_bitset&&) noexcept = default;
public:
    inline std::size_t size() const { return buffer.size(); };
    std::size_t count() const;
public: // element manipulation
    void push_back(bool bit);
    void clear();
    void flip(std::size_t n);
public: // element access
    inline auto at(std::size_t n) { return buffer.at(n); }
    inline auto at(std::size_t n) const { return buffer.at(n); }
public:
    inline auto operator[](std::size_t n) { return this->at(n); };
    inline auto operator[](std::size_t n) const { return this->at(n); };
public: // comparison operations
    bool operator==(const dynamic_bitset&) const;
    bool operator!=(const dynamic_bitset&) const;
    bool operator==(unsigned long) const;
    bool operator!=(unsigned long) const;
public: // bit operations
    dynamic_bitset operator~() const;
    dynamic_bitset& operator^=(const dynamic_bitset& rhs);
    dynamic_bitset& operator&=(const dynamic_bitset& rhs);
    dynamic_bitset& operator|=(const dynamic_bitset& rhs);
public:
};

inline dynamic_bitset operator^(dynamic_bitset lhs, const dynamic_bitset& rhs) { return lhs ^= rhs; };
inline dynamic_bitset operator&(dynamic_bitset lhs, const dynamic_bitset& rhs) { return lhs &= rhs; };
inline dynamic_bitset operator|(dynamic_bitset lhs, const dynamic_bitset& rhs) { return lhs |= rhs; };

}

#endif /* DRAMPOWER_UTIL_DYNAMIC_BITSET_H */
