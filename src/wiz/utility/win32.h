#ifndef WIZ_UTILITY_WIN32_H
#define WIZ_UTILITY_WIN32_H

#ifdef _WIN32

#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace wiz {
    bool isMinTTY(HANDLE handle);
    HANDLE getStdHandleForFile(std::FILE* file);
}

#endif

#endif