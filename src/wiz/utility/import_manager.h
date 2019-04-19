#ifndef WIZ_UTILITY_IMPORT_MANAGER_H
#define WIZ_UTILITY_IMPORT_MANAGER_H

#include <memory>
#include <unordered_set>

#include <wiz/utility/array_view.h>
#include <wiz/utility/string_pool.h>
#include <wiz/utility/string_view.h>
#include <wiz/utility/import_options.h>

namespace wiz {
    class Reader;
    class Report;
    class ResourceManager;

    enum class ImportResult {
        Failed,
        JustImported,
        AlreadyImported,
    };

    class ImportManager {
        public:
            ImportManager(StringPool* stringPool, ResourceManager* resourceManager, ArrayView<StringView> importDirs);

            StringView getStartPath() const;
            void setStartPath(StringView value);

            StringView getCurrentPath() const;
            void setCurrentPath(StringView value);

            ImportResult attemptAbsoluteImport(StringView originalPath, StringView attemptedPath, ImportOptions importOptions, StringView& displayPath, StringView& canonicalPath, std::unique_ptr<Reader>& reader);
            ImportResult attemptRelativeImport(StringView originalPath, ImportOptions importOptions, StringView& displayPath, StringView& canonicalPath, std::unique_ptr<Reader>& reader);
            ImportResult importModule(StringView originalPath, ImportOptions importOptions, StringView& displayPath, StringView& canonicalPath, std::unique_ptr<Reader>& reader);

        private:
            StringPool* stringPool;
            ResourceManager* resourceManager;
            ArrayView<StringView> importDirs;

            StringView startPath;
            StringView currentPath;
            std::unordered_set<StringView> alreadyImportedPaths;
    };
}

#endif