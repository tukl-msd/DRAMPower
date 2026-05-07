#ifndef DRAMPOWER_UTIL_ADL_SERIALIZER_H
#define DRAMPOWER_UTIL_ADL_SERIALIZER_H

#include "DRAMUtils/util/types.h"

#include <cstdint>
#include <ostream>
#include <istream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <stdint.h>


namespace DRAMPower {

namespace type_utils {

template<typename T, typename SFINAE = void>
struct ADLSerializer {
    static_assert(DRAMUtils::util::always_false<T>::value, "No serialize function for T defined");
};

template<typename T, typename SFINAE = void>
struct ADLDeserializer {
    static_assert(DRAMUtils::util::always_false<T>::value, "No deserialize function for T defined");
};

namespace detail {

namespace adl_probe {
    // Fallbacks
    template<typename T> void serialize(...) = delete;
    template<typename T> void deserialize(...) = delete;
}

template<typename T, typename = std::void_t<>>
struct has_serialize : std::false_type {};

template<typename T>
struct has_serialize<T, std::void_t<
    // This checks if 'serialize(stream, val)' is a valid expression
    decltype(serialize(std::declval<std::ostream&>(), std::declval<const T&>()))
>> : std::true_type {};

template<typename T, typename = std::void_t<>>
struct has_deserialize : std::false_type {};

template<typename T>
struct has_deserialize<T, std::void_t<
    // This checks if 'deserialize(stream, val)' is a valid expression
    decltype(deserialize(std::declval<std::istream&>(), std::declval<T&>()))
>> : std::true_type {};

template<typename T>
inline constexpr bool has_serialize_v = has_serialize<T>::value;

template<typename T>
inline constexpr bool has_deserialize_v = has_deserialize<T>::value;

} // namespace detail



// Hierarchy
template <typename T>
struct ADLSerializer<T, std::enable_if_t <detail::has_serialize_v<T>>> {
    static void serialize(std::ostream& stream, const T& val) {
        serialize(stream, val);
    }
};
template <typename T>
struct ADLDeserializer<T, std::enable_if_t <detail::has_deserialize_v<T>>> {
    static void deserialize(std::istream& stream, T& val) {
        deserialize(stream, val);
    }
};

// Integral types
template<typename T>
struct ADLSerializer<T, std::enable_if_t<std::is_integral_v<T>, void>> {
    static void serialize(std::ostream& stream, const T& val) {
        stream.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }
};
template<typename T>
struct ADLDeserializer<T, std::enable_if_t<std::is_integral_v<T>, void>> {
    static void deserialize(std::istream& stream, T& val) {
        stream.read(reinterpret_cast<char*>(&val), sizeof(val));
    }
};


// Tuple
template<typename... Entries>
struct ADLSerializer<std::tuple<Entries...>> {
    static void serialize(std::ostream& stream, const std::tuple<Entries...>& val) {
        std::apply([&stream](const auto&... args) {
            (ADLSerializer<std::decay_t<decltype(args)>>::serialize(stream, args), ...);
        }, val);
    }
};
template<typename... Entries>
struct ADLDeserializer<std::tuple<Entries...>> {
    static void deserialize(std::istream& stream, std::tuple<Entries...>& val) {
        std::apply([&stream](auto&... args) {
            (ADLDeserializer<std::decay_t<decltype(args)>>::deserialize(stream, args), ...);
        }, val);
    }
};

// Variant
template<typename... Ts>
struct ADLSerializer<std::variant<Ts...>> {
    static void serialize(std::ostream& stream, const std::variant<Ts...>& v) {
        uint32_t index = static_cast<uint32_t>(v.index());
        stream.write(reinterpret_cast<const char*>(&index), sizeof(index));
        std::visit([&stream](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            ADLSerializer<T>::serialize(stream, val);
        }, v);
    }
};
template<typename... Ts>
struct ADLDeserializer<std::variant<Ts...>> {
    static void deserialize(std::istream& stream, std::variant<Ts...>& v) {
        uint32_t index = 0;
        stream.read(reinterpret_cast<char*>(&index), sizeof(index));
        v = reconstruct(stream, index, std::make_index_sequence<sizeof...(Ts)>{});
    }

private:
    template<std::size_t... Is>
    static std::variant<Ts...> reconstruct(std::istream& s, uint32_t idx, std::index_sequence<Is...>) {
        using Reconstructor = std::variant<Ts...>(*)(std::istream&);
        static constexpr Reconstructor lut[] = {
            [](std::istream& s) -> std::variant<Ts...> {
                using T = std::variant_alternative_t<Is, std::variant<Ts...>>;
                T obj{}; // default constructed
                ADLDeserializer<T>::deserialize(s, obj);
                return obj;
            }...
        };
        return lut[idx](s);
    }
};

} // namespace type_utils

} // namespace DRAMPower

#endif /* DRAMPOWER_UTIL_ADL_SERIALIZER_H */
