#ifndef WIZ_UTILITY_BITWISE_OVERLOADS_H
#define WIZ_UTILITY_BITWISE_OVERLOADS_H

#include <cstdint>

#include <wiz/utility/macros.h>

#define WIZ_BITWISE_OVERLOADS(T) \
    WIZ_MAYBE_UNUSED_FORCE_INLINE(T operator ~(T value)) { \
        return static_cast<T>(~static_cast<std::uint32_t>(value)); \
    } \
    WIZ_MAYBE_UNUSED_FORCE_INLINE(T operator |(T left, T right)) { \
        return static_cast<T>(static_cast<std::uint32_t>(left) | static_cast<std::uint32_t>(right)); \
    } \
    WIZ_MAYBE_UNUSED_FORCE_INLINE(T operator &(T left, T right)) { \
        return static_cast<T>(static_cast<std::uint32_t>(left) & static_cast<std::uint32_t>(right)); \
    } \
    WIZ_MAYBE_UNUSED_FORCE_INLINE(T operator ^(T left, T right)) { \
        return static_cast<T>(static_cast<std::uint32_t>(left) ^ static_cast<std::uint32_t>(right)); \
    } \
    WIZ_MAYBE_UNUSED_FORCE_INLINE(T& operator |=(T& left, T right)) { return left = left | right; } \
    WIZ_MAYBE_UNUSED_FORCE_INLINE(T& operator &=(T& left, T right)) { return left = left & right; } \
    WIZ_MAYBE_UNUSED_FORCE_INLINE(T& operator ^=(T& left, T right)) { return left = left ^ right; }

#endif