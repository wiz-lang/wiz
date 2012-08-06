module wiz.parse.token;

enum Token
{
    Empty,
    EOF,
    InvalidChar,
    // Identifier. Keywords are also identifiers, but have reserved meaning determined later.
    Identifier,
    // Numeric constants.
    Integer,
    Hexadecimal,
    Binary,
    // String literal.
    String,
    // Punctuation
    Colon,
    Comma,
    Dot,
    LParen,
    RParen,
    LBracket,
    RBracket,
    LBrace,
    RBrace,
    Semi,
    Exclaim,
    Hash,
    // Operator symbols.
    Add,
    AddC,
    Inc,
    Sub,
    SubC,
    Dec,
    Mul,
    Div,
    Mod,
    At,
    Set,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    NotEqual,
    Equal,
    Swap,
    ShiftL,
    ShiftR,
    Not,
    And,
    Xor,
    Or,
    ArithShiftL,
    ArithShiftR,
    RotateL,
    RotateR,
    RotateLC,
    RotateRC,
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
    if(token == Token.String)
    {
        text = "\"" ~ text ~ "\"";
    }
    if(text.length > 32)
    {
        text = text[0..29] ~ "...";
    }
    switch(token)
    {
        case Token.Identifier:
            Keyword keyword = findKeyword(text);
            if(keyword == Keyword.NONE)
            {
                return "identifier '" ~ text ~ "'";
            }
            return "keyword '" ~ text ~ "'";
        case Token.Integer, Token.Hexadecimal, Token.Binary:
            return "number '" ~ text ~ "'";
        case Token.String:
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