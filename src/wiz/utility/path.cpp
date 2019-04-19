#if defined(_WIN32)
    #include <direct.h>
    #define GETCWD _getcwd
#elif !defined(__EMSCRIPTEN__)
    #include <unistd.h>
    #define GETCWD getcwd
#endif

#include <cstdlib>
#include <numeric>
#include <vector>
#include <algorithm>

#include <wiz/utility/path.h>
#include <wiz/utility/text.h>

namespace wiz {
    namespace {
        const StringView SeparatorChars("/\\");
        const char Separator = '/';
        const char Separator2 = '\\';
        const StringView Dot(".");
        const StringView DotDot("..");

        bool isSeparator(char c) {
            return c == Separator || c == Separator2;
        }
    }

    namespace path {
        std::string getCurrentWorkingDirectory() {
#if defined(GETCWD)
            auto buffer = GETCWD(nullptr, 0);
            if (buffer != nullptr) {
                std::string result(buffer); 
                free(buffer);
                return result;
            }
#endif

            return "";
        }

        // Converts a path into an absolute path that has been normalized.
        // For absolute paths, it just normalizes them.
        // For relative paths, turns them into absolute paths relative to the current working directory, and then normalizes them.
        std::string toNormalizedAbsolute(StringView path) {
            const auto absPrefix = getAbsolutePrefix(path);

            if (absPrefix.length() > 0) {
                return toNormalized(path);
            } else {
                const auto cwd = getCurrentWorkingDirectory();
                return toNormalized(cwd.length() > 0 ? StringView(cwd + Separator + path.toString()) : path);
            }
        }

        // Tries to collapse redundant .. and . out of paths, while preserving absolute path prefixes.
        std::string toNormalized(StringView path) {
            const auto absPrefix = getAbsolutePrefix(path);

            auto pieces = text::split(path, SeparatorChars, absPrefix.length());            
            for (std::size_t i = 0; i < pieces.size(); ) {
                const auto piece = pieces[i];
                if (piece.getLength() == 0 || piece == Dot) {
                    pieces.erase(pieces.begin() + i);
                } else if (piece == DotDot) {
                    if (i == 0 && absPrefix.length() > 0) {
                        pieces.erase(pieces.begin() + i);
                    } else if (i > 0 && pieces[i - 1] != DotDot) {
                        --i;
                        pieces.erase(pieces.begin() + i, pieces.begin() + i + 2);
                    } else {
                        ++i;
                    }
                } else {
                    ++i;
                }
            }

            bool separator = false;
            std::string result;

            if (absPrefix.length() > 0) {
                result = absPrefix;
            }

            for (const auto& piece : pieces) {
                if (separator) {
                    result += Separator + piece.toString();
                } else {
                    result += piece.toString();
                }
                separator = true;
            }

            return result;
        }

        // Returns a relative path, provided an path and an origin to make the relative path from. Both must be normalized absolute paths.
        std::string toRelative(StringView path, StringView origin) {
            const auto commonPrefixOffset = findCommonDirectoryPrefix(path, origin);
            if (commonPrefixOffset != SIZE_MAX) {
                std::size_t levelsAbove = 0;
                for (std::size_t i = commonPrefixOffset + 1; i != origin.getLength(); ++i) {
                    if (isSeparator(origin[i])) {
                        ++levelsAbove;
                    }
                }

                std::string result;
                result.reserve((DotDot.getLength() + 1) * levelsAbove);
                for (std::size_t i = 0; i != levelsAbove; ++i) {
                    result += DotDot.toString();
                    result += Separator;
                }

                result += path.sub(commonPrefixOffset + 1).toString();
                return result;
            }
            return "";
        }

        // Returns a string containing the absolute path prefix part of a path, but with slashes normalized.
        std::string getAbsolutePrefix(StringView path) {
            const auto pathLength = path.getLength();
            if (pathLength >= 1 && isSeparator(path[0])) {
                // Leading slash.
                return std::string(1, Separator);
            } else if (pathLength >= 3 && path[1] == ':' && isSeparator(path[2])) {
                // Windows drive letter: eg. C:\folder\folder
                return path.sub(0, 2).toString() + Separator;
            } else if (pathLength >= 2 && isSeparator(path[0]) && isSeparator(path[1])) {
                // Windows UNC path: eg. \\SERVER\DRIVE\folder\folder
                const auto i = path.findFirstOf(SeparatorChars, 2);                
                if (i < path.getLength()) {
                    const auto j = path.findFirstOf(SeparatorChars, i + 1);
                    if (j < path.getLength()) {
                        return path.sub(0, i).toString() + Separator + path.sub(i + 1, j).toString() + Separator;
                    } else {
                        return path.toString();
                    }
                }
            }
            return "";
        }

        std::size_t findCommonDirectoryPrefix(StringView path, StringView path2) {
            if (path2.getLength() < path.getLength()) {
                std::swap(path, path2);
            }
            const auto mismatch = std::mismatch(path.begin(), path.end(), path2.begin(), path2.end());
            const auto commonStringPrefixLength = std::distance(path.begin(), mismatch.first);
            const auto lastSlashOffset = path.findLastOf(SeparatorChars, commonStringPrefixLength);
            if (lastSlashOffset != SIZE_MAX) {
                return lastSlashOffset;
            }
            return SIZE_MAX;
        }

        StringView getDirectory(StringView path) {
            const auto i = path.findLastOf(SeparatorChars);
            if (i >= path.getLength()) {
                return "."_sv;
            }
            return path.sub(0, i);
        }

        StringView getFilename(StringView path) {
            const auto i = path.findLastOf(SeparatorChars);
            if (i >= path.getLength()) {
                return path;
            } else {
                return path.sub(i + 1);
            }
        }

        StringView getExtension(StringView path) {
            const auto i = path.rfind("."_sv);
            if (i >= path.getLength()) {
                return StringView();
            } else {
                return path.sub(i + 1);
            }
        }

        StringView stripExtension(StringView path) {
            const auto i = path.rfind("."_sv);
            if (i >= path.getLength()) {
                return path;
            }
            return path.sub(0, i);
        }
    }
}
