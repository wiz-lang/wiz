#ifndef WIZ_UTILITY_STRING_POOL_H
#define WIZ_UTILITY_STRING_POOL_H

#include <cstddef>
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>

#include <wiz/utility/macros.h>
#include <wiz/utility/string_view.h>

namespace wiz {
    class StringPool {
        public:
            WIZ_FORCE_INLINE StringView intern(const char* source) {
                return intern(StringView(source));
            }

            WIZ_FORCE_INLINE StringView intern(const std::string& source) {
                return intern(StringView(source));
            }

            StringView intern(StringView source) {
                const auto match = views.find(source);

                if (match != views.end()) {
                    return *match;
                } else {
                    strings.push_back(std::make_unique<std::string>(source.getData(), source.getLength()));

                    const auto view = StringView(*strings.back());
                    views.insert(view);
                    return view;
                }
            }

        private:
            std::vector<std::unique_ptr<std::string>> strings;
            std::unordered_set<StringView> views;
    };
}

#endif
