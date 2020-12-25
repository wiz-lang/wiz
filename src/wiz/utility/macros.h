#ifndef WIZ_UTILITY_MACROS_H
#define WIZ_UTILITY_MACROS_H

#ifdef _MSC_VER
#define WIZ_FORCE_INLINE __forceinline
#elif defined(__clang__) || defined(__GNUC__)
#define WIZ_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define WIZ_FORCE_INLINE inline
#endif

#endif
