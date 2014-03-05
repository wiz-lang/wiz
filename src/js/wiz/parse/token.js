(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.parse = typeof(wiz.parse) !== 'undefined' ? wiz.parse : {};

    var tk = 0;
    var Token = wiz.parse.Token =  {
        None: '$tk' + (tk++),
        EndOfFile: '$tk' + (tk++),
        InvalidChar: '$tk' + (tk++),
        // Identifier. Keywords are also identifiers, but have reserved meaning determined later.
        Identifier: '$tk' + (tk++),
        // Numeric constants.
        Integer: '$tk' + (tk++),
        Hexadecimal: '$tk' + (tk++),
        Binary: '$tk' + (tk++),
        // String literal.
        String: '$tk' + (tk++),
        // Punctuation
        Colon: '$tk' + (tk++),
        Comma: '$tk' + (tk++),
        Dot: '$tk' + (tk++),
        LParen: '$tk' + (tk++),
        RParen: '$tk' + (tk++),
        LBracket: '$tk' + (tk++),
        RBracket: '$tk' + (tk++),
        LBrace: '$tk' + (tk++),
        RBrace: '$tk' + (tk++),
        Semi: '$tk' + (tk++),
        Exclaim: '$tk' + (tk++),
        Hash: '$tk' + (tk++),
        // Operator symbols.
        Add: '$tk' + (tk++),
        AddC: '$tk' + (tk++),
        Inc: '$tk' + (tk++),
        Sub: '$tk' + (tk++),
        SubC: '$tk' + (tk++),
        Dec: '$tk' + (tk++),
        Mul: '$tk' + (tk++),
        Div: '$tk' + (tk++),
        Mod: '$tk' + (tk++),
        At: '$tk' + (tk++),
        Set: '$tk' + (tk++),
        Less: '$tk' + (tk++),
        Greater: '$tk' + (tk++),
        LessEqual: '$tk' + (tk++),
        GreaterEqual: '$tk' + (tk++),
        NotEqual: '$tk' + (tk++),
        Equal: '$tk' + (tk++),
        Swap: '$tk' + (tk++),
        ShiftL: '$tk' + (tk++),
        ShiftR: '$tk' + (tk++),
        Not: '$tk' + (tk++),
        And: '$tk' + (tk++),
        Xor: '$tk' + (tk++),
        Or: '$tk' + (tk++),
        ArithShiftL: '$tk' + (tk++),
        ArithShiftR: '$tk' + (tk++),
        RotateL: '$tk' + (tk++),
        RotateR: '$tk' + (tk++),
        RotateLC: '$tk' + (tk++),
        RotateRC: '$tk' + (tk++),
    };
    for(var k in Token) {
        Token[Token[k]] = Token[k];
    }

    var tokenNames = {
        None: "(???)",
        EndOfFile: "end-of-file",
        InvalidChar: "invalid character",
        // Identifier. Keywords are also identifiers, but have reserved meaning determined later.
        Identifier: "identifier",
        // Numeric constants.// Numeric constants.
        Integer: "integer constant",
        Hexadecimal: "hexadecimal constant",
        Binary: "binary constant",
        // String literal.// String literal.
        String: "string literal",
        // Punctuation// Punctuation
        Colon: "':'",
        Comma: "','",
        Dot: "'.'",
        LParen: "'('",
        RParen: "')'",
        LBracket: "'['",
        RBracket: "']'",
        LBrace: "'{'",
        RBrace: "'}'",
        Semi: "';'",
        Exclaim: "'!'",
        Hash: "'#'",
        // Operator symbols.// Operator symbols.
        Add: "'+'",
        AddC: "'+#'",
        Inc: "'++'",
        Sub: "'-'",
        SubC: "'-#'",
        Dec: "'--'",
        Mul: "'*'",
        Div: "'/'",
        Mod: "'%'",
        At: "'@'",
        Set: "'='",
        Less: "'<'",
        Greater: "'>'",
        LessEqual: "'<='",
        GreaterEqual: "'>='",
        NotEqual: "'~='",
        Equal: "'=='",
        Swap: "'<>'",
        ShiftL: "'<<'",
        ShiftR: "'>>'",
        Not: "'~'",
        And: "'&'",
        Xor: "'^'",
        Or: "'|'",
        ArithShiftL: "'<<-'",
        ArithShiftR: "'>>-'",
        RotateL: "'<<<'",
        RotateR: "'>>>'",
        RotateLC: "'<<<#'",
        RotateRC: "'>>>#'",
    };
    for(var k in tokenNames) {
        tokenNames[Token[k]] = tokenNames[k];
    }

    var Branch = wiz.parse.Branch = {
        Less: Token.Less,
        Greater: Token.Greater,
        LessEqual: Token.LessEqual,
        GreaterEqual: Token.GreaterEqual,
        NotEqual: Token.NotEqual,
        Equal: Token.Equal,
    };
    for(var k in Branch) {
        Branch[Branch[k]] = Branch[k];
    }

    var Prefix = wiz.parse.Prefix = {
        Not: Token.Not,
        Sub: Token.Sub,
        Low: Token.Less,
        High: Token.Greater,
        Swap: Token.Swap,
        Grouping: Token.LParen,
        Indirection: Token.LBracket,
    };
    for(var k in Prefix) {
        Prefix[Prefix[k]] = Prefix[k];
    }

    var Postfix = wiz.parse.Postfix = {
        Inc: Token.Inc,
        Dec: Token.Dec,
    };
    for(var k in Postfix) {
        Postfix[Postfix[k]] = Postfix[k];
    }

    var Infix = wiz.parse.Infix = {
        At: Token.At,
        Add: Token.Add,
        Sub: Token.Sub,
        Mul: Token.Mul,
        Div: Token.Div,
        Mod: Token.Mod,
        AddC: Token.AddC,
        SubC: Token.SubC,
        ShiftL: Token.ShiftL,
        ShiftR: Token.ShiftR,
        RotateL: Token.RotateL,
        RotateR: Token.RotateR,
        RotateLC: Token.RotateLC,
        RotateRC: Token.RotateRC,
        ArithShiftL: Token.ArithShiftL,
        ArithShiftR: Token.ArithShiftR,
        Or: Token.Or,
        And: Token.And,
        Xor: Token.Xor,
        Colon: Token.Colon,
    };
    for(var k in Infix) {
        Infix[Infix[k]] = Infix[k];
    }

    var kw = 0;
    var Keyword = wiz.parse.Keyword = {
        None: '$kw' + (kw++),
        Def: '$kw' + (kw++),
        Let: '$kw' + (kw++),
        Var: '$kw' + (kw++),
        Goto: '$kw' + (kw++),
        When: '$kw' + (kw++),
        Call: '$kw' + (kw++),
        Return: '$kw' + (kw++),
        Resume: '$kw' + (kw++),
        Sleep: '$kw' + (kw++),
        Suspend: '$kw' + (kw++),
        Nop: '$kw' + (kw++),
        Bank: '$kw' + (kw++),
        In: '$kw' + (kw++),
        Byte: '$kw' + (kw++),
        Word: '$kw' + (kw++),
        Package: '$kw' + (kw++),
        End: '$kw' + (kw++),
        Include: '$kw' + (kw++),
        Embed: '$kw' + (kw++),
        Enum: '$kw' + (kw++),
        If: '$kw' + (kw++),
        Then: '$kw' + (kw++),
        Else: '$kw' + (kw++),
        ElseIf: '$kw' + (kw++),
        While: '$kw' + (kw++),
        Do: '$kw' + (kw++),
        Loop: '$kw' + (kw++),
        Until: '$kw' + (kw++),
        And: '$kw' + (kw++),
        Or: '$kw' + (kw++),
        Compare: '$kw' + (kw++),
        To: '$kw' + (kw++),
        Push: '$kw' + (kw++),
        Pop: '$kw' + (kw++),
        Via: '$kw' + (kw++),
        Break: '$kw' + (kw++),
        Continue: '$kw' + (kw++),
        Abort: '$kw' + (kw++),
        Inline: '$kw' + (kw++),
        Unroll: '$kw' + (kw++),
        Is: '$kw' + (kw++),
        Func: '$kw' + (kw++),
        Task: '$kw' + (kw++),
    };
    for(var k in Keyword) {
        Keyword[Keyword[k]] = Keyword[k];
    }

    var keywords = {
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
        "abort": Keyword.Abort,
        "inline": Keyword.Inline,
        "unroll": Keyword.Unroll,
        "is": Keyword.Is,
        "func": Keyword.Func,
        "task": Keyword.Task,
    };
        
    var keywordNames = {};
    for(var name in keywords) {
        if(keywords.hasOwnProperty(name)) {
            keywordNames[keywords[name]] = name;
        }
    }

    wiz.parse.getSimpleTokenName = function(token) {
        return tokenNames[token];
    };

    wiz.parse.getVerboseTokenName = function(token, text) {
        if(token == Token.String)
        {
            text = "\"" + text + "\"";
        }
        if(text.length > 32)
        {
            text = text.substring(0, 29) + "...";
        }
        switch(token)
        {
            case Token.Identifier:
                var keyword = wiz.parse.findKeyword(text);
                if(keyword == Keyword.None)
                {
                    return "identifier '" + text + "'";
                }
                return "keyword '" + text + "'";
            case Token.Integer:
            case Token.Hexadecimal:
            case Token.Binary:
                return "number '" + text + "'";
            case Token.String:
                return "string " + text + "";
            default:
                return wiz.parse.getSimpleTokenName(token);
        }
    };

    wiz.parse.getKeywordName = function(keyword) {
        return keywordNames[keyword];
    };

    wiz.parse.findKeyword = function(text) {
        return keywords[text] || Keyword.None;
    };

})(this);