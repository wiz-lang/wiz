#include <wiz/utility/source_location.h>

namespace wiz {
    SourceLocation::SourceLocation()
    : line(0),
    displayPath(),
    canonicalPath() {}

    SourceLocation::SourceLocation(
        StringView path)
    : line(0), 
    displayPath(path),
    canonicalPath(path) {}

    SourceLocation::SourceLocation(
        StringView path,
        std::size_t line)
    : line(line),
    displayPath(path),
    canonicalPath(path) {}

    SourceLocation::SourceLocation(
        StringView originalPath,
        StringView expandedPath,
        std::size_t line)
    : line(line),
    displayPath(originalPath),
    canonicalPath(expandedPath) {}
    
    std::string SourceLocation::toString() const {
        return canonicalPath.getLength() != 0
            ? canonicalPath.toString() + (line > 0 ? ":" + std::to_string(line) : "")
            : "";
    }
}