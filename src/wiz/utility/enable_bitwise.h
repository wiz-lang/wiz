#ifndef WIZ_UTILITY_ENABLE_BITWISE_H
#define WIZ_UTILITY_ENABLE_BITWISE_H

#include <type_traits>
namespace wiz {
    template <typename T> struct EnableBitwise { static constexpr bool enabled = false; };

    template <typename T>
    std::enable_if_t<EnableBitwise<T>::enabled, T> operator |(T left, T right) {
        return static_cast<T>(static_cast<std::underlying_type_t<T>>(left) | static_cast<std::underlying_type_t<T>>(right));
    }

    template <typename T>
    std::enable_if_t<EnableBitwise<T>::enabled, T> operator &(T left, T right) {
        return static_cast<T>(static_cast<std::underlying_type_t<T>>(left) & static_cast<std::underlying_type_t<T>>(right));
    }

    template <typename T>
    std::enable_if_t<EnableBitwise<T>::enabled, T> operator ^(T left, T right) {
        return static_cast<T>(static_cast<std::underlying_type_t<T>>(left) ^ static_cast<std::underlying_type_t<T>>(right));
    }

    template <typename T>
    std::enable_if_t<EnableBitwise<T>::enabled, T> operator ~(T operand) {
        return static_cast<T>(~static_cast<std::underlying_type_t<T>>(operand));
    }

    template <typename T>
    std::enable_if_t<EnableBitwise<T>::enabled, T>& operator |=(T& left, T right) {
        left = static_cast<T>(static_cast<std::underlying_type_t<T>>(left) | static_cast<std::underlying_type_t<T>>(right));
        return left;
    }

    template <typename T>
    std::enable_if_t<EnableBitwise<T>::enabled, T>& operator &=(T& left, T right) {
        left = static_cast<T>(static_cast<std::underlying_type_t<T>>(left) & static_cast<std::underlying_type_t<T>>(right));
        return left;
    }

    template <typename T>
    std::enable_if_t<EnableBitwise<T>::enabled, T>& operator ^=(T& left, T right) {
        left = static_cast<T>(static_cast<std::underlying_type_t<T>>(left) ^ static_cast<std::underlying_type_t<T>>(right));
        return left;
    }
}


#endif
