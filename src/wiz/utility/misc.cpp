#include <wiz/utility/misc.h>

#include <limits>

namespace wiz {
    std::size_t log2(std::size_t value) {
        std::size_t result = 0;
        while (value >>= 1) {
            ++result;
        }
        return result;
    }

    std::string toHexString(std::size_t value) {
        char buffer[std::numeric_limits<std::size_t>::digits + 1] = {0};
        std::sprintf(buffer, "%X", static_cast<unsigned int>(value));
        return std::string(buffer);
    }
}