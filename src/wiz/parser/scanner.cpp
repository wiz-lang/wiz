#include <cstring>
#include <utility>

#include <wiz/utility/text.h>
#include <wiz/utility/report.h>
#include <wiz/utility/reader.h>
#include <wiz/parser/token.h>
#include <wiz/parser/scanner.h>

namespace wiz {
    namespace {
        const char* const ErrorText = "<error>";
    }

    enum class Scanner::State {
        Start,
        String,
        StringEscape,
        HexEscapeFirstDigit,
        HexEscapeSecondDigit,
        LeadingZero,
        IntegerDigits,
        HexadecimalDigits,
        OctalDigits,
        BinaryDigits,
        LiteralSuffix,
        Identifier,
        Asterisk,
        Percent,
        Slash,
        DoubleSlashComment,
        SlashStarComment,
        SlashStarCommentStar,
        Exclamation,
        Plus,
        PlusHash,
        Minus,
        MinusHash,
        Equals,
        Ampersand,
        Bar,
        Caret,
        LessThan,
        LessThanGreaterThan,
        DoubleLessThan,
        TripleLessThan,
        QuadrupleLessThan,
        QuadrupleLessThanHash,
        GreaterThan,
        GreaterThanHash,
        DoubleGreaterThan,
        TripleGreaterThan,
        QuadrupleGreaterThan,
        QuadrupleGreaterThanHash,
        Dot,
        DoubleDot,
        Hash,
    };

    Scanner::Scanner(
        std::unique_ptr<Reader> reader,
        StringView originalPath,
        StringView expandedPath,
        StringPool* stringPool,
        Report* report)
    : reader(std::move(reader)),
    location(originalPath, expandedPath, 0),
    commentStartLocation(originalPath, expandedPath, 0),
    stringPool(stringPool),
    report(report),
    terminator(0),
    position(0),
    state(State::Start),
    baseTokenType(TokenType::None),
    intermediateCharCode(0) {}

    Scanner::~Scanner() {}

    SourceLocation Scanner::getLocation() const {
        return location;
    }

    Token Scanner::next() {
        std::string text;
        while (true) {
            while (position < buffer.length()) {
                char c = buffer[position];
                switch (state) {
                    case State::Start:
                        switch (c) {
                            case '0':
                                state = State::LeadingZero;
                                text += c;
                                break;
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                            case '8':
                            case '9':
                                state = State::IntegerDigits;
                                text += c;
                                break;
                            case '_':
                            case 'a':
                            case 'b':
                            case 'c':
                            case 'd':
                            case 'e':
                            case 'f':
                            case 'g':
                            case 'h':
                            case 'i':
                            case 'j':
                            case 'k':
                            case 'l':
                            case 'm':
                            case 'n':
                            case 'o':
                            case 'p':
                            case 'q':
                            case 'r':
                            case 's':
                            case 't':
                            case 'u':
                            case 'v':
                            case 'w':
                            case 'x':
                            case 'y':
                            case 'z':
                            case 'A':
                            case 'B':
                            case 'C':
                            case 'D':
                            case 'E':
                            case 'F':
                            case 'G':
                            case 'H':
                            case 'I':
                            case 'J':
                            case 'K':
                            case 'L':
                            case 'M':
                            case 'N':
                            case 'O':
                            case 'P':
                            case 'Q':
                            case 'R':
                            case 'S':
                            case 'T':
                            case 'U':
                            case 'V':
                            case 'W':
                            case 'X':
                            case 'Y':
                            case 'Z':
                                state = State::Identifier;
                                text += c;
                                break;
                            case '\'': case '\"':
                                terminator = c;
                                state = State::String;
                                break;
                            case ' ': case '\t': case '\r': case '\n': break;
                            case ':': position++; return Token(TokenType::Colon);
                            case ',': position++; return Token(TokenType::Comma);
                            case '.': state = State::Dot; break;
                            case '(': position++; return Token(TokenType::LeftParenthesis);
                            case ')': position++; return Token(TokenType::RightParenthesis);
                            case '[': position++; return Token(TokenType::LeftBracket);
                            case ']': position++; return Token(TokenType::RightBracket);
                            case '{': position++; return Token(TokenType::LeftBrace);
                            case '}': position++; return Token(TokenType::RightBrace);
                            case ';': position++; return Token(TokenType::Semicolon);
                            case '#': state = State::Hash; break;
                            case '|': state = State::Bar; break;
                            case '*': state = State::Asterisk; break;
                            case '%': state = State::Percent; break;
                            case '@': position++; return Token(TokenType::At);
                            case '&': state = State::Ampersand; break;
                            case '^': state = State::Caret; break;
                            case '!': state = State::Exclamation; break;
                            case '`': position++; return Token(TokenType::Backtick);
                            case '~': position++; return Token(TokenType::Tilde);
                            case '+': state = State::Plus; break;
                            case '-': state = State::Minus; break;
                            case '/': state = State::Slash; break;
                            case '=': state = State::Equals; break;
                            case '<': state = State::LessThan; break;
                            case '>': state = State::GreaterThan; break;
                            case '$': position++; return Token(TokenType::Dollar);
                            default:
                                report->error("unrecognized character `\\" + std::string(1, c) + "` found", location);
                                break;
                        }
                        break;
                    case State::Identifier:
                        switch (c) {
                            case '_':
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                            case '8':
                            case '9':
                            case 'a':
                            case 'b':
                            case 'c':
                            case 'd':
                            case 'e':
                            case 'f':
                            case 'g':
                            case 'h':
                            case 'i':
                            case 'j':
                            case 'k':
                            case 'l':
                            case 'm':
                            case 'n':
                            case 'o':
                            case 'p':
                            case 'q':
                            case 'r':
                            case 's':
                            case 't':
                            case 'u':
                            case 'v':
                            case 'w':
                            case 'x':
                            case 'y':
                            case 'z':
                            case 'A':
                            case 'B':
                            case 'C':
                            case 'D':
                            case 'E':
                            case 'F':
                            case 'G':
                            case 'H':
                            case 'I':
                            case 'J':
                            case 'K':
                            case 'L':
                            case 'M':
                            case 'N':
                            case 'O':
                            case 'P':
                            case 'Q':
                            case 'R':
                            case 'S':
                            case 'T':
                            case 'U':
                            case 'V':
                            case 'W':
                            case 'X':
                            case 'Y':
                            case 'Z':
                                text += c;
                                break;
                            default: {
                                state = State::Start;
                                const auto internedText = stringPool->intern(text);
                                return Token(TokenType::Identifier, findKeyword(internedText), internedText);
                            }
                        }
                        break;
                    case State::String:
                        if (c == terminator) {
                            position++;
                            state = State::Start;
                            switch (terminator) {
                                case '\'':
                                    if (text.size() != 1) {
                                        report->error("invalid character literal '" + text::escape(StringView(text), '\'') + "' (character literals must be exactly one character)", location);
                                        return Token(TokenType::Character, StringView(ErrorText));
                                    } else {
                                        return Token(TokenType::Character, stringPool->intern(text));
                                    }
                                default: return Token(TokenType::String, stringPool->intern(text));
                            }
                        } else switch (c) {
                            case '\\':
                                state = State::StringEscape;
                                break;
                            default:
                                text += c;
                                break;
                        }
                        break;
                    case State::StringEscape:
                        state = State::String;
                        switch (c) {
                            case '\"': case '\'': case '\\':
                                text += c;
                                break;
                            case 't': text += '\t'; break;
                            case 'r': text += '\r'; break;
                            case 'n': text += '\n'; break;
                            case 'f': text += '\f'; break;
                            case 'b': text += '\b'; break;
                            case 'a': text += '\a'; break;
                            case '0': text += '\0'; break;
                            case 'x':
                                state = State::HexEscapeFirstDigit;
                                break;
                            default:
                                report->error("invalid escape sequence `\\" + std::string(1, c) + "` in quoted literal", location);
                                break;
                        }
                        break;
                    case State::HexEscapeFirstDigit: {
                        state = State::HexEscapeSecondDigit;
                        switch (c) {
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                            case '8':
                            case '9':
                                intermediateCharCode = static_cast<std::uint8_t>(c - '0') << 4;
                                break;
                            case 'a':
                            case 'b':
                            case 'c':
                            case 'd':
                            case 'e':
                            case 'f':
                                intermediateCharCode = static_cast<std::uint8_t>(c - 'a' + 10) << 4;
                                break;
                            case 'A':
                            case 'B':
                            case 'C':
                            case 'D':
                            case 'E':
                            case 'F':
                                intermediateCharCode = static_cast<std::uint8_t>(c - 'A' + 10) << 4;
                                break;
                            default:
                                state = State::String;
                                report->error("hex escape `\\x` contains illegal character `" + std::string(1, c) + "`", location);
                                break;
                        }
                        break;
                    }
                    case State::HexEscapeSecondDigit: {
                        state = State::String;
                        switch (c) {
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                            case '8':
                            case '9':
                                text += static_cast<char>(intermediateCharCode | static_cast<std::uint8_t>(c - '0'));
                                break;
                            case 'a':
                            case 'b':
                            case 'c':
                            case 'd':
                            case 'e':
                            case 'f':
                                text += static_cast<char>(intermediateCharCode | static_cast<std::uint8_t>(c - 'a' + 10));
                                break;
                            case 'A':
                            case 'B':
                            case 'C':
                            case 'D':
                            case 'E':
                            case 'F':
                                text += static_cast<char>(intermediateCharCode | static_cast<std::uint8_t>(c - 'A' + 10));
                                break;
                            default:
                                state = State::String;
                                report->error("hex escape `\\x` contains illegal character `" + std::string(1, c) + "`", location);
                                break;
                        }
                        break;
                    }
                    case State::LeadingZero:
                        switch (c) {
                            case '_':
                                state = State::IntegerDigits;
                                break;
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                            case '8':
                            case '9':
                                state = State::IntegerDigits;
                                text += c;
                                break;
                            case 'x': state = State::HexadecimalDigits; text += c; break;
                            case 'b': state = State::BinaryDigits; text += c; break;
                            case 'o': state = State::OctalDigits; text += c; break;
                            case 'u': case 'i':
                                text += c;
                                baseTokenType = TokenType::Integer;
                                state = State::LiteralSuffix;
                                break;
                            default:
                                state = State::Start;
                                return Token(TokenType::Integer, stringPool->intern(text));
                        }
                        break;
                    case State::IntegerDigits:
                        switch (c) {
                            case '_':
                                break;
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                            case '8':
                            case '9':
                                text += c;
                                break;
                            case 'u': case 'i':
                                text += c;
                                baseTokenType = TokenType::Integer;
                                state = State::LiteralSuffix;
                                break;
                            default:
                                state = State::Start;
                                return Token(TokenType::Integer, stringPool->intern(text));
                        }
                        break;
                    case State::HexadecimalDigits:
                        switch (c) {
                            case '_':
                                break;
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                            case '8':
                            case '9':
                            case 'a':
                            case 'b':
                            case 'c':
                            case 'd':
                            case 'e':
                            case 'f':
                            case 'A':
                            case 'B':
                            case 'C':
                            case 'D':
                            case 'E':
                            case 'F':
                                text += c;
                                break;
                            case 'u': case 'i':
                                text += c;
                                baseTokenType = TokenType::Hexadecimal;
                                state = State::LiteralSuffix;
                                break;
                            default:
                                state = State::Start;
                                return Token(TokenType::Hexadecimal, stringPool->intern(text));
                        }
                        break;
                    case State::OctalDigits:
                        switch (c) {
                            case '_':
                                break;
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                                text += c;
                                break;
                            case 'u': case 'i':
                                text += c;
                                baseTokenType = TokenType::Octal;
                                state = State::LiteralSuffix;
                                break;
                            default:
                                state = State::Start;
                                return Token(TokenType::Octal, stringPool->intern(text));
                        }
                        break;
                    case State::BinaryDigits:
                        switch (c) {
                            case '_':
                                break;
                            case '0': case '1':
                                text += c;
                                break;
                            case 'u': case 'i':
                                text += c;
                                baseTokenType = TokenType::Binary;
                                state = State::LiteralSuffix;
                                break;
                            default:
                                state = State::Start;
                                return Token(TokenType::Binary, stringPool->intern(text));
                        }
                        break;
                    case State::LiteralSuffix:
                        switch (c) {
                            case '_':
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                            case '8':
                            case '9':
                            case 'a':
                            case 'b':
                            case 'c':
                            case 'd':
                            case 'e':
                            case 'f':
                            case 'g':
                            case 'h':
                            case 'i':
                            case 'j':
                            case 'k':
                            case 'l':
                            case 'm':
                            case 'n':
                            case 'o':
                            case 'p':
                            case 'q':
                            case 'r':
                            case 's':
                            case 't':
                            case 'u':
                            case 'v':
                            case 'w':
                            case 'x':
                            case 'y':
                            case 'z':
                            case 'A':
                            case 'B':
                            case 'C':
                            case 'D':
                            case 'E':
                            case 'F':
                            case 'G':
                            case 'H':
                            case 'I':
                            case 'J':
                            case 'K':
                            case 'L':
                            case 'M':
                            case 'N':
                            case 'O':
                            case 'P':
                            case 'Q':
                            case 'R':
                            case 'S':
                            case 'T':
                            case 'U':
                            case 'V':
                            case 'W':
                            case 'X':
                            case 'Y':
                            case 'Z':
                                text += c;
                                break;
                            default: {
                                state = State::Start;
                                return Token(baseTokenType, Keyword::None, stringPool->intern(text));
                            }
                        }
                        break;
                    case State::Exclamation:
                        state = State::Start;
                        switch (c) {
                            case '=': position++; return Token(TokenType::ExclamationEquals);
                            default: return Token(TokenType::Exclamation);
                        }
                        break;
                    case State::Plus:
                        state = State::Start;
                        switch (c) {
                            case '#': state = State::PlusHash; break;
                            case '+': position++; return Token(TokenType::DoublePlus);
                            case '=': position++; return Token(TokenType::PlusEquals);
                            default: return Token(TokenType::Plus);
                        }
                        break;
                    case State::PlusHash:
                        state = State::Start;
                        switch (c) {
                            case '=': position++; return Token(TokenType::PlusHashEquals);
                            default: return Token(TokenType::PlusHash);
                        }
                        break;
                    case State::Minus:
                        state = State::Start;
                        switch (c) {
                            case '#': state = State::MinusHash; break;
                            case '-': position++; return Token(TokenType::DoubleMinus);
                            case '=': position++; return Token(TokenType::MinusEquals);
                            case '0':
                                state = State::LeadingZero;
                                text += '-';
                                text += c;
                                break;
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                            case '8':
                            case '9':
                                state = State::IntegerDigits;
                                text += '-';
                                text += c;
                                break;
                            default: return Token(TokenType::Minus);
                        }
                        break;
                    case State::MinusHash:
                        state = State::Start;
                        switch (c) {
                            case '=': position++; return Token(TokenType::MinusHashEquals);
                            default: return Token(TokenType::MinusHash);
                        }
                        break;
                    case State::Asterisk:
                        state = State::Start;
                        switch (c) {
                            case '=': position++; return Token(TokenType::AsteriskEquals);
                            default: return Token(TokenType::Asterisk);
                        }
                        break;
                    case State::Percent:
                        state = State::Start;
                        switch (c) {
                            case '=': position++; return Token(TokenType::PercentEquals);
                            default: return Token(TokenType::Percent);
                        }
                        break;
                    case State::Slash:
                        state = State::Start;
                        switch (c) {
                            case '/':
                                state = State::DoubleSlashComment;
                                break;
                            case '*':
                                state = State::SlashStarComment;
                                commentStartLocation.line = location.line;
                                break;
                            case '=': position++; return Token(TokenType::SlashEquals);
                            default:
                                return Token(TokenType::Slash);
                        }
                        break;
                    case State::DoubleSlashComment: break;
                    case State::SlashStarComment:
                        switch (c) {
                            case '*': state = State::SlashStarCommentStar;
                            default: break;
                        }
                        break;
                    case State::SlashStarCommentStar:
                        switch (c) {
                            case '/': state = State::Start; break;
                            default: state = State::SlashStarComment; break;
                        }
                        break;
                    case State::Equals:
                        state = State::Start;
                        switch (c) {
                            case '=': position++; return Token(TokenType::DoubleEquals);
                            default: return Token(TokenType::Equals);
                        }
                        break;
                    case State::Ampersand:
                        state = State::Start;
                        switch (c) {
                            case '&': position++; return Token(TokenType::DoubleAmpersand);
                            case '=': position++; return Token(TokenType::AmpersandEquals);
                            default: return Token(TokenType::Ampersand);
                        }
                        break;
                    case State::Bar:
                        state = State::Start;
                        switch (c) {
                            case '|': position++; return Token(TokenType::DoubleBar);
                            case '=': position++; return Token(TokenType::BarEquals);
                            default: return Token(TokenType::Bar);
                        }
                        break;
                    case State::Caret:
                        state = State::Start;
                        switch (c) {
                            case '=': position++; return Token(TokenType::CaretEquals);
                            default: return Token(TokenType::Caret);
                        }
                        break;
                    case State::LessThan:
                        state = State::Start;
                        switch (c) {
                            case '<': state = State::DoubleLessThan; break;
                            case '>': state = State::LessThanGreaterThan; break;
                            case '=': position++; return Token(TokenType::LessThanEquals);
                            case ':': position++; return Token(TokenType::LessColon);
                            default: return Token(TokenType::LessThan);
                        }
                        break;
                    case State::LessThanGreaterThan:
                        state = State::Start;
                        switch (c) {
                            case ':': position++; return Token(TokenType::LessGreaterColon);
                            default: return Token(TokenType::LessGreater);
                        }
                        break;
                    case State::DoubleLessThan:
                        state = State::Start;
                        switch (c) {
                            case '<': state = State::TripleLessThan; break;
                            case '=': position++; return Token(TokenType::DoubleLessThanEquals);
                            default: return Token(TokenType::DoubleLessThan);
                        }
                        break;
                    case State::TripleLessThan:
                        state = State::Start;
                        switch (c) {
                            case '<': state = State::QuadrupleLessThan; break;
                            case '=': position++; return Token(TokenType::TripleLessThanEquals);
                            default: return Token(TokenType::TripleLessThan);
                        }
                        break;
                    case State::QuadrupleLessThan:
                        state = State::Start;
                        switch (c) {
                            case '#': state = State::QuadrupleLessThanHash; break;
                            case '=': position++; return Token(TokenType::QuadrupleLessThanEquals);
                            default: return Token(TokenType::QuadrupleLessThan);
                        }
                        break;
                    case State::QuadrupleLessThanHash:
                        state = State::Start;
                        switch (c) {                            
                            case '=': position++; return Token(TokenType::QuadrupleLessThanHashEquals);
                            default: return Token(TokenType::QuadrupleLessThanHash);
                        }
                        break;
                    case State::GreaterThan:
                        state = State::Start;
                        switch (c) {
                            case '>': state = State::DoubleGreaterThan; break;
                            case '#': state = State::GreaterThanHash; break;
                            case '=': position++; return Token(TokenType::GreaterThanEquals);
                            case ':': position++; return Token(TokenType::GreaterColon);
                            default: return Token(TokenType::GreaterThan);
                        }
                        break;
                    case State::GreaterThanHash:
                        state = State::Start;
                        switch (c) {
                            case ':': position++; return Token(TokenType::GreaterHashColon);
                            default: return Token(TokenType::GreaterHash);
                        }
                        break;
                    case State::DoubleGreaterThan:
                        state = State::Start;
                        switch (c) {
                            case '>': state = State::TripleGreaterThan; break;
                            case ':': position++; return Token(TokenType::DoubleGreaterColon);
                            case '=': position++; return Token(TokenType::DoubleGreaterThanEquals);
                            default: return Token(TokenType::DoubleGreaterThan);
                        }
                        break;
                    case State::TripleGreaterThan:
                        state = State::Start;
                        switch (c) {
                            case '>': state = State::QuadrupleGreaterThan; break;
                            case '=': position++; return Token(TokenType::TripleGreaterThanEquals);
                            default: return Token(TokenType::TripleGreaterThan);
                        }
                        break;
                    case State::QuadrupleGreaterThan:
                        state = State::Start;
                        switch (c) {
                            case '#': state = State::QuadrupleGreaterThanHash; break;
                            case '=': position++; return Token(TokenType::QuadrupleGreaterThanEquals);
                            default: return Token(TokenType::QuadrupleGreaterThan);
                        }
                        break;
                    case State::QuadrupleGreaterThanHash:
                        state = State::Start;
                        switch (c) {                            
                            case '=': position++; return Token(TokenType::QuadrupleGreaterThanHashEquals);
                            default: return Token(TokenType::QuadrupleGreaterThanHash);
                        }
                        break;
                    case State::Dot:
                        state = State::Start;
                        switch (c) {
                            case '.': state = State::DoubleDot; break;
                            default: return Token(TokenType::Dot);
                        }
                        break;
                    case State::DoubleDot:
                        state = State::Start;
                        switch (c) {
                            case '.': position++; return Token(TokenType::DotDotDot);
                            default: return Token(TokenType::DotDot);
                        }
                        break;
                    case State::Hash:
                        state = State::Start;
                        switch (c) {
                            case '[': position++; return Token(TokenType::HashBracket);
                            case ':': position++; return Token(TokenType::HashColon);
                            default: return Token(TokenType::Hash);
                        }
                        break;
                }
                position++;
            }

            if (reader && reader->isOpen() && reader->readLine(buffer)) {
                // Special handling in states for end-of-line.
                switch (state) {
                    case State::DoubleSlashComment:
                        state = State::Start;
                        break;
                    case State::String:
                        state = State::Start;
                        report->error("expected closing quote `" + std::string(1, terminator) + "`, but got end-of-line", location);
                        break;
                    case State::StringEscape:
                        state = State::Start;
                        report->error("expected string escape sequence, but got end-of-line", location);
                        break;
                    default:
                        break;
                }
                position = 0;
                location.line++;
            }
            else {
                // End-of-file.
                switch (state) {
                    case State::Identifier: {
                        state = State::Start;
                        const auto internedText = stringPool->intern(text);
                        return Token(TokenType::Identifier, findKeyword(internedText), internedText);
                    }
                    case State::LeadingZero:
                    case State::IntegerDigits:
                        state = State::Start;
                        return Token(TokenType::Integer, stringPool->intern(text));
                    case State::HexadecimalDigits:
                        state = State::Start;
                        return Token(TokenType::Hexadecimal, stringPool->intern(text));
                    case State::BinaryDigits:
                        state = State::Start;
                        return Token(TokenType::Binary, stringPool->intern(text));
                    case State::String:
                        state = State::Start;
                        report->error("expected closing quote `" + std::string(1, terminator) + "`, but got end-of-file", location);
                        break;
                    case State::StringEscape:
                        state = State::Start;
                        report->error("expected string escape sequence, but got end-of-file", location);
                        break;
                    case State::SlashStarComment:
                        report->error("expected `*/` to close comment `/*`, but got end-of-file", location, ReportErrorFlags::Continued);
                        report->error("comment `/*` started here", commentStartLocation);
                        break;
                    default:
                        break;
                }
                return Token(TokenType::EndOfFile);
            }
        }
        return Token(TokenType::EndOfFile);
    }
}