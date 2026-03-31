#ifndef DRAMPOWER_UTIL_TYPE_TRAITS_H
#define DRAMPOWER_UTIL_TYPE_TRAITS_H

#include <cstddef>
#include <utility>

namespace DRAMPower::type_traits {

template <typename T>
struct enumerate_wrapper {
    T& iterable;

    struct iterator {
        std::size_t index;
        typename T::iterator iter;

        bool operator !=(const iterator& other) const { return iter != other.iter; }
        void operator++(){ ++index; ++iter; }
        auto operator*() const { return std::pair<std::size_t, typename T::reference>{index, *iter}; }
    };

    iterator begin() { return {0, iterable.begin()}; }
    iterator end() { return {0, iterable.end()}; }
};

template <typename T>
struct const_enumerate_wrapper {
    const T& iterable; // Captures by const reference
    
    struct iterator {
        std::size_t index;
        typename T::const_iterator iter; // Uses const_iterator

        bool operator!=(const iterator& other) const { return iter != other.iter; }
        void operator++() { ++index; ++iter; }
        
        // Returns a pair containing the index and a const reference to the value
        auto operator*() const { 
            return std::pair<std::size_t, typename T::const_reference>{index, *iter}; 
        }
    };

    iterator begin() const { return {0, iterable.begin()}; }
    iterator end() const { return {0, iterable.end()}; }
};

template <typename T>
enumerate_wrapper<T> enumerate(T& iterable) { return {iterable}; }
template <typename T>
const_enumerate_wrapper<T> enumerate(const T& iterable) { return {iterable}; }

} // namespace DRAMPower::type_traits

#endif /* DRAMPOWER_UTIL_TYPE_TRAITS_H */
