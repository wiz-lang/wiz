#ifndef WIZ_UTILITY_BIT_FLAGS_H
#define WIZ_UTILITY_BIT_FLAGS_H

#include <cstdint>
#include <utility>
#include <type_traits>
#include <initializer_list>
#include <wiz/utility/macros.h>

namespace wiz {
    namespace detail {
        template <typename T, T Flag, T... Flags>
        struct BitFlagDisjunction {
            static constexpr std::uint32_t value = 
                (1 << static_cast<std::uint32_t>(Flag))
                | BitFlagDisjunction<T, Flags...>::value;
        };

        template <typename T, T Flag>
        struct BitFlagDisjunction<T, Flag> {
            static constexpr std::uint32_t value = static_cast<std::uint32_t>(Flag);
        };
    }

    template <typename T, T FlagCount>
    struct BitFlags {
        public:
            using UnderlyingType = std::underlying_type_t<T>;

            static_assert(0 <= static_cast<UnderlyingType>(FlagCount)
                && static_cast<UnderlyingType>(FlagCount) <= 32,
                "BitFlags type must use a FlagCount in the range 0 .. 32.");

            WIZ_FORCE_INLINE BitFlags()
            : flags(0) {}

            WIZ_FORCE_INLINE BitFlags(const BitFlags& other)
            : flags(other.flags) {}

            WIZ_FORCE_INLINE BitFlags(BitFlags&& other)
            : flags(other.flags) {}

            template <T... Flags>
            WIZ_FORCE_INLINE static BitFlags of() {
                return BitFlags(detail::BitFlagDisjunction<T, Flags...>::value);
            }

            WIZ_FORCE_INLINE BitFlags operator |(BitFlags other) const {
                return BitFlags(flags | other.flags);
            }

            WIZ_FORCE_INLINE  BitFlags operator &(BitFlags other) const {
                return BitFlags(flags & other.flags);
            }

            WIZ_FORCE_INLINE BitFlags operator ^(BitFlags other) const {
                return BitFlags(flags ^ other.flags);
            }

            WIZ_FORCE_INLINE BitFlags operator ~() const {
                return BitFlags(flags ^ ((1 << static_cast<std::uint32_t>(FlagCount)) - 1));
            }

            WIZ_FORCE_INLINE BitFlags& operator =(BitFlags other) {
                flags = other.flags;
                return *this;
            }

            WIZ_FORCE_INLINE BitFlags& operator |=(BitFlags other) {
                flags |= other.flags;
                return *this;
            }

            WIZ_FORCE_INLINE BitFlags& operator &=(BitFlags other) {
                flags &= other.flags;
                return *this;
            }

            WIZ_FORCE_INLINE BitFlags& operator ^=(BitFlags other) {
                flags ^= other.flags;
                return *this;
            }

            WIZ_FORCE_INLINE bool operator ==(BitFlags other) const {
                return flags == other.flags;
            }

            WIZ_FORCE_INLINE bool operator !=(BitFlags other) const {
                return !(*this == other);
            }

            template <T Flag>
            WIZ_FORCE_INLINE bool contains() const {
                return (flags & (1 << static_cast<std::uint32_t>(Flag))) != 0;
            }

            WIZ_FORCE_INLINE std::uint32_t underlying() const {
                return flags;
            }

            template <T... Flags>
            WIZ_FORCE_INLINE BitFlags add() const {
                return BitFlags(flags | detail::BitFlagDisjunction<T, Flags...>::value);
            }

            template <T... Flags>
            WIZ_FORCE_INLINE BitFlags intersect() const {
                return BitFlags(flags & detail::BitFlagDisjunction<T, Flags...>::value);
            }

            template <T... Flags>
            WIZ_FORCE_INLINE BitFlags toggle() const {
                return BitFlags(flags ^ detail::BitFlagDisjunction<T, Flags...>::value);
            }

            template <T... Flags>
            WIZ_FORCE_INLINE BitFlags remove() const {
                return BitFlags(flags & ~detail::BitFlagDisjunction<T, Flags...>::value);
            }

        private:
            WIZ_FORCE_INLINE explicit BitFlags(std::uint32_t flags)
            : flags(flags) {}

            std::uint32_t flags;
    };
}
#endif