#ifndef WIZ_COMPILER_CONFIG_H
#define WIZ_COMPILER_CONFIG_H

#include <string>
#include <utility>
#include <unordered_map>

#include <wiz/utility/int128.h>
#include <wiz/utility/optional.h>
#include <wiz/utility/string_view.h>
#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/source_location.h>

namespace wiz {
    class Report;
    struct Expression;

    class Config {
        public:
            Config();
            ~Config();

            bool add(Report* report, StringView key, FwdUniquePtr<const Expression> value);
            const Expression* get(StringView key) const;
            bool has(StringView key) const;
            const Expression* checkValue(Report* report, StringView key, bool required) const;
            Optional<std::pair<const Expression*, bool>> checkBoolean(Report* report, StringView key, bool required) const;
            Optional<std::pair<const Expression*, Int128>> checkInteger(Report* report, StringView key, bool required) const;
            Optional<std::pair<const Expression*, StringView>> checkString(Report* report, StringView key, bool required) const;
            Optional<std::pair<const Expression*, StringView>> checkFixedString(Report* report, StringView key, std::size_t maxLength, bool required) const;

        private:
            std::unordered_map<StringView, FwdUniquePtr<const Expression>> items;
    };
}

#endif