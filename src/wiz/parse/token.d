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
    None,
    Def,
    Let,
    Var,
    Goto,
    When,
    Call,
    Return,
    Resume,
    Sleep,
    Suspend,
    Nop,
    Not,
    Bank,
    In,
    Byte,
    Word,
    Begin,
    Package,
    End,
    Include,
    Embed,
    Enum,
    If,
    Then,
    Else,
    ElseIf,
    While,
    Do,
    Repeat,
    Until,
    And,
    Or,
    Far,
    Compare,
    To,
    Bit,
    Push,
    Pop,
    Via,
    Break,
    Continue,
    Abort,
};

private Keyword[string] keywords;
private string[Keyword] keywordNames;

static this()
{
    keywords = [
        "def": Keyword.Def,
        "let": Keyword.Let,
        "var": Keyword.Var,
        "goto": Keyword.Goto,
        "when": Keyword.When,
        "call": Keyword.Call,
        "return": Keyword.Return,
        "resume": Keyword.Resume,
        "sleep": Keyword.Sleep,
        "suspend": Keyword.Suspend,
        "nop": Keyword.Nop,
        "not": Keyword.Not,
        "bank": Keyword.Bank,
        "in": Keyword.In,
        "byte": Keyword.Byte,
        "word": Keyword.Word,
        "begin": Keyword.Begin,
        "package": Keyword.Package,
        "end": Keyword.End,
        "include": Keyword.Include,
        "embed": Keyword.Embed,
        "enum": Keyword.Enum,
        "if": Keyword.If,
        "then": Keyword.Then,
        "else": Keyword.Else,
        "elseif": Keyword.ElseIf,
        "while": Keyword.While,
        "do": Keyword.Do,
        "repeat": Keyword.Repeat,
        "until": Keyword.Until,
        "and": Keyword.And,
        "or": Keyword.Or,
        "far": Keyword.Far,
        "compare": Keyword.Compare,
        "to": Keyword.To,
        "bit": Keyword.Bit,
        "push": Keyword.Push,
        "pop": Keyword.Pop,
        "via": Keyword.Via,
        "break": Keyword.Break,
        "continue": Keyword.Continue,
        "abort": Keyword.Abort,
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
            if(keyword == Keyword.None)
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
    return keywords.get(text, Keyword.None);
}