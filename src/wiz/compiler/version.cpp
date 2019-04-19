#include <string>

#include <wiz/compiler/version.h>

namespace wiz {
    namespace version {
        extern const char* const Text = "0.1.0 (alpha)";

        // Numeric version format:
        // major (4 digits)
        // minor (2 digits)
        // revision (2 digits).
        const std::uint32_t ID = UINT64_C(100);
    }
}
