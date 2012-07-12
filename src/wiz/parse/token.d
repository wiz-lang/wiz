module wiz.parse.token;

enum Token
{
    EMPTY,
    EOF,
    INVALID_CHAR,
    // Identifier. Keywords are also identifiers, but have reserved meaning determined later.
    IDENTIFIER,
    // Numeric constants.
    INTEGER,
    HEXADECIMAL,
    BINARY,
    // String literal.
    STRING,
    // Punctuation
    COLON,
    COMMA,
    DOT,
    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    LBRACE,
    RBRACE,
    SEMI,
    EXCLAIM,
    HASH,
    // Operator symbols.
    ADD,
    ADDC,
    INC,
    SUB,
    SUBC,
    DEC,
    MUL,
    DIV,
    MOD,
    AT,
    SET,
    LT,
    GT,
    LE,
    GE,
    NE,
    EQ,
    SWAP,
    SHL,
    SHR,
    NOT,
    AND,
    XOR,
    OR,
    ASHL,
    ASHR,
    ROL,
    ROR,
    ROLC,
    RORC,
}

private string[] tokenNames = [
    "(???)",
    "end-of-file",
    "invalid character",
    // Identifier. Keywords are also identifiers, but have reserved meaning determined later.
    "identifier",
    // Numeric constants.
    "integer constant",
    "hexadecimal constant",
    "binary constant",
    // String literal.
    "string literal",
    // Punctuation
    "':'",
    "','",
    "'.'",
    "'('",
    "')'",
    "'['",
    "']'",
    "'{'",
    "'}'",
    "';'",
    "'!'",
    "'#'",
    // Operator symbols.
    "'+'",
    "'+#'",
    "'++'",
    "'-'",
    "'-#'",
    "'--'",
    "'*'",
    "'/'",
    "'%'",
    "'@'",
    "'='",
    "'<'",
    "'>'",
    "'<='",
    "'>='",
    "'!='",
    "'=='",
    "'<>'",
    "'<<'",
    "'>>'",
    "'~'",
    "'&'",
    "'^'",
    "'|'",
    "'<<-'",
    "'>>-'",
    "'<<<'",
    "'>>>'",
    "'<<<#'",
    "'>>>#'",
];

enum Keyword
{
    NONE,
    DEF,
    LET,
    VAR,
    GOTO,
    WHEN,
    CALL,
    RETURN,
    RESUME,
    SLEEP,
    SUSPEND,
    NOP,
    NOT,
    BANK,
    IN,
    BYTE,
    WORD,
    BEGIN,
    PACKAGE,
    END,
    INCLUDE,
    EMBED,
    ENUM,
    IF,
    THEN,
    ELSE,
    ELSEIF,
    WHILE,
    DO,
    REPEAT,
    UNTIL,
    AND,
    OR,
    FAR,
    COMPARE,
    TO,
    BIT,
    PUSH,
    POP,
    VIA,
    BREAK,
    CONTINUE,
    ABORT,
};

enum Operator
{
    ADD,
    ADDC,
    INC,
    SUB,
    SUBC,
    DEC,
    MUL,
    DIV,
    MOD,
    AT,
    SET,
    LT,
    GT,
    LE,
    GE,
    NE,
    EQ,
    SWAP,
    SHL,
    SHR,
    NOT,
    AND,
    XOR,
    OR,
    ASHL,
    ASHR,
    ROL,
    ROR,
    ROLC,
    RORC,
};

private Keyword[string] keywords;
private string[Keyword] keywordNames;

static this()
{
    keywords = [
        "def": Keyword.DEF,
        "let": Keyword.LET,
        "var": Keyword.VAR,
        "goto": Keyword.GOTO,
        "when": Keyword.WHEN,
        "call": Keyword.CALL,
        "return": Keyword.RETURN,
        "resume": Keyword.RESUME,
        "sleep": Keyword.SLEEP,
        "suspend": Keyword.SUSPEND,
        "nop": Keyword.NOP,
        "not": Keyword.NOT,
        "bank": Keyword.BANK,
        "in": Keyword.IN,
        "byte": Keyword.BYTE,
        "word": Keyword.WORD,
        "begin": Keyword.BEGIN,
        "package": Keyword.PACKAGE,
        "end": Keyword.END,
        "include": Keyword.INCLUDE,
        "embed": Keyword.EMBED,
        "enum": Keyword.ENUM,
        "if": Keyword.IF,
        "then": Keyword.THEN,
        "else": Keyword.ELSE,
        "elseif": Keyword.ELSEIF,
        "while": Keyword.WHILE,
        "do": Keyword.DO,
        "repeat": Keyword.REPEAT,
        "until": Keyword.UNTIL,
        "and": Keyword.AND,
        "or": Keyword.OR,
        "far": Keyword.FAR,
        "compare": Keyword.COMPARE,
        "to": Keyword.TO,
        "bit": Keyword.BIT,
        "push": Keyword.PUSH,
        "pop": Keyword.POP,
        "via": Keyword.VIA,
        "break": Keyword.BREAK,
        "continue": Keyword.CONTINUE,
        "abort": Keyword.ABORT,
    ];
    
    foreach(name, keyword; keywords)
    {
        keywordNames[keyword] = name;
    }
}

string getSimpleTokenName(Token token)
{
    return tokenNames[token];
}

string getVerboseTokenName(Token token, string text)
{
    if(token == Token.STRING)
    {
        text = "\"" ~ text ~ "\"";
    }
    if(text.length > 32)
    {
        text = text[0..29] ~ "...";
    }
    switch(token)
    {
        case Token.IDENTIFIER:
            Keyword keyword = findKeyword(text);
            if(keyword != Keyword.NONE)
            {
                return "keyword '" ~ text ~ "'";
            }
            else
            {
                return "identifier '" ~ text ~ "'";
            }

        case Token.INTEGER:
        case Token.HEXADECIMAL:
        case Token.BINARY:
            return "number '" ~ text ~ "'";
        case Token.STRING:
            return "string " ~ text ~ "";
        default:
            return getSimpleTokenName(token);
    }
}

string getKeywordName(Keyword keyword)
{
    return keywordNames[keyword];
}

Keyword findKeyword(string text)
{
    return keywords.get(text, Keyword.NONE);
}