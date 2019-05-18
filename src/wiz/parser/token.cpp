#include <cstring>
#include <unordered_map>

#include <wiz/parser/token.h>
#include <wiz/utility/text.h>

namespace wiz {
    namespace {
        const char* const keywordNames[] = {
            "(no keyword)",
            "alignof",
            "as",
            "bank",
            "break",
            "by",
            "case",
            "const",
            "continue",
            "do",
            "default",
            "else",
            "embed",
            "enum",
            "export",
            "extern",
            "false",
            "far",
            "for",
            "func",
            "goto",
            "if",
            "in",
            "inline",
            "import",
            "irqreturn",
            "is",
            "let",
            "namespace",
            "nmireturn",
            "offsetof",
            "private",
            "public",
            "return",
            "sizeof",
            "static",
            "struct",
            "switch",
            "true",
            "typealias",
            "typeof",
            "union",
            "var",
            "via",
            "void",
            "while",
            "writeonly",
        };

        static_assert(sizeof(keywordNames) / sizeof(*keywordNames) == static_cast<std::size_t>(Keyword::Count), "`keywordNames` table must have an entry for every `Keyword`");

        const char* const tokenNames[] = {
            "nothing",
            "end-of-file",
            "invalid character",
            // Identifier. Keywords are also identifiers, but have reserved meaning determined later.
            "identifier",
            // Literals.
            "integer constant",
            "hexadecimal constant",
            "octal constant",
            "binary constant",
            "string literal",
            "character literal",
            // Punctuation
            "`:`",
            "`,`",
            "`.`",
            "`..`",
            "`...`",
            "`(`",
            "`)`",
            "`[`",
            "`]`",
            "`{`",
            "`}`",
            "`;`",
            "`#`",
            "`#[`",
            "`",
            // Operator symbols.
            "`+`",
            "`+#`",
            "`++`",
            "`-`",
            "`-#`",
            "`--`",
            "`*`",
            "`/`",
            "`%`",
            "`@`",
            "`=`",
            "`<`",
            "`>`",
            "`<=`",
            "`>=`",
            "`!=`",
            "`==`",
            "`<:`",
            "`>:`",
            "`#:`",
            "`~`",
            "`&`",
            "`^`",
            "`|`",
            "`<<`",
            "`>>`",
            "`<<<`",
            "`>>>`",
            "`<<<<`",
            "`>>>>`",
            "`<<<<#`",
            "`>>>>#`",
            "`!`",
            "`&&`",
            "`||`",
            "`+=`",
            "`-=`",
            "`+#=`",
            "`-#=`",
            "`*=`",
            "`/=`",
            "`%=`",
            "`&=`",
            "`^=`",
            "`|=`",
            "`<<=`",
            "`>>=`",
            "`<<<=`",
            "`>>>=`",
            "`<<<<=`",
            "`>>>>=`",
            "`<<<<#=`",
            "`>>>>#=`",
            "`$`",
        };
    }

    static_assert(sizeof(tokenNames) / sizeof(*tokenNames) == static_cast<std::size_t>(TokenType::Count), "`tokenNames` table must have an entry for every `Token`");

    StringView getSimpleTokenName(Token token) {
        return StringView(tokenNames[static_cast<std::size_t>(token.type)]);
    }

    std::string getVerboseTokenName(Token token) {
        const auto MaxLength = 64;
        const auto tokenText = token.text;

        switch (token.type) {
            case TokenType::Identifier: {
                Keyword keyword = findKeyword(tokenText);
                if (keyword == Keyword::None) {
                    return "identifier `" + text::truncate(tokenText, MaxLength) + "`";
                }
                return "keyword `" + tokenText.toString() + "`";
            }
            case TokenType::Integer:
            case TokenType::Hexadecimal:
            case TokenType::Binary:
                return "number `" + text::truncate(tokenText, MaxLength) + "`";
            case TokenType::String: {
                std::string truncatedText = text::truncate(tokenText, MaxLength);
                return "string \"" + text::escape(StringView(truncatedText), '\"') + "\"";
            }
            case TokenType::Character: {
                std::string truncatedText = text::truncate(tokenText, MaxLength);
                return "character '" + text::escape(StringView(truncatedText), '\'') + "'";
            }
            default:
                return getSimpleTokenName(token).toString();
        }
    }

    StringView getKeywordName(Keyword keyword) {
        return StringView(keywordNames[static_cast<std::size_t>(keyword)]);
    }

    Keyword findKeyword(StringView text) {
        static std::unordered_map<StringView, Keyword> keywords;
        if (keywords.empty()) {
            for (std::size_t i = 0; i != sizeof(keywordNames) / sizeof(*keywordNames); ++i) {
                keywords[StringView(keywordNames[i])] = static_cast<Keyword>(i);
            }
        }

        const auto match = keywords.find(text);
        return match != keywords.end() ? match->second : Keyword::None;
    }
}