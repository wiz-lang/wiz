#define WIZ_TTY_INCLUDE_LOWLEVEL
#include <wiz/utility/tty.h>

namespace wiz {
    bool canReceiveEOF(std::FILE* file) {
#ifdef _WIN32
        if (const auto handle = getStdHandleForFile(file)) {
            if (isMinTTY(handle)) {
                return false;
            }
        }
#endif
        static_cast<void>(file);
        return true;
    }

    bool isTTY(std::FILE* file) {
#ifdef WIZ_ISATTY
        if (WIZ_ISATTY(WIZ_FILENO(file))) {
            return true;
        }
#endif
        static_cast<void>(file);
        return false;
    }

    bool isAnsiTTY(std::FILE* file) {
#ifdef _WIN32
        if (const auto handle = getStdHandleForFile(file)) {
            return isMinTTY(handle);
        } else {
            return false;
        }
#else
        return isTTY(file);
#endif
    }
}