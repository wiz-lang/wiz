#ifndef WIZ_UTILITY_SOURCE_LOCATION_H
#define WIZ_UTILITY_SOURCE_LOCATION_H

#include <string>
#include <cstddef>
#include <wiz/utility/string_view.h>

namespace wiz {
    class SourceLocation {
        public:
            SourceLocation();
            SourceLocation(StringView path);
            SourceLocation(StringView path, std::size_t line);
            SourceLocation(StringView displayPath, StringView canonicalPath, std::size_t line);

            std::string toString() const;

            std::size_t line;
            StringView displayPath;
            StringView canonicalPath;
    };
}

#endif
