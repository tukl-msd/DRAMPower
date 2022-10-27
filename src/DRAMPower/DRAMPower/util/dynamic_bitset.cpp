#include "dynamic_bitset.h"

namespace DRAMPower::util {

dynamic_bitset::dynamic_bitset()
{
    buffer.reserve(32);
};

dynamic_bitset::dynamic_bitset(std::size_t num_bits)
    : buffer(num_bits, 0) {};

dynamic_bitset::dynamic_bitset(std::size_t num_bits, unsigned long value)
    : buffer(num_bits)
{
    std::size_t accumulator = 1;

    for (std::size_t i = 0; i < num_bits; ++i) {
        this->buffer[i] = value & accumulator;
        accumulator *= 2;
    };
};

void dynamic_bitset::push_back(bool v)
{
    this->buffer.push_back(v);
};

void dynamic_bitset::clear()
{
    this->buffer.clear();
};

void dynamic_bitset::flip(std::size_t n)
{
    for (std::size_t i = 0; i < this->size(); ++i) {
        this->at(n) = !this->at(n);
    }
};

std::size_t dynamic_bitset::count() const
{
    std::size_t n = 0;

    for (std::size_t i = 0; i < this->size(); ++i) {
        if (this->at(i))
            ++n;
    }

    return n;
};

bool dynamic_bitset::operator==(const dynamic_bitset& rhs) const
{
    if (this->size() != rhs.size())
        return false;

    for (std::size_t i = 0; i < this->size(); ++i) {
        if (this->at(i) != rhs.at(i))
            return false;
    };

    return true;
};

bool dynamic_bitset::operator!=(const dynamic_bitset& rhs) const
{
    return !(*this == rhs);
};

bool dynamic_bitset::operator==(unsigned long rhs) const
{
    return *this == dynamic_bitset { this->size(), rhs };
};

bool dynamic_bitset::operator!=(unsigned long rhs) const
{
    return !(*this == rhs);
};

dynamic_bitset dynamic_bitset::operator~() const
{
    auto bitset = *this;

    for (std::size_t i = 0; i < this->size(); ++i) {
        bitset.at(i) = !this->at(i);
    };

    return bitset;
};

dynamic_bitset& dynamic_bitset::operator^=(const dynamic_bitset& rhs)
{
    assert(this->size() == rhs.size());

    for (std::size_t i = 0; i < this->size(); ++i) {
        this->at(i) = this->at(i) ^ rhs.at(i);
    };

    return *this;
};

dynamic_bitset& dynamic_bitset::operator&=(const dynamic_bitset& rhs)
{
    assert(this->size() == rhs.size());

    for (std::size_t i = 0; i < this->size(); ++i) {
        this->at(i) = this->at(i) && rhs.at(i);
    };

    return *this;
};

dynamic_bitset& dynamic_bitset::operator|=(const dynamic_bitset& rhs)
{
    assert(this->size() == rhs.size());

    for (std::size_t i = 0; i < this->size(); ++i) {
        this->at(i) = this->at(i) || rhs.at(i);
    };

    return *this;
};

};