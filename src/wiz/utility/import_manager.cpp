#include <wiz/utility/text.h>
#include <wiz/utility/path.h>
#include <wiz/utility/reader.h>
#include <wiz/utility/report.h>
#include <wiz/utility/import_manager.h>
#include <wiz/utility/source_location.h>
#include <wiz/utility/resource_manager.h>

namespace wiz {
    namespace {
        const char* const SourceExtension = ".wiz";
    }

    ImportManager::ImportManager(StringPool* stringPool, ResourceManager* resourceManager, ArrayView<StringView> importDirs)
    : stringPool(stringPool), resourceManager(resourceManager), importDirs(importDirs) {}

    StringView ImportManager::getStartPath() const {
        return startPath;
    }

    void ImportManager::setStartPath(StringView value) {
        startPath = value;
    }

    StringView ImportManager::getCurrentPath() const {
        return currentPath;
    }

    void ImportManager::setCurrentPath(StringView value) {
        currentPath = value;
    }

    ImportResult ImportManager::attemptAbsoluteImport(StringView originalPath, StringView attemptedPath, ImportOptions importOptions, StringView& displayPath, StringView& canonicalPath, std::unique_ptr<Reader>& reader) {
        static_cast<void>(originalPath);
        const auto appendExtension = (importOptions & ImportOptions::AppendExtension) != ImportOptions::None;
        const auto allowShellResources = (importOptions & ImportOptions::AllowShellResources) != ImportOptions::None;

        reader = nullptr;

        if (attemptedPath.startsWith("<"_sv) && attemptedPath.endsWith(">"_sv)) {
            displayPath = canonicalPath = attemptedPath;
        } else {
            canonicalPath = stringPool->intern(path::toNormalizedAbsolute(StringView(attemptedPath.toString() + (appendExtension && !attemptedPath.endsWith(StringView(SourceExtension)) ? SourceExtension : ""))));
            displayPath = StringView();

            if (startPath.getLength() != 0) {
                const auto relativeDisplayPath = path::toRelative(canonicalPath, startPath);
                displayPath = stringPool->intern(relativeDisplayPath);
            }

            if (displayPath.getLength() == 0) {
                displayPath = stringPool->intern(path::toNormalized(StringView(attemptedPath.toString() + (appendExtension && !attemptedPath.endsWith(StringView(SourceExtension)) ? SourceExtension : ""))));
            }
        }

        if (alreadyImportedPaths.find(canonicalPath) != alreadyImportedPaths.end()) {
            // Already included, do nothing.
            return ImportResult::AlreadyImported;
        } else {
            reader = resourceManager->openReader(canonicalPath, allowShellResources);
            if (reader != nullptr && reader->isOpen()) {
                alreadyImportedPaths.insert(canonicalPath);
                
                return ImportResult::JustImported;
            } else {
                displayPath = StringView();
                canonicalPath = StringView();
                return ImportResult::Failed;
            }
        }
    }

    ImportResult ImportManager::attemptRelativeImport(StringView originalPath, ImportOptions importOptions, StringView& displayPath, StringView& canonicalPath, std::unique_ptr<Reader>& reader) {
        return attemptAbsoluteImport(originalPath, StringView(path::getDirectory(currentPath).toString() + "/" + originalPath.toString()), importOptions, displayPath, canonicalPath, reader);
    }

    ImportResult ImportManager::importModule(StringView originalPath, ImportOptions importOptions, StringView& displayPath, StringView& canonicalPath, std::unique_ptr<Reader>& reader) {
        if (originalPath.startsWith("./"_sv) || originalPath.startsWith("../"_sv)) {
            const auto result = attemptRelativeImport(originalPath, importOptions, displayPath, canonicalPath, reader);
            if (result != ImportResult::Failed) {
                return result;
            }
        } else {
            {
                const auto result = attemptRelativeImport(originalPath, importOptions, displayPath, canonicalPath, reader);
                if (result != ImportResult::Failed) {
                    return result;
                }
            }
            for (const auto& dir : importDirs) {
                const auto sanitizedDir = dir.findLastOf("/\\"_sv) >= dir.getLength()
                    ? path::getDirectory(dir)
                    : dir;

                const auto result = attemptAbsoluteImport(originalPath, StringView(sanitizedDir.toString() + "/" + originalPath.toString()), importOptions, displayPath, canonicalPath, reader);
                if (result != ImportResult::Failed) {
                    return result;
                }
            }
        }

        displayPath = StringView();
        canonicalPath = StringView();
        return ImportResult::Failed;
    }
}