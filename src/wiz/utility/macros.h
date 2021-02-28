#ifndef WIZ_UTILITY_MACROS_H
#define WIZ_UTILITY_MACROS_H

#ifdef _MSC_VER
#define WIZ_FORCE_INLINE __forceinline
#define WIZ_MAYBE_UNUSED_FORCE_INLINE(x) __forceinline x
#elif defined(__clang__) || defined(__GNUC__)
#define WIZ_FORCE_INLINE  inline
#define WIZ_MAYBE_UNUSED_FORCE_INLINE(x) inline static __attribute__((always_inline)) __attribute__((unused)) x
#else
#define WIZ_FORCE_INLINE inline
#define WIZ_MAYBE_UNUSED_FORCE_INLINE(x) inline x
#endif

#endif
