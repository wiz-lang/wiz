#include <wiz/utility/misc.h>

namespace wiz {
    std::size_t log2(std::size_t value) {
        std::size_t result = 0;
        while (value >>= 1) {
            ++result;
        }
        return result;
    }
}