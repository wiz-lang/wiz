#ifndef WIZ_UTILITY_TTY_H
#define WIZ_UTILITY_TTY_H

#include <cstdio>

#ifdef WIZ_TTY_INCLUDE_LOWLEVEL
#ifdef _WIN32
    #include <wiz/utility/win32.h>
    #include <io.h>
    #define WIZ_ISATTY _isatty
    #define WIZ_FILENO _fileno
#elif (defined(__APPLE__) || defined(__unix__)) && !defined(__EMSCRIPTEN__) && defined(_POSIX_SOURCE)
    #include <unistd.h>
    #include <sys/ioctl.h>

    #define WIZ_ISATTY isatty
    #define WIZ_FILENO fileno
#endif
#endif

namespace wiz {
    bool canReceiveEOF(std::FILE* file);
    bool isTTY(FILE* file);
    bool isAnsiTTY(FILE* file);
}

#endif