#ifndef DRAMPOWER_UTIL_DBIHELPERS_H
#define DRAMPOWER_UTIL_DBIHELPERS_H

#include <type_traits>
#include <bitset>
#include "DRAMPower/util/dbitypes.h"

namespace DRAMPower::util {


template<typename It1, typename It2>
class ConcatIterator
{
public:
    using value_type        = std::common_type_t<
        typename std::iterator_traits<It1>::value_type,
        typename std::iterator_traits<It2>::value_type>;

    using reference         = std::common_type_t<
        typename std::iterator_traits<It1>::reference,
        typename std::iterator_traits<It2>::reference>;

    using iterator_category = std::input_iterator_tag;

    ConcatIterator(It1 cur1, It1 end1, It2 cur2)
        : m_it1(cur1), m_end1(end1), m_it2(cur2) {}

    reference operator*() const
    {
        return (m_it1 != m_end1) ? *m_it1 : *m_it2;
    }

    ConcatIterator& operator++()
    {
        if (m_it1 != m_end1) {
            ++m_it1;
        } else {
            ++m_it2;
        }
        return *this;
    }

    bool operator==(const ConcatIterator& other) const
    {
        return (m_it1 == other.m_it1) && (m_it2 == other.m_it2) && (m_end1 == other.m_end1);
    }

    bool operator!=(const ConcatIterator& other) const
    {
        return !(*this == other);
    }

private:
    It1 m_it1;
    It1 m_end1;
    It2 m_it2;
};

/** SubChunkView
* SubChunkView is a iterator_value class for iterating over the chunks in a burst if
* the bus width is a multiple of the chunk size. The class itself provides
*/
template <typename iterator, std::size_t CHUNKSIZE>
class SubChunkView {
// Public type definitions, constructors, member functions
public:

    using parent_iterator_value_type = typename iterator::value_type;

    using iterator_category = std::input_iterator_tag;
    using value_type = typename iterator::value_type;
    using difference_type = typename iterator::difference_type;
    using pointer = typename iterator::pointer;
    using reference = typename iterator::reference;

    static constexpr std::size_t DATATYPEDIGITS = types::digit_count_v<parent_iterator_value_type>;
    static_assert(DATATYPEDIGITS > 0, "The BufferType must contain at least one digit");
    static_assert(std::is_integral_v<parent_iterator_value_type> || std::is_same_v<parent_iterator_value_type, std::bitset<CHUNKSIZE>>, "Parent iterator value type must be an integral type or a std::bitset of the chunk size");
    static_assert(0 == (CHUNKSIZE % DATATYPEDIGITS), "CHUNKSIZE must be a multiple of DATATYPEDIGITS.");
    static_assert(CHUNKSIZE >= DATATYPEDIGITS, "CHUNKSIZE must be greater or equal to DATATYPEDIGITS");

    static constexpr std::size_t COUNT_DATA = CHUNKSIZE / DATATYPEDIGITS;

private:
    SubChunkView(iterator it, std::size_t current_chunk_idx, std::size_t current_data_idx)
        : m_it(it)
        , m_current_chunk_idx(current_chunk_idx)
        , m_current_data_idx(current_data_idx)
    {}

public:
    SubChunkView(iterator it)
        : SubChunkView{it, 0, 0}
    {}

public:

    SubChunkView& operator++() {
        m_current_data_idx++;
        m_it++;
        if (m_current_data_idx >= COUNT_DATA) {
            m_current_data_idx = 0;
            m_current_chunk_idx++;
        }
        return *this;
    }

    SubChunkView operator++(int) {
        SubChunkView tmp = *this;
        ++(*this);
        return tmp;
    }

    inline const std::size_t& getDataIdx() const {
        return m_current_data_idx;
    }
    inline std::size_t getTotalChunkIdx() const {
        return m_current_chunk_idx;
    }

    inline bool last() const {
        return 0 == (m_current_data_idx % COUNT_DATA);
    }

    inline static constexpr std::size_t getDataTotal() {
        return COUNT_DATA;
    }
    inline static constexpr std::size_t getDigits() {
        return DATATYPEDIGITS;
    }

    bool operator==(const SubChunkView& other) const noexcept {
        return (m_it == other.m_it);
    }

    bool operator!=(const SubChunkView& other) const noexcept {
        return !(*this == other);
    }

    decltype(auto) operator*() const {
        return *m_it;
    }

// Private members
private:
    iterator m_it;
    std::size_t m_current_chunk_idx = 0;
    std::size_t m_current_data_idx = 0;
};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_DBIHELPERS_H */
