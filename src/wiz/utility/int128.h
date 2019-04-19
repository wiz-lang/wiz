#ifndef WIZ_UTILITY_INT128_H
#define WIZ_UTILITY_INT128_H

#include <cassert>
#include <climits>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <utility>
#include <string>

#ifdef WIZ_UTILITY_INT128_USE_IOSTREAM
#include <iostream>
#endif

namespace wiz {
    struct Int128 {
        static_assert(sizeof(int) <= sizeof(std::uint64_t));
        static_assert(sizeof(long) <= sizeof(std::uint64_t));
        static_assert(sizeof(long long) <= sizeof(std::uint64_t));

        Int128()
        : low(0), high(0) {}

        Int128(const Int128& other) = default;
        Int128(Int128&& other) = default;

        explicit Int128(signed char value)
        : low(value < 0
            ? (((static_cast<std::uint64_t>(-value) ^ UCHAR_MAX) + 1) | (UINT64_MAX &~ static_cast<std::uint64_t>(UCHAR_MAX)))
            : static_cast<std::uint64_t>(value)),
        high(value < 0 ? UINT64_MAX : 0) {}

        explicit Int128(short value)
        : low(value < 0
            ? (((static_cast<std::uint64_t>(-value) ^ USHRT_MAX) + 1) | (UINT64_MAX &~ static_cast<std::uint64_t>(USHRT_MAX)))
            : static_cast<std::uint64_t>(value)),
        high(value < 0 ? UINT64_MAX : 0) {}

        explicit Int128(int value)
        : low(value < 0
            ? (((static_cast<std::uint64_t>(-value) ^ UINT_MAX) + 1) | (UINT64_MAX &~ static_cast<std::uint64_t>(UINT_MAX)))
            : static_cast<std::uint64_t>(value)),
        high(value < 0 ? UINT64_MAX : 0) {}

        explicit Int128(long value)
        : low(value < 0
            ? (((static_cast<std::uint64_t>(-value) ^ ULONG_MAX) + 1) | (UINT64_MAX &~ static_cast<std::uint64_t>(ULONG_MAX)))
            : static_cast<std::uint64_t>(value)),
        high(value < 0 ? UINT64_MAX : 0) {}

        explicit Int128(long long value)
        : low(value < 0
            ? (((static_cast<std::uint64_t>(-value) ^ ULLONG_MAX) + 1) | (UINT64_MAX &~ static_cast<std::uint64_t>(ULLONG_MAX)))
            : static_cast<std::uint64_t>(value)),
        high(value < 0 ? UINT64_MAX : 0) {}        

        explicit Int128(unsigned char value)
        : low(value), high(0) {}

        explicit Int128(unsigned short value)
        : low(value), high(0) {}

        explicit Int128(unsigned int value)
        : low(value), high(0) {}

        explicit Int128(unsigned long value)
        : low(value), high(0) {}

        explicit Int128(unsigned long long value)
        : low(value), high(0) {}

        Int128(std::uint64_t low, std::uint64_t high)
        : low(low), high(high) {}

        static Int128 zero() {
            return Int128(0, 0);
        }

        static Int128 one() {
            return Int128(1, 0);
        }

        static Int128 minValue() {
            return Int128(0, UINT64_C(0x8000000000000000));
        }

        static Int128 maxValue() {
            return Int128(UINT64_MAX, UINT64_C(0x7FFFFFFFFFFFFFFF));
        }

        enum class ParseResult {
            Success,
            InvalidArgument,
            FormatError,
            RangeError,
        };

        static std::pair<ParseResult, Int128> parse(const char* str, std::size_t base = 10) {
            std::size_t length = std::strlen(str);
            return parse(str, str + length, base);
        }

        template <typename InputIterator>
        static std::pair<ParseResult, Int128> parse(InputIterator first, InputIterator last, std::size_t base = 10, bool negative = false) {
            if (base < 2 || base > 36 || first == last) {
                return {ParseResult::InvalidArgument, zero()};
            }

            if (!negative) {
                if (first != last) {
                    if (*first == '-') {
                        negative = true;
                        ++first;
                    } else if (*first == '+') {
                        ++first;
                    }
                }
            }

            std::pair<CheckedArithmeticResult, Int128> result = {CheckedArithmeticResult::Success, zero()};

            if (first == last) {
                return {ParseResult::FormatError, zero()};
            }

            while (first != last) {
                result = result.second.checkedMultiply(Int128(base, 0));
                if (result.first == CheckedArithmeticResult::OverflowError) {
                    return {ParseResult::RangeError, zero()};
                }

                const auto c = static_cast<uint8_t>(*first);

                Int128 digit;
                if (c >= '0' && c <= '0' + base) {
                    digit = Int128(c - '0');
                } else if (base > 10 && c >= 'a' && c <= ('a' + base - 10)) {
                    digit = Int128((c - 'a') + 10);
                } else if (base > 10 && c >= 'A' && c <= ('A' + base - 10)) {
                    digit = Int128((c - 'A') + 10);
                } else {
                    return {ParseResult::FormatError, zero()};
                }

                result = result.second.checkedAdd(negative ? -digit : digit);
                if (result.first == CheckedArithmeticResult::OverflowError) {
                    return {ParseResult::RangeError, zero()};
                }

                ++first;
            }

            return {ParseResult::Success, result.second};
        }

        bool isZero() const {
            return low == 0 && high == 0;
        }

        bool isPositive() const {
            return !isZero() && !isNegative();
        }

        bool isNegative() const {
            return (high & 0x8000000000000000) != 0;
        }

        Int128 getAbsoluteValue() const {
            return isNegative() ? -*this : *this;
        }
            
        bool getBit(std::size_t bit) const {
            if (bit >= 128) {
                return 0;
            } else if (bit >= 64) {
                return (high & (static_cast<uint64_t>(1) << static_cast<uint64_t>(bit - 64))) != 0;
            } else {
                return (low & (static_cast<uint64_t>(1) << static_cast<uint64_t>(bit))) != 0;
            }
        }

        void setBit(std::size_t bit, bool value) {
            if (bit >= 128) {
                return;
            } else if (bit >= 64) {
                std::uint64_t mask = static_cast<uint64_t>(1) << static_cast<uint64_t>(bit - 64);
                if (value) {
                    high |= mask;
                } else {
                    high &= ~mask;
                }
            } else {
                std::uint64_t mask = static_cast<uint64_t>(1) << static_cast<uint64_t>(bit);
                if (value) {
                    low |= mask;
                } else {
                    low &= ~mask;
                }
            }
        }

        Int128 logicalLeftShiftOnce() const {
            return Int128(low << 1, (high << 1) | (low >> 63));
        }

        Int128 logicalRightShiftOnce() const {
            return Int128((low >> 1) | (high << 63), high >> 1);
        }

        Int128 arithmeticRightShiftOnce() const {
            return Int128((low >> 1) | (high << 63), (high >> 1) | (high & 0x8000000000000000));
        }

        Int128 logicalLeftShift(std::size_t bits) const {
            return *this << bits;
        }

        Int128 logicalRightShift(std::size_t bits) const {
            if (bits == 0) {
                return *this;
            } else if (bits >= 128) {
                return zero();
            } else if (bits >= 64) {
                return Int128(high >> (bits - 64), 0);
            } else {
                return Int128((low >> bits) | (high << (64 - bits)), high >> bits);
            }
        }

        Int128 arithmeticRightShift(std::size_t bits) const {
            if (bits == 0) {
                return *this;
            } else if (bits >= 128) {
                return isNegative() ? Int128(-1) : zero();
            } else if (bits >= 64) {
                return Int128((high >> (bits - 64)) | (isNegative() ? (UINT64_MAX << (64 - (bits - 64))) : 0), UINT64_MAX);
            } else {
                return Int128((low >> bits) | (high << (64 - bits)), (high >> bits) | (isNegative() ? (UINT64_MAX << (64 - bits)) : 0));
            }
        }

        std::pair<Int128, Int128> unsignedDivisionWithRemainder(Int128 other) const {
            if (other.isZero()) {
                assert(!other.isZero());
                std::abort();
                return {zero(), zero()};
            } else if (other == one()) {
                return {*this, zero()};
            } else if (*this == other) {
                return {one(), zero()};
            } else if (isZero() || (*this != minValue() && *this < other)) {
                return {zero(), *this};
            } else if (high == 0 && other.high == 0) {
                return {Int128(low / other.low, 0), Int128(low % other.low, 0)};
            } else {
                auto quotient = zero();
                auto remainder = zero();
                for (std::size_t i = findMostSignificantBit(); i <= 128; --i) {
                    remainder = remainder.logicalLeftShiftOnce();
                    remainder.setBit(0, getBit(i));
                    if (remainder >= other) {
                        remainder -= other;
                        quotient.setBit(i, true);
                    }
                }
                 return {quotient, remainder};
            }
        }

        std::pair<Int128, Int128> divisionWithRemainder(Int128 other) const {
            if (isNegative()) {
                const auto negativeThis = -*this;
                if (other.isNegative()) {
                    const auto result = negativeThis.unsignedDivisionWithRemainder(-other);
                    return {result.first, -result.second};
                } else {
                    const auto result = negativeThis.unsignedDivisionWithRemainder(other);                        
                    return {-result.first, -result.second};
                }
            } else {
                if (other.isNegative()) {
                    const auto result = unsignedDivisionWithRemainder(-other);
                    return {-result.first, result.second};
                } else {
                    return unsignedDivisionWithRemainder(other);
                }
            }
        }

        std::size_t findLeastSignificantBit() const {
            std::size_t index = 0;
            auto value = *this;
            while (!value.getBit(0)) {
                ++index;
                value = value.logicalRightShiftOnce();
            }
            return index;
        }

        std::size_t findMostSignificantBit() const {
            std::size_t index = 0;
            auto value = *this;
            while (!value.isZero()) {
                ++index;
                value = value.logicalRightShiftOnce();
            }
            return index;
        }

        bool isPowerOfTwo() const {
            return !isZero() && (*this & (*this - Int128(1))).isZero();
        }

        std::string toString(std::size_t base = 10) const {
            if (base < 2 || base >= 36) {
                return "";
            }

            const auto negative = isNegative();
            if (base == 10 && (high == 0 || (low < 0x8000000000000000 && high == UINT64_MAX))) {
                if (negative) {
                    return std::to_string(-static_cast<int64_t>((low - 1) ^ UINT64_MAX));
                } else {
                    return std::to_string(low);
                }
            } else {
                char buffer[129] = {0};
                std::size_t bufferIndex = 128;
                std::pair<Int128, Int128> quotientAndRemainder(getAbsoluteValue(), zero());

                do {
                    quotientAndRemainder = quotientAndRemainder.first.unsignedDivisionWithRemainder(Int128(base));
                    if (quotientAndRemainder.second.low < 10) {
                        buffer[--bufferIndex] = static_cast<char>(quotientAndRemainder.second.low + '0');
                    } else if (quotientAndRemainder.second.low < 36) {
                        buffer[--bufferIndex] = static_cast<char>(quotientAndRemainder.second.low - 10 + 'A');
                    }
                } while (!quotientAndRemainder.first.isZero());

                if (negative) {
                    buffer[--bufferIndex] = '-';
                }

                return std::string(&buffer[bufferIndex]);
            }
        }

        enum class CheckedArithmeticResult {
            Success,
            OverflowError,
            DivideByZeroError
        };

        std::pair<CheckedArithmeticResult, Int128> checkedAdd(Int128 other) const {
            if (isNegative()) {
                if (other.isNegative() && *this < minValue() - other) {
                    return {CheckedArithmeticResult::OverflowError, zero()};
                }
            } else {
                if (!other.isNegative() && *this > maxValue() - other) {
                    return {CheckedArithmeticResult::OverflowError, zero()};
                }
            }
            return {CheckedArithmeticResult::Success, *this + other};
        }

        std::pair<CheckedArithmeticResult, Int128> checkedSubtract(Int128 other) const {
            if (isNegative()) {
                if (!other.isNegative() && *this < minValue() + other) {
                    return {CheckedArithmeticResult::OverflowError, zero()};
                }
            } else {
                if (other.isNegative() && *this > maxValue() + other) {
                    return {CheckedArithmeticResult::OverflowError, zero()};
                }
            }
            return {CheckedArithmeticResult::Success, *this - other};
        }

        std::pair<CheckedArithmeticResult, Int128> checkedMultiply(Int128 other) const {
            Int128 result;
            if (isZero() || other.isZero()) {
                return {CheckedArithmeticResult::Success, Int128()};
            }

            if (isNegative()) {
                if (other.isNegative()) {
                    if (other < maxValue() / *this) {
                        return {CheckedArithmeticResult::OverflowError, Int128()};
                    }
                } else {
                    const auto limit = minValue() / other;
                    if (*this < limit) {
                        return {CheckedArithmeticResult::OverflowError, Int128()};
                    }
                }
            } else {
                if (other.isNegative()) {
                    if (other < minValue() / *this) {
                        return {CheckedArithmeticResult::OverflowError, Int128()};
                    }
                } else {
                    if (*this > maxValue() / other) {
                        return {CheckedArithmeticResult::OverflowError, Int128()};
                    }
                }
            }

            return {CheckedArithmeticResult::Success, *this * other};
        }


        std::pair<CheckedArithmeticResult, Int128> checkedDivide(Int128 other) const {
            if (other.isZero()) {
                return {CheckedArithmeticResult::DivideByZeroError, Int128()};
            } else if (*this == minValue() && other == Int128(-1)) {
                return {CheckedArithmeticResult::OverflowError, Int128()};
            }

            return {CheckedArithmeticResult::Success, *this / other};
        }

        std::pair<CheckedArithmeticResult, Int128> checkedModulo(Int128 other) const {
            if (other.isZero()) {
                return {CheckedArithmeticResult::DivideByZeroError, Int128()};
            } else if (*this == minValue() && other == Int128(-1)) {
                return {CheckedArithmeticResult::OverflowError, Int128()};
            }

            return {CheckedArithmeticResult::Success, *this % other};
        }

        std::pair<CheckedArithmeticResult, Int128> checkedLogicalLeftShift(std::size_t bits) const {
            if (!isZero()) {
                if (bits >= 128 || (bits > 0 && !logicalRightShift(127 - bits).isZero())) {
                    return {CheckedArithmeticResult::OverflowError, Int128()};
                }
            }
            return {CheckedArithmeticResult::Success, *this << bits};
        }

        explicit operator signed char() const {
            return isNegative() ? -static_cast<signed char>((low - 1) ^ UINT64_MAX) : static_cast<signed char>(low);
        }

        explicit operator short() const {
            return isNegative() ? -static_cast<short>((low - 1) ^ UINT64_MAX) : static_cast<short>(low);
        }

        explicit operator int() const {
            return isNegative() ? -static_cast<int>((low - 1) ^ UINT64_MAX) : static_cast<int>(low);
        }

        explicit operator long() const {
            return isNegative() ? -static_cast<long>((low - 1) ^ UINT64_MAX) : static_cast<long>(low);
        }

        explicit operator long long() const {
            return isNegative() ? -static_cast<long long>((low - 1) ^ UINT64_MAX) : static_cast<long long>(low);
        }

        explicit operator unsigned char() const {
            return static_cast<unsigned char>(low);
        }

        explicit operator unsigned short() const {
            return static_cast<unsigned short>(low);
        }

        explicit operator unsigned int() const {
            return static_cast<unsigned int>(low);
        }

        explicit operator unsigned long() const {
            return static_cast<unsigned long>(low);
        }

        explicit operator unsigned long long() const {
            return static_cast<unsigned long long>(low);
        }

        Int128& operator =(const Int128& other) {
            low = other.low;
            high = other.high;
            return *this;
        }

        Int128& operator =(Int128&& other) {
            low = other.low;
            high = other.high;
            return *this;
        }

        bool operator ==(Int128 other) const {
            return low == other.low && high == other.high;
        }

        bool operator !=(Int128 other) const {
            return !(*this == other);
        }

        bool operator <(Int128 other) const {
            if (isNegative()) {
                if (other.isNegative()) {
                    return high < other.high
                        || (high == other.high && low < other.low);
                } else {
                    return true;
                }
            } else {
                if (other.isNegative()) {
                    return false;
                } else {
                    return high < other.high
                        || (high == other.high && low < other.low);
                }
            }
        }

        bool operator <=(Int128 other) const {
            return !(other < *this);
        }

        bool operator >(Int128 other) const {
            return other < *this;
        }

        bool operator >=(Int128 other) const {
            return !(*this < other);
        }

        Int128 operator ~() const {
            return Int128(~low, ~high);
        }

        Int128 operator +() const {
            return *this;
        }

        Int128 operator -() const {
            Int128 result = ~*this;
            ++result;
            return result;
        }

        Int128 operator &(Int128 other) const {
            return Int128(low & other.low, high & other.high);
        }

        Int128 operator |(Int128 other) const {
            return Int128(low | other.low, high | other.high);
        }

        Int128 operator ^(Int128 other) const {
            return Int128(low ^ other.low, high ^ other.high);
        }

        Int128 operator <<(std::size_t bits) const {
            if (bits == 0) {
                return *this;
            } else if (bits >= 128) {
                return zero();
            } else if (bits >= 64) {
                return Int128(0, low << (bits - 64));
            } else {
                return Int128(low << bits, (high << bits) | (low >> (64 - bits)));
            }
        }

        Int128& operator --() {
            if (low == 0) {
                --high;
            }
            --low;
            return *this;
        }

        Int128& operator ++() {
            ++low;
            if (low == 0) {
                ++high;
            }
            return *this;
        }

        Int128 operator --(int) {
            Int128 result = *this;
            --*this;
            return result;
        }

        Int128 operator ++(int) {
            Int128 result = *this;
            ++*this;
            return result;
        }

        Int128 operator +(Int128 other) const {
            const auto carry = other.low > UINT64_MAX - low;
            return Int128(low + other.low, high + other.high + (carry ? 1 : 0));
        }

        Int128 operator -(Int128 other) const {
            return *this + -other;
        }

        Int128 operator *(Int128 other) const {
            if (other == one()) {
                return *this;
            } else if (isZero() || other.isZero()) {
                return Int128();
            } else if (((high == 0 || high == UINT64_MAX) && low <= UINT32_MAX) && ((other.high == 0 || other.high == UINT64_MAX) && other.low <= UINT32_MAX)) {
                return Int128(low * other.low, (high == UINT64_MAX) != (other.high == UINT64_MAX) ? UINT64_MAX : 0);
            } else {
                // First do a 64 x 64 -> 128-bit multiply.
                //
                // a * b
                // = (2^32 * ah + al) * (2^32 * bh + bl) [rewriting 64-bit values in their split 32-bit form]
                // = 2^64 * ah * bh + 2^32 * ah * bl + 2^32 * al * bh + al * bl [expanding product]
                // = w + z + y + x [giving names to each sub-product]
                //
                // x: 32 x 32 -> 64-bit product, bits 0..63
                // y and z: 32 x 32 -> 64-bit product, shifted by 32 bits, bits 32..95
                // w: 32 x 32 -> 64-bit product, bits 64..128
                // addition happens across lo/hi word boundaries and can generate middle carries, so we need to add each piece separately as 128-bit integers.
                //
                // Then to make a 128 x 128 -> 128-bit multiply, we do similarly, but since we're asking for 128-bit instead of 256-bit result,
                // we discard the upper product, and just keep the lower 128 bits of the result.
                //
                // 2^128 * (ah64 * bh64) + 2^64 * (ah64 * bl64 + al64 * bh64) + (al64 * bl64)
                // = 2^64 * (ah64 * bl64 + al64 * bh64) + (al64 * bl64) modulo 2^128 [note: al64 * bl64 was figured out by 64x64 -> 64 multiply]
                const auto al = low & 0xFFFFFFFF;
                const auto ah = (low >> 32) & 0xFFFFFFFF;
                const auto bl = other.low & 0xFFFFFFFF;
                const auto bh = (other.low >> 32) & 0xFFFFFFFF;
                const auto x = al * bl;
                const auto y = al * bh;
                const auto z = ah * bl;
                const auto w = ah * bh;
                return Int128(x, 0) + Int128(y << 32, y >> 32) + Int128(z << 32, z >> 32) + Int128(0, w + low * other.high + high * other.low);
            }
        }

        Int128 operator /(Int128 other) const {
            return divisionWithRemainder(other).first;
        }

        Int128 operator %(Int128 other) const {
            return divisionWithRemainder(other).second;
        }

        Int128& operator +=(Int128 other) {
            *this = *this + other;
            return *this;
        }

        Int128& operator -=(Int128 other) {
            *this = *this - other;
            return *this;
        }

        Int128& operator *=(Int128 other) {
            *this = *this * other;
            return *this;
        }

        Int128& operator /=(Int128 other) {
            *this = *this / other;
            return *this;
        }

        Int128& operator %=(Int128 other) {
            *this = *this % other;
            return *this;
        }

        Int128& operator &=(Int128 other) {
            *this = *this & other;
            return *this;
        }

        Int128& operator |=(Int128 other) {
            *this = *this | other;
            return *this;
        }

        Int128& operator ^=(Int128 other) {
            *this = *this ^ other;
            return *this;
        }

        Int128& operator <<=(std::size_t bits) {
            *this = *this << bits;
            return *this;
        }

#ifdef WIZ_UTILITY_INT128_USE_IOSTREAM
        friend std::ostream& operator<<(std::ostream& out, const Int128& value);
#endif

        std::uint64_t low;
        std::uint64_t high;
    };

#ifdef WIZ_UTILITY_INT128_USE_IOSTREAM
    inline std::ostream& operator <<(std::ostream& out, const Int128& value) {
        const auto negative = value.isNegative();
        if (value.high == 0 || (value.low < 0x8000000000000000 && value.high == UINT64_MAX)) {
            if (negative) {
                out << -static_cast<int64_t>((value.low - 1) ^ UINT64_MAX);
            } else {
                out << value.low;
            }
        } else {
            std::size_t base = 10;
            if ((out.flags() & out.dec) != 0) {
                base = 10;
            } else if ((out.flags() & out.hex) != 0) {
                base = 16;
            } else if ((out.flags() & out.oct) != 0) {
                base = 8;
            }

            char buffer[129] = {0};
            std::size_t bufferIndex = 128;
            std::pair<Int128, Int128> quotientAndRemainder(value.getAbsoluteValue(), Int128::zero());

            do {
                quotientAndRemainder = quotientAndRemainder.first.unsignedDivisionWithRemainder(Int128(base));
                if (quotientAndRemainder.second.low < 10) {
                    buffer[--bufferIndex] = static_cast<char>(quotientAndRemainder.second.low + '0');
                } else if (quotientAndRemainder.second.low < 36) {
                    buffer[--bufferIndex] = static_cast<char>(quotientAndRemainder.second.low - 10 + 'A');
                }
            } while (!quotientAndRemainder.first.isZero());

            if (negative) {
                buffer[--bufferIndex] = '-';
            }

            out << &buffer[bufferIndex];
        }

        return out;
    }
#endif

}

#endif