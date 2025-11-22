#pragma once

#include <type_traits>

#define ENABLE_BITMASK(Enum_Type)                                              \
    template <> struct enable_bitmask<Enum_Type> : std::true_type {};

// Primary template: disabled by default
template <typename E> struct enable_bitmask : std::false_type {};

// Helper alias: underlying integer type of enum
template <typename E> using enum_u = std::underlying_type_t<E>;

template <typename E>
constexpr std::enable_if_t<enable_bitmask<E>::value, E> operator|(E a, E b) {
    return static_cast<E>(
        static_cast<enum_u<E>>(a) | static_cast<enum_u<E>>(b));
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask<E>::value, E> operator&(E a, E b) {
    return static_cast<E>(
        static_cast<enum_u<E>>(a) & static_cast<enum_u<E>>(b));
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask<E>::value, E> operator^(E a, E b) {
    return static_cast<E>(
        static_cast<enum_u<E>>(a) ^ static_cast<enum_u<E>>(b));
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask<E>::value, E> operator~(E a) {
    return static_cast<E>(~static_cast<enum_u<E>>(a));
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask<E>::value, E&> operator|=(E& a, E b) {
    a = a | b;
    return a;
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask<E>::value, E&> operator&=(E& a, E b) {
    a = a & b;
    return a;
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask<E>::value, E&> operator^=(E& a, E b) {
    a = a ^ b;
    return a;
}
