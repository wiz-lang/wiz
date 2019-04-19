#ifndef WIZ_UTILITY_BIT_FLAGS_H
#define WIZ_UTILITY_BIT_FLAGS_H

#include <cstdint>
#include <type_traits>
#include <initializer_list>
#include <utility>

namespace wiz {
    template <typename T, T FlagCount>
    struct BitFlags {
        public:
            using UnderlyingType = std::underlying_type_t<T>;

            static_assert(0 <= static_cast<UnderlyingType>(FlagCount)
                && static_cast<UnderlyingType>(FlagCount) <= 32,
                "BitFlags type must use a FlagCount in the range 0 .. 32.");

            BitFlags()
            : flags(0) {}

            BitFlags(const BitFlags& other)
            : flags(other.flags) {}

            BitFlags(BitFlags&& other)
            : flags(std::move(other.flags)) {}

            explicit BitFlags(T flag)
            : flags(1 << static_cast<uint32_t>(flag)) {}

            explicit BitFlags(std::initializer_list<T> flags)
            : flags(0) {
                for (const auto flag : flags) {
                    this->flags |= (1 << static_cast<uint32_t>(flag));
                }
            }

            BitFlags operator |(BitFlags other) const {
                return BitFlags(flags | other.flags);
            }

            BitFlags operator &(BitFlags other) const {
                return BitFlags(flags & other.flags);
            }

            BitFlags operator ^(BitFlags other) const {
                return BitFlags(flags ^ other.flags);
            }

            BitFlags operator ~() const {
                return BitFlags(flags ^ ((1 << static_cast<uint32_t>(FlagCount)) - 1));
            }

            BitFlags& operator =(BitFlags other) {
                flags = other.flags;
                return *this;
            }

            BitFlags& operator |=(BitFlags other) {
                *this = *this | other;
                return *this;
            }

            BitFlags& operator &=(BitFlags other) {
                *this = *this & other;
                return *this;
            }

            BitFlags& operator ^=(BitFlags other) {
                *this = *this ^ other;
                return *this;
            }

            bool operator ==(BitFlags other) const {
                return flags == other.flags;
            }

            bool operator !=(BitFlags other) const {
                return !(*this == other);
            }

            template <T Flag>
            bool contains() const {
                static_assert(0 <= static_cast<UnderlyingType>(Flag)
                    && static_cast<UnderlyingType>(Flag) < static_cast<UnderlyingType>(FlagCount),
                    "contains() check must use a Flag in the range 0 .. FlagCount - 1.");
                return (flags & (1 << static_cast<uint32_t>(Flag))) != 0;
            }

            bool contains(T flag) const {
                return (flags & (1 << static_cast<uint32_t>(flag))) != 0;
            }

        private:
            explicit BitFlags(std::uint32_t flags)
            : flags(flags) {}

            std::uint32_t flags;
    };
}

#endif

