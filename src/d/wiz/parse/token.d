module wiz.parse.token;

enum Token
{
    None,
    EndOfFile,
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
    Low,
    High,
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
    "'~='",
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
    "'<:'",
    "'>:'",
];

enum Branch
{
    Less = Token.Less,
    Greater = Token.Greater,
    LessEqual = Token.LessEqual,
    GreaterEqual = Token.GreaterEqual,
    NotEqual = Token.NotEqual,
    Equal = Token.Equal,
}

enum Prefix
{
    Not = Token.Not,
    Sub = Token.Sub,
    Low = Token.Low,
    High = Token.High,
    Swap = Token.Swap,
    Grouping = Token.LParen,
    Indirection = Token.LBracket,
}

enum Postfix
{
    Inc = Token.Inc,
    Dec = Token.Dec,
}

enum Infix
{
    At = Token.At,
    Add = Token.Add,
    Sub = Token.Sub,
    Mul = Token.Mul,
    Div = Token.Div,
    Mod = Token.Mod,
    AddC = Token.AddC,
    SubC = Token.SubC,
    ShiftL = Token.ShiftL,
    ShiftR = Token.ShiftR,
    RotateL = Token.RotateL,
    RotateR = Token.RotateR,
    RotateLC = Token.RotateLC,
    RotateRC = Token.RotateRC,
    ArithShiftL = Token.ArithShiftL,
    ArithShiftR = Token.ArithShiftR,
    Or = Token.Or,
    And = Token.And,
    Xor = Token.Xor,
    Colon = Token.Colon,
}

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
    Bank,
    In,
    Byte,
    Word,
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
    Loop,
    Until,
    And,
    Or,
    Compare,
    To,
    Push,
    Pop,
    Via,
    Break,
    Continue,
    Assert,
    Inline,
    Unroll,
    Is,
    Func,
    Task,
    InlineAssert,
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
        "bank": Keyword.Bank,
        "in": Keyword.In,
        "byte": Keyword.Byte,
        "word": Keyword.Word,
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
        "loop": Keyword.Loop,
        "until": Keyword.Until,
        "and": Keyword.And,
        "or": Keyword.Or,
        "compare": Keyword.Compare,
        "to": Keyword.To,
        "push": Keyword.Push,
        "pop": Keyword.Pop,
        "via": Keyword.Via,
        "break": Keyword.Break,
        "continue": Keyword.Continue,
        "assert": Keyword.Assert,
        "inline": Keyword.Inline,
        "unroll": Keyword.Unroll,
        "is": Keyword.Is,
        "func": Keyword.Func,
        "task": Keyword.Task,
        "inline assert": Keyword.InlineAssert,
    ];
    
    foreach(name, keyword; keywords)
    {
        keywordNames[keyword] = name;
    }
}

auto getSimpleTokenName(Token token)
{
    return tokenNames[token];
}

auto getBranchName(Branch op)
{
    return tokenNames[op];
}

auto getPrefixName(Prefix op)
{
    return tokenNames[op];
}

auto getPostfixName(Postfix op)
{
    return tokenNames[op];
}

auto getInfixName(Infix op)
{
    return tokenNames[op];
}

auto getVerboseTokenName(Token token, string text)
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

auto getKeywordName(Keyword keyword)
{
    return keywordNames[keyword];
}

auto findKeyword(string text)
{
    return keywords.get(text, Keyword.None);
}