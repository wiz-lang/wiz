#ifndef WIZ_UTILITY_STRING_POOL_H
#define WIZ_UTILITY_STRING_POOL_H

#include <cstddef>
#include <string>
#include <unordered_set>
#include <wiz/utility/string_view.h>

namespace wiz {
    class StringPool {
        public:
            template <typename T>
            StringView intern(T&& source) {
                const auto result = items.emplace(std::forward<T>(source)).first;
                return StringView(result->data(), result->length());
            }

        private:
            std::unordered_set<std::string> items;
    };
}

#endif
