#ifndef WIZ_UTILITY_PATH_H
#define WIZ_UTILITY_PATH_H

#include <string>
#include <wiz/utility/string_view.h>

namespace wiz {
    namespace path {
        std::string getCurrentWorkingDirectory();
        std::string toNormalizedAbsolute(StringView path);
        std::string toNormalized(StringView path);
        std::string toRelative(StringView path, StringView origin);
        std::string getAbsolutePrefix(StringView path);
        std::size_t findCommonDirectoryPrefix(StringView path, StringView path2);
        StringView getDirectory(StringView path);
        StringView getFilename(StringView path);
        StringView getExtension(StringView path);    
        StringView stripExtension(StringView path);
    }
}

#endif
