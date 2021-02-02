#ifndef WIZ_PARSER_TOKEN_H
#define WIZ_PARSER_TOKEN_H

#include <string>
#include <wiz/utility/string_view.h>

namespace wiz {
    enum class Keyword {
        None,
        AlignOf,
        As,
        Bank,
        Break,
        By,
        Case,
        Const,
        Continue,
        Do,
        Default,
        Else,
        Embed,
        Enum,
        Export,
        Extern,
        False,
        Far,
        For,
        Func,
        Goto,
        If,
        In,
        Inline,
        Import,
        IrqReturn,
        Is,
        Let,
        Namespace,
        NmiReturn,
        OffsetOf,
        Private,
        Public,
        Return,
        SizeOf,
        Static,
        Struct,
        Switch,
        True,
        TypeAlias,
        TypeOf,
        Unaligned,
        Union,
        Var,
        Via,
        Void,
        While,
        WriteOnly,

        Count
    };

    enum class TokenType {
        None,
        EndOfFile,
        InvalidChar,
        Identifier, // Keywords are also identifiers, but have reserved meaning determined later.
        Integer,
        Hexadecimal,
        Octal,
        Binary,
        String,
        Character,
        Colon,
        Comma,
        Dot,
        DotDot,
        DotDotDot,
        LeftParenthesis,
        RightParenthesis,
        LeftBracket,
        RightBracket,
        LeftBrace,
        RightBrace,
        Semicolon,
        Hash,
        HashBracket,
        Backtick,
        Plus,
        PlusHash,
        DoublePlus,
        Minus,
        MinusHash,
        DoubleMinus,
        Asterisk,
        Slash,
        Percent,
        At,
        Equals,
        LessThan,
        GreaterThan,
        LessThanEquals,
        GreaterThanEquals,
        ExclamationEquals,
        DoubleEquals,
        LessGreater,
        GreaterHash,
        LessColon,
        GreaterColon,
        HashColon,
        LessGreaterColon,
        GreaterHashColon,
        DoubleGreaterColon,
        Tilde,
        Ampersand,
        Caret,
        Bar,
        DoubleLessThan,
        DoubleGreaterThan,
        TripleLessThan,
        TripleGreaterThan,
        // note to self: rlc/rrc on Z80 does NOT include the carry, it's an 8-bit shift.
        // Meanwhile, rl/rr on Z80 includes the carry, not rlc/rrc like you'd expect. so >>>># extends a >> to 16-bits.
        QuadrupleLessThan,
        QuadrupleGreaterThan,
        QuadrupleLessThanHash,
        QuadrupleGreaterThanHash,
        Exclamation,
        DoubleAmpersand,
        DoubleBar,
        PlusEquals,
        PlusHashEquals,
        MinusEquals,
        MinusHashEquals,
        AsteriskEquals,
        SlashEquals,
        PercentEquals,
        AmpersandEquals,
        CaretEquals,
        BarEquals,
        DoubleLessThanEquals,
        DoubleGreaterThanEquals,
        TripleLessThanEquals,
        TripleGreaterThanEquals,
        QuadrupleLessThanEquals,
        QuadrupleGreaterThanEquals,
        QuadrupleLessThanHashEquals,
        QuadrupleGreaterThanHashEquals,
        Dollar,

        Count
    };

    struct Token {
        Token()
        : type(TokenType::None), keyword(Keyword::None), text() {}

        Token(TokenType type)
        : type(type), keyword(Keyword::None), text() {}

        Token(TokenType type, StringView text)
        : type(type), keyword(Keyword::None), text(text) {}

        Token(TokenType type, Keyword keyword, StringView text)
        : type(type), keyword(keyword), text(text) {}

        TokenType type;
        Keyword keyword;
        StringView text;
    };

    StringView getSimpleTokenName(Token token);
    std::string getVerboseTokenName(Token token);
    StringView getKeywordName(Keyword keyword);
    Keyword findKeyword(StringView text);
}

#endif
