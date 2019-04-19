#ifndef WIZ_PARSER_SCANNER_H
#define WIZ_PARSER_SCANNER_H

#include <cstdint>
#include <string>
#include <memory>
#include <utility>

#include <wiz/utility/string_pool.h>
#include <wiz/utility/source_location.h>

namespace wiz {
    class Reader;
    class Report;
    class Location;
    struct Token;
    enum class TokenType;

    class Scanner {
        public:
            Scanner(std::unique_ptr<Reader> reader, StringView originalPath, StringView expandedPath, StringPool* stringPool, Report* report);
            ~Scanner();

            SourceLocation getLocation() const;
            Token next();

        private:
            enum class State;

            std::unique_ptr<Reader> reader;
            SourceLocation location;
            SourceLocation commentStartLocation;
            StringPool* stringPool;
            Report* report;
            char terminator;
            std::size_t position;
            State state;
            TokenType baseTokenType;
            std::uint8_t intermediateCharCode;

            std::string buffer;
    };
}

#endif
