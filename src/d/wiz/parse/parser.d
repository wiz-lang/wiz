module wiz.parse.parser;

static import std.conv;
static import std.path;
static import std.array;
static import std.stdio;
static import std.string;
static import std.algorithm;

import wiz.lib;
import wiz.parse.lib;

class Parser
{
    private Scanner scanner;
    private Scanner[] includes;
    private bool[string] included;
    private Token token;
    private string text;
    private Keyword keyword;

    this(Scanner scanner)
    {
        this.scanner = scanner;

        auto fn = std.path.dirName(scanner.getLocation().file)
            ~ std.path.dirSeparator ~ std.path.baseName(scanner.getLocation().file);
        included[fn] = true;
    }
    
    void nextToken()
    {
        token = scanner.next();
        text = scanner.getLastText();
        keyword = Keyword.None;
        if(token == Token.Identifier)
        {
            keyword = findKeyword(text);
        }
    }
    
    void reject(string expectation = null, bool advance = true)
    {
        reject(token, text, expectation, advance);
    }

    void reject(Token token, string text, string expectation = null, bool advance = true)
    {
        if(expectation is null)
        {
            error("unexpected " ~ getVerboseTokenName(token, text), scanner.getLocation());
        }
        else
        {
            error("expected " ~ expectation ~ ", but got " ~ getVerboseTokenName(token, text) ~ " instead", scanner.getLocation());
        }
        if(advance)
        {
            nextToken();
        }
    }

    bool consume(Token expected)
    {
        if(token == expected)
        {
            nextToken();
            return true;
        }
        else
        {
            reject(getSimpleTokenName(expected));
            return false;
        }     
    }
    
    bool checkIdentifier(bool allowKeywords = false)
    {
        if(token == Token.Identifier && (!allowKeywords && keyword == Keyword.None || allowKeywords))
        {
            return true; 
        }
        else
        {
            error("expected identifier but got " ~ getVerboseTokenName(token, text) ~ " instead", scanner.getLocation());
            return false;
        }
    }
    
    bool checkIdentifier(Keyword[] permissibleKeywords)
    {
        checkIdentifier(true);
        
        if(keyword == Keyword.None || std.algorithm.find(permissibleKeywords, keyword).length > 0)
        {
            return true;
        }
        else
        {
            string[] keywordNames = [];
            foreach(keyword; permissibleKeywords)
            {
                keywordNames ~= "keyword '" ~ getKeywordName(keyword) ~ "'";
            }
            keywordNames[keywordNames.length - 1] = "or " ~ keywordNames[keywordNames.length - 1];
            
            error("expected identifier, " ~ std.array.join(keywordNames, ", ") ~ ", but got " ~ getVerboseTokenName(token, text) ~ " instead", scanner.getLocation());
            return false;
        }
    }
    
    auto parse()
    {
        nextToken();
        auto program = parseProgram();
        compile.verify();
        return program;
    }
    
    bool handleEndOfFile()
    {
        if(token == Token.EndOfFile)
        {
            if(includes.length == 0)
            {
                return true;
            }
            else
            {
                // Remove include guard.
                included.remove(scanner.getLocation().file);
                // Pop previous scanner off stack.
                auto old = scanner;
                scanner = std.array.back(includes);
                std.array.popBack(includes);
                // Ready a new token for the scanner.
                nextToken();
                return false;
            }
        }
        return false;
    }

    auto parseProgram()
    {
        // program = (include | statement)* EOF
        auto location = scanner.getLocation();
        ast.Statement[] statements;
        while(true)
        {
            if(handleEndOfFile())
            {
                break;
            }
            if(token == Token.EndOfFile)
            {
                continue;
            }
            if(keyword == Keyword.End || keyword == Keyword.Else || keyword == Keyword.ElseIf)
            {
                reject();
            }
            if(auto statement = parseStatement())
            {
                statements ~= statement;
            }
        }
        return new ast.Block(statements, location);
    }
    
    ast.Statement[] parseCompound()
    {
        // compound = statement* 'end'
        ast.Statement[] statements;
        while(true)
        {
            if(handleEndOfFile())
            {
                reject("'end'");
                return null;
            }
            if(keyword == Keyword.End)
            {
                return statements;
            }
            if(keyword == Keyword.Else || keyword == Keyword.ElseIf)
            {
                reject("'end'");
            }
            if(auto statement = parseStatement())
            {
                statements ~= statement;
            }
        }
    }

    ast.Statement[] parseConditionalPrelude()
    {
        // preconditional_compound = statement* 'is'
        ast.Statement[] statements;
        while(true)
        {
            if(handleEndOfFile())
            {
                reject("'is'");
                return null;
            }
            if(keyword == Keyword.Is)
            {
                return statements;
            }
            if(keyword == Keyword.End || keyword == Keyword.Else || keyword == Keyword.ElseIf)
            {
                reject("'is'");
            }
            if(auto statement = parseStatement())
            {
                statements ~= statement;
            }
        }
    }
    
    ast.Statement[] parseConditionalBlock()
    {
        // conditional_compound = statement* ('else' | 'elseif' | 'end')
        ast.Statement[] statements;
        while(true)
        {
            if(handleEndOfFile())
            {
                reject("'end'");
                return null;
            }
            if(keyword == Keyword.End || keyword == Keyword.Else || keyword == Keyword.ElseIf)
            {
                return statements;
            }
            if(auto statement = parseStatement())
            {
                statements ~= statement;
            }
        }
    }

    ast.Statement parseStatement()
    {
        // statement =
        //      embed
        //      | relocation
        //      | block
        //      | bank
        //      | label
        //      | let
        //      | var
        //      | func
        //      | inline
        //      | data
        //      | jump
        //      | conditional
        //      | loop
        //      | comparison
        //      | command
        //      | assignment
        switch(token)
        {
            case Token.Identifier:
                switch(keyword)
                {
                    case Keyword.Include:
                        parseInclude();
                        return null;
                    case Keyword.Embed:
                        return parseEmbed();
                    case Keyword.In:
                        return parseRelocation();
                    case Keyword.Do, Keyword.Package:
                        return parseBlock();
                    case Keyword.Bank:
                        return parseBankDecl();
                    case Keyword.Def:
                        return parseLabelDecl();
                    case Keyword.Let:
                        return parseLetDecl();
                    case Keyword.Var:
                        return parseVarDecl();
                    case Keyword.Func, Keyword.Task:
                        return parseFuncDecl();
                    case Keyword.Inline:
                        return parseInline();
                    case Keyword.Byte, Keyword.Word:
                        return parseData();
                    case Keyword.Goto, Keyword.Call,
                        Keyword.Return, Keyword.Resume,
                        Keyword.Break, Keyword.Continue,
                        Keyword.While, Keyword.Until,
                        Keyword.Abort, Keyword.Sleep, Keyword.Suspend, Keyword.Nop:
                        return parseJump();
                    case Keyword.If:
                        return parseConditional();
                    case Keyword.Loop:
                        return parseLoop();
                    case Keyword.Compare:
                        return parseComparison();
                    case Keyword.Push:
                        return parsePush();
                    case Keyword.None:
                        // Some unreserved identifier. Try and parse as a term in an assignment!
                        return parseAssignment();
                    default:
                        reject("statement");
                        break;
                }
                break;
            case Token.Integer, Token.Hexadecimal, Token.Binary, Token.String, Token.LParen:
                reject("statement", false);
                skipAssignment(true);
                break;
            case Token.Set:
                reject("statement", false);
                skipAssignment(false);
                break;
            case Token.LBracket:
                return parseAssignment();
                break;
            case Token.Semi:
                // semi-colon, skip.
                nextToken();
                break;
            default:
                reject("statement");
                break;
        }
        return null;
    }
        
    void parseInclude()
    {
        // include = 'include' STRING
        nextToken(); // IDENTIFIER (keyword 'include')
        
        string filename = null;
        if(token == Token.String)
        {
            // Don't call nextToken() here, we'll be doing that when the scanner's popped off later.
            filename = text;
        }
        else
        {
            consume(Token.String);
            return;
        }
        
        // Make the filename relative to its current source.
        filename = std.path.dirName(scanner.getLocation().file) ~ std.path.dirSeparator ~ filename;

        // Make sure the start path is included in the list of included filenames.
        if(included.length == 0)
        {
            string cur = scanner.getLocation().file;
            included[std.path.dirName(cur) ~ std.path.dirSeparator ~ cur] = true;
        }
        // Already included. Skip!
        if(included.get(filename, false))
        {
            nextToken(); // STRING
            return;
        }
        
        // Push old scanner onto stack.
        includes ~= scanner;
        // Add include guard.
        included[filename] = true;
        
        // Open the new file.
        std.stdio.File file;
        try
        {
            file = std.stdio.File(filename, "rb");
        }
        catch(Exception e)
        {
            // If file fails to open, then file will be not be open. Ignore exceptions.
        }
        if(file.isOpen())
        {
            // Swap scanner.
            scanner = new Scanner(file, filename);
        }
        else
        {
            error("could not include file '" ~ filename ~ "'", scanner.getLocation(), true);
        }
        // Now, ready the first token of the file.
        nextToken();
    }
    
    auto parseEmbed()
    {
        // embed = 'embed' STRING
        auto location = scanner.getLocation();
        nextToken(); // IDENTIFIER (keyword 'embed')
        
        if(token == Token.String)
        {
            string filename = text;
            nextToken(); // STRING
            
            // Make the filename relative to its current source.
            filename = std.path.dirName(scanner.getLocation().file) ~ std.path.dirSeparator ~ filename;   
            return new ast.Embed(filename, location);
        }
        else
        {
            consume(Token.String);
            return null;
        }
    }
    
    auto parseRelocation()
    {
        // relocation = 'in' IDENTIFIER (',' expression)? ':'
        auto location = scanner.getLocation();
        string name;
        ast.Expression dest;
        
        nextToken(); // IDENTIFIER (keyword 'in')
        if(checkIdentifier())
        {
            name = text;
        }
        nextToken(); // IDENTIFIER
        // (, expr)?
        if(token == Token.Comma)
        {
            nextToken(); // ,
            dest = parseExpression(); // expression
        }
        consume(Token.Colon); // :
        
        return new ast.Relocation(name, dest, location);
    }
    
    auto parseBlock()
    {
        // block = ('package' IDENTIFIER | 'do') statement* 'end'
        auto location = scanner.getLocation();
        switch(keyword)
        {
            case Keyword.Do:
                nextToken(); // IDENTIFIER (keyword 'do')
                auto statements = parseCompound(); // compound statement
                nextToken(); // IDENTIFIER (keyword 'end')
                return new ast.Block(statements, location);
            case Keyword.Package:
                string name;
                
                nextToken(); // IDENTIFIER (keyword 'package')
                if(checkIdentifier())
                {
                    name = text;
                }
                nextToken(); // IDENTIFIER
                if(keyword == Keyword.Do)
                {
                    nextToken(); // IDENTIFIER (keyword 'do')
                }
                else
                {
                    reject("'do'");
                }

                auto statements = parseCompound(); // compound statement
                nextToken(); // IDENTIFIER (keyword 'end')
                return new ast.Block(name, statements, location);
            default:
                error("unexpected compilation error: incorrectly classified token as start of block statement", scanner.getLocation());
                assert(false);
        }
    }
    
    auto parseBankDecl()
    {
        // bank = 'bank' IDENTIFIER (',' IDENTIFIER)* ':' IDENTIFIER '*' expression
        auto location = scanner.getLocation();
        
        ast.Expression[] indices;
        string[] names;
        string type;
        ast.Expression size;
        
        nextToken(); // IDENTIFIER (keyword 'bank')
        

        if(token == Token.Hash)
        {
            nextToken();
            indices ~= parseExpression();
        }
        else
        {
            indices ~= null;
        }
        if(checkIdentifier())
        {
            names ~= text;
        }
        nextToken(); // IDENTIFIER
        
        // Check if we should match (',' id)*
        while(token == Token.Comma)
        {
            nextToken(); // ,
            if(token == Token.Hash)
            {
                nextToken();
                indices ~= parseExpression();
            }
            else
            {
                indices ~= null;
            }
            if(token == Token.Identifier)
            {
                if(checkIdentifier())
                {
                    // parse name
                    names ~= text;
                }
                nextToken(); // IDENTIFIER
            }
            else
            {
                reject("identifier after ',' in bank declaration");
                break;
            }
        }
        
        consume(Token.Colon); // :
        
        if(checkIdentifier())
        {
            type = text;
        }
        nextToken(); // IDENTIFIER (bank type)
        consume(Token.Mul); // *
        size = parseExpression(); // term
        
        return new ast.BankDecl(indices, names, type, size, location);
    }
    
    auto parseLabelDecl()
    {
        // label = 'def' IDENTIFIER ':'
        auto location = scanner.getLocation();
        
        string name;
        
        nextToken(); // IDENTIFIER (keyword 'def')
        if(checkIdentifier())
        {
            name = text;
        }
        nextToken(); // IDENTIFIER
        consume(Token.Colon);
        
        return new ast.LabelDecl(name, location);
    }

    auto parseStorage()
    {
        // storage = ('byte' | 'word') ('*' expression)?
        auto location = scanner.getLocation();
        if(checkIdentifier(true))
        {
            Keyword storageType;
            switch(keyword)
            {
                case Keyword.Byte:
                case Keyword.Word:
                    storageType = keyword;
                    break;
                default:
                    error("invalid type specifier '" ~ text ~ "'. only 'byte' and 'word' are allowed.", scanner.getLocation());
            }
            nextToken(); // IDENTIFIER (keyword 'byte'/'word')
            // ('*' array_size)?
            ast.Expression arraySize;
            if(token == Token.Mul)
            {
                nextToken(); // *
                arraySize = parseExpression(); // expression
            } 
            return new ast.Storage(storageType, arraySize, location);
        }
        else
        {
            reject("type specifier");
            return null;
        }
    }
    
    auto parseLetDecl()
    {
        // let = 'let' IDENTIFIER '=' expression
        auto location = scanner.getLocation();
        
        string name;
        ast.Storage storage;
        ast.Expression value;
        
        nextToken(); // IDENTIFIER (keyword 'let')
        if(checkIdentifier())
        {
            name = text;
        }
        nextToken(); // IDENTIFIER
        consume(Token.Set); // =
        value = parseExpression(); // expression
        return new ast.LetDecl(name, value, location);
    }
    
    auto parseVarDecl()
    {
        // var = 'var' IDENTIFIER (',' IDENTIFIER)*
        //      ':' ('byte' | 'word') '*' expression
        auto location = scanner.getLocation();
        
        string[] names;
        nextToken(); // IDENTIFIER (keyword 'var')
        
        if(checkIdentifier())
        {
            names ~= text;
        }
        nextToken(); // IDENTIFIER
        
        // Check if we should match (',' id)*
        while(token == Token.Comma)
        {
            nextToken(); // ,
            if(token == Token.Identifier)
            {
                if(checkIdentifier())
                {
                    // parse name
                    names ~= text;
                }
                nextToken(); // IDENTIFIER
            }
            else
            {
                reject("identifier after ',' in variable declaration");
                break;
            }
        }
        
        consume(Token.Colon); // :
        auto storage = parseStorage();
        return new ast.VarDecl(names, storage, location);
    }

    auto parseFuncDecl()
    {
        // func = 'func' IDENTIFIER 'do' statement* 'end'
        auto location = scanner.getLocation();

        string name;
        Keyword type = keyword;

        nextToken(); // IDENTIFIER (keyword 'func')

        if(checkIdentifier())
        {
            name = text;
        }
        nextToken(); // IDENTIFIER

        if(keyword == Keyword.Do)
        {
            nextToken(); // IDENTIFIER (keyword 'do')
        }
        else
        {
            reject("'do'");
        }
        auto block = new ast.Block(parseCompound(), location); // statement*
        nextToken(); // IDENTIFIER (keyword 'end')

        return new ast.FuncDecl(type, name, block, location);   
    }

    ast.Statement parseInline()
    {
        // inline = 'inline' func | call
        auto location = scanner.getLocation();

        string name;

        nextToken(); // IDENTIFIER (keyword 'inline')
        if(keyword == Keyword.Func)
        {
            auto func = parseFuncDecl();
            func.inlined = true;
            return func;
        }
        else if(keyword == Keyword.Task)
        {
            auto func = parseFuncDecl();
            error("'inline task' is not a valid construct, try 'inline func' instead.", location);
            return null;
        }
        else if(keyword == Keyword.Call)
        {
            bool far;
            nextToken(); // IDENTIFIER (keyword 'call')
            if(token == Token.Exclaim)
            {
                nextToken(); // '!'
                far = true;
            }
            auto destination = parseExpression();
            ast.JumpCondition condition = null;
            if(token == Token.Identifier && keyword == Keyword.When)
            {
                nextToken(); // IDENTIFIER (keyword 'when')
                condition = parseJumpCondition("'when'");
            }
            return new ast.Jump(Keyword.Inline, far, destination, condition, location);
        }
        else if(keyword == Keyword.If)
        {
            return parseInlineConditional();
        }
        else if(keyword == Keyword.For)
        {
            return parseInlineFor();
        }
        else if(keyword == Keyword.Abort)
        {
            bool far;
            nextToken(); // IDENTIFIER (keyword 'abort')
            return new ast.Jump(Keyword.InlineAbort, far, null, null, location);
        }
        else
        {
            reject("'func'");
            return null;
        }
    }
    
    auto parseData()
    {
        // data = ('byte' | 'word') data_item (',' data_item)*
        //      where data_item = expression | STRING
        auto location = scanner.getLocation();
        auto storage = parseStorage();
        consume(Token.Colon); // :
        
        ast.Expression[] items;

        // item (',' item)*
        while(true)
        {
            ast.Expression expr = parseExpression(); // expression
            items ~= expr;
            // (',' item)*
            if(token == Token.Comma)
            {
                nextToken(); // ,
                continue;
            }
            break;
        }
        return new ast.Data(storage, items, location);
    }
    
    auto parseJump()
    {
        // jump = 'goto' expression ('when' jump_condition)?
        //      | 'call' expression ('when' jump_condition)?
        //      | 'return' ('when' jump_condition)?
        //      | 'resume' ('when' jump_condition)?
        //      | 'break' ('when' jump_condition)?
        //      | 'continue' ('when' jump_condition)?
        //      | 'while' jump_condition
        //      | 'until' jump_condition
        //      | 'assert'
        //      | 'sleep'
        //      | 'suspend'
        //      | 'nop'
        auto location = scanner.getLocation();

        auto type = keyword;
        bool far = false;
        nextToken(); // IDENTIFIER (keyword)
        if(token == Token.Exclaim)
        {
            nextToken(); // '!'
            far = true;
        }
        switch(type)
        {
            case Keyword.Goto, Keyword.Call:
                auto destination = parseExpression();
                ast.JumpCondition condition = null;
                if(token == Token.Identifier && keyword == Keyword.When)
                {
                    nextToken(); // IDENTIFIER (keyword 'when')
                    condition = parseJumpCondition("'when'");
                }
                return new ast.Jump(type, far, destination, condition, location);
            case Keyword.Return, Keyword.Resume, Keyword.Break, Keyword.Continue:
                ast.JumpCondition condition = null;
                if(token == Token.Identifier && keyword == Keyword.When)
                {
                    nextToken(); // IDENTIFIER (keyword 'when')
                    condition = parseJumpCondition("'when'");
                }
                return new ast.Jump(type, far, condition, location);
            case Keyword.While:
                return new ast.Jump(type, far, parseJumpCondition("'while'"), location);
            case Keyword.Until:
                return new ast.Jump(type, far, parseJumpCondition("'until'"), location);
            default:
                return new ast.Jump(type, far, location);
        }
    }
    
    auto parseJumpCondition(string context)
    {
        // jump_condition = '~'* (IDENTIFIER | '~=' | '==' | '<' | '>' | '<=' | '>=')
        ast.JumpCondition condition = null;
        
        // '~'*
        bool negated = false;
        while(token == Token.Identifier && keyword == Keyword.Not)
        {
            nextToken(); // 'not'
            negated = !negated;
            context = "'not'";
        }

        switch(token)
        {
            case Token.Identifier:
                auto attr = parseAttribute();
                return new ast.JumpCondition(negated, attr, scanner.getLocation());
            case Token.NotEqual:
            case Token.Equal:
            case Token.Less:
            case Token.Greater:
            case Token.LessEqual:
            case Token.GreaterEqual:
                auto type = cast(Branch) token;
                nextToken(); // operator token
                return new ast.JumpCondition(negated, type, scanner.getLocation());
            default:
                reject("condition after " ~ context);
                return null;
        }        
    }

    auto isPossibleJumpCondition()
    {
        switch(token)
        {
            case Token.Identifier:
                return keyword == Keyword.None || keyword == Keyword.Not;
            case Token.NotEqual:
            case Token.Equal:
            case Token.Less:
            case Token.Greater:
            case Token.LessEqual:
            case Token.GreaterEqual:
                return true;
            default:
                return false;
        }
    }

    auto isPossibleAssignmentExpression()
    {
        if(isPrefixToken())
        {
            return true;
        }
        switch(token)
        {
            case Token.Identifier:
                return keyword == Keyword.None;
            case Token.Integer, Token.Hexadecimal, Token.Binary, Token.String, Token.LParen:
                return true;
            default:
                return false;
        }
    }
    
    auto parseConditional()
    {
        // conditional = 'if' statement* 'is' condition 'then' statement*
        //      ('elseif' statement* 'is' condition 'then' statement*)*
        //      ('else' statement)? 'end'
        ast.Conditional first = null;
        ast.Conditional statement = null;
        
        // 'if' condition 'then' statement* ('elseif' condition 'then' statement*)*
        do
        {
            auto location = scanner.getLocation();
            nextToken(); // IDENTIFIER (keyword 'if' / 'elseif')
            
            bool far = false;
            if(token == Token.Exclaim)
            {
                nextToken(); // '!'
                far = true;
            }

            ast.Statement[] preludeStatements;
            ast.JumpCondition condition;
            bool simpleConditional;
            if(isPossibleJumpCondition())
            {
                bool useFallback = isPossibleAssignmentExpression();
                auto exprLocation = scanner.getLocation();

                condition = parseJumpCondition("'if'");
                if(keyword == Keyword.Then)
                {
                    simpleConditional = true;
                }
                else if(useFallback)
                {
                    preludeStatements ~= parseAssignmentInner(exprLocation, condition.attr);
                }
                else
                {
                    reject("'then'");
                }
            }

            if(!simpleConditional)
            {
                preludeStatements ~= parseConditionalPrelude();  // statement*

                if(keyword == Keyword.Is)
                {
                    nextToken(); // IDENTIFIER (keyword 'is')
                }
                else
                {
                    reject("'is'");
                }

                condition = parseJumpCondition("'is'"); // condition
            }

            auto prelude = new ast.Block(preludeStatements, location);
            
            if(keyword == Keyword.Then)
            {
                nextToken(); // IDENTIFIER (keyword 'then')
            }
            else
            {
                reject("'then'");
            }
            
            auto block = new ast.Block(parseConditionalBlock(), location); // statement*
            
            // Construct if statement/
            auto previous = statement;
            statement = new ast.Conditional(condition, far, prelude, block, location);

            // If this is an 'elseif', join to previous 'if'/'elseif'.
            if(previous)
            {
                previous.alternative = statement;
            }
            else if(first is null)
            {
                first = statement;
            }
        } while(keyword == Keyword.ElseIf);
        
        // ('else' statement*)? 'end' (with error recovery for an invalid trailing else/elseif placement)
        if(keyword == Keyword.Else)
        {
            auto location = scanner.getLocation();
            nextToken(); // IDENTIFIER (keyword 'else')
            statement.alternative = new ast.Block(parseConditionalBlock(), location); // statement*
        }
        switch(keyword)
        {
            case Keyword.Else:
                error("duplicate 'else' clause found.", scanner.getLocation());
                break;
            case Keyword.ElseIf:
                // Seeing as we loop on elseif before an else/end, this must be an illegal use of elseif.
                error("'elseif' can't appear after 'else' clause.", scanner.getLocation());
                break;
            default:
        }

        if(keyword == Keyword.End)
        {
            nextToken(); // IDENTIFIER (keyword 'end')
        }
        else
        {
            reject("'end'");
        }
        return first;
    }

    auto parseInlineConditional()
    {
        // conditional = 'if' statement* 'is' condition 'then' statement*
        //      ('elseif' statement* 'is' condition 'then' statement*)*
        //      ('else' statement)? 'end'
        ast.InlineConditional first = null;
        ast.InlineConditional statement = null;
        
        // 'if' condition 'then' statement* ('elseif' condition 'then' statement*)*
        do
        {
            auto location = scanner.getLocation();
            nextToken(); // IDENTIFIER (keyword 'if' / 'elseif')
            
            if(token == Token.Exclaim)
            {
                nextToken(); // '!'
            }

            bool negated = false;
            while(token == Token.Identifier && keyword == Keyword.Not)
            {
                nextToken(); // 'not'
                negated = !negated;
            }

            auto condition = parseExpression();
            
            if(keyword == Keyword.Then)
            {
                nextToken(); // IDENTIFIER (keyword 'then')
            }
            else
            {
                reject("'then'");
            }
            
            auto block = new ast.Block(parseConditionalBlock(), location); // statement*
            
            // Construct if statement/
            auto previous = statement;
            statement = new ast.InlineConditional(negated, condition, block, location);

            // If this is an 'elseif', join to previous 'if'/'elseif'.
            if(previous)
            {
                previous.alternative = statement;
            }
            else if(first is null)
            {
                first = statement;
            }
        } while(keyword == Keyword.ElseIf);
        
        // ('else' statement*)? 'end' (with error recovery for an invalid trailing else/elseif placement)
        if(keyword == Keyword.Else)
        {
            auto location = scanner.getLocation();
            nextToken(); // IDENTIFIER (keyword 'else')
            statement.alternative = new ast.Block(parseConditionalBlock(), location); // statement*
        }
        switch(keyword)
        {
            case Keyword.Else:
                error("duplicate 'else' clause found.", scanner.getLocation());
                break;
            case Keyword.ElseIf:
                // Seeing as we loop on elseif before an else/end, this must be an illegal use of elseif.
                error("'elseif' can't appear after 'else' clause.", scanner.getLocation());
                break;
            default:
        }

        if(keyword == Keyword.End)
        {
            nextToken(); // IDENTIFIER (keyword 'end')
        }
        else
        {
            reject("'end'");
        }
        return first;
    }

    auto parseLoop()
    {
        // loop = 'loop' statement* 'end'
        auto location = scanner.getLocation();
        nextToken(); // IDENTIFIER (keyword 'loop')
        bool far = false;
        if(token == Token.Exclaim)
        {
            nextToken(); // '!'
            far = true;
        }
        auto block = new ast.Block(parseCompound(), location); // statement*
        nextToken(); // IDENTIFIER (keyword 'end')
        return new ast.Loop(block, far, location);
    }

    auto parseInlineFor()
    {
        // unroll = 'inline' 'for' 'let' identifier '=' expression, expression, expression? ':' statement
        auto location = scanner.getLocation();
        nextToken(); // IDENTIFIER (keyword 'for')

        string name;
        if(keyword == Keyword.Let)
        {
            nextToken(); // IDENTIFIER (keyword 'let')
            if(checkIdentifier())
            {
                name = text;
            }
            nextToken(); // IDENTIFIER
            consume(Token.Set); // =
        }
        else
        {
            reject("'let'");
        }

        ast.Expression start = parseExpression();
        consume(Token.Comma); // ,
        ast.Expression end = parseExpression();

        ast.Expression step = null;
        if(token == Token.Comma)
        {
            consume(Token.Comma); // ,
            step = parseExpression();
        }

        if(keyword == Keyword.Do)
        {
            nextToken(); // IDENTIFIER (keyword 'do')
        }
        else
        {
            reject("'do'");
        }
        auto block = new ast.Block(parseCompound(), location); // statement*
        nextToken(); // IDENTIFIER (keyword 'end')

        return new ast.InlineFor(name, start, end, step, block, location);
    }

    auto parseComparison()
    {
        // comparison = 'compare' expression ('to' expression)?
        auto location = scanner.getLocation();
        nextToken(); // IDENTIFIER (keyword 'compare')
        auto term = parseExpression();
        if(keyword == Keyword.To)
        {
            nextToken(); // IDENTIFIER (keyword 'to')
            ast.Expression other = parseExpression();
            return new ast.Comparison(term, other, location);
        }
        else
        {
            return new ast.Comparison(term, location);
        }
    }

    auto parsePush()
    {
        // command = command_token expression
        auto location = scanner.getLocation();
        auto command = keyword;
        nextToken(); // IDENTIFIER (keyword)
        auto argument = parseExpression();
        if(token == Token.Identifier && keyword == Keyword.Via)
        {
            nextToken(); // IDENTIFIER (keyword 'via')
            auto intermediary = parseTerm(); // term
            return new ast.Push(argument, intermediary, location);
        }
        return new ast.Push(argument, location);
    }

    void skipAssignment(bool leadingExpression)
    {
        // Some janky error recovery. Gobble an expression.
        if(leadingExpression)
        {
            parseExpression(); // expr
        }
        // If the expression is followed by an assignment, then gobble the assignment.
        if(token == Token.Set)
        {
            nextToken(); // =
            // If the thing after the = is an expression, then gobble that too.
            switch(token)
            {
                case Token.Integer, Token.Hexadecimal, Token.Binary, Token.String, Token.LParen:
                    parseExpression();
                    break;
                case Token.Identifier:
                    if(keyword == Keyword.None || keyword == Keyword.Pop)
                    {
                        parseExpression();
                    }
                    break;
                default:
            }
        }
    }

    auto parseAssignment()
    {
        // assignment = assignable_term ('=' expression ('via' term)? | postfix_token)
        auto location = scanner.getLocation();
        auto dest = parseAssignableTerm(); // term
        return parseAssignmentInner(location, dest);
    }

    auto parseAssignmentInner(compile.Location location, ast.Expression dest)
    {
        auto op = token;
        auto opText = text;
        if(token == Token.Set)
        {
            nextToken(); // =
            auto src = parseExpression(); // expression
            if(token == Token.Identifier && keyword == Keyword.Via)
            {
                nextToken(); // IDENTIFIER (keyword 'via')
                auto intermediary = parseTerm(); // term
                return new ast.Assignment(dest, intermediary, src, location);
            }
            else
            {
                return new ast.Assignment(dest, src, location);
            }
        }
        else if(isPostfixToken())
        {
            nextToken(); // postfix_token
            return new ast.Assignment(dest, cast(Postfix) op, location);
        }
        else if(auto postfix = cast(ast.Postfix) dest)
        {
            return new ast.Assignment(postfix.operand, postfix.type, location);
        }
        else
        {
            if(token == Token.Identifier || token == Token.Integer || token == Token.Hexadecimal || token == Token.Binary)
            {
                reject(op, opText, "statement", false);
            }
            else
            {
                reject("an assignment operator like '=', '++', or '--'");
            }
            skipAssignment(true);
            return null;
        }
    }

    auto parseExpression()
    {
        // expression = infix
        return parseInfix();
    }

    bool isInfixToken()
    {
        // infix_token = ...
        switch(token)
        {
            case Token.At:
            case Token.Add:
            case Token.Sub:
            case Token.Mul:
            case Token.Div:
            case Token.Mod:
            case Token.AddC:
            case Token.SubC:
            case Token.ShiftL:
            case Token.ShiftR:
            case Token.ArithShiftL:
            case Token.ArithShiftR:
            case Token.RotateL:
            case Token.RotateR:
            case Token.RotateLC:
            case Token.RotateRC:
            case Token.Or:
            case Token.And:
            case Token.Xor:
                return true;
            default:
                return false;
        }
    }

    bool isPrefixToken()
    {
        // prefix_token = ...
        switch(token)
        {
            case Token.Low:
            case Token.High:
            case Token.Swap:
            case Token.Sub:
            case Token.Not:
                return true;
            default:
                return false;
        }
    }

    bool isPostfixToken()
    {
        // postfix_token = ...
        switch(token)
        {
            case Token.Inc:
            case Token.Dec:
                return true;
            default:
                return false;
        }
    }

    ast.Expression parseInfix()
    {
        // infix = postfix (infix_token postfix)*
        auto location = scanner.getLocation();
        Infix[] types;
        ast.Expression[] operands;

        operands ~= parsePrefix(); // postfix
        while(true)
        {
            if(isInfixToken())
            {
                types ~= cast(Infix) token;
                nextToken(); // operator token
                operands ~= parsePrefix(); // postfix
            }
            else
            {
                if(operands.length > 1)
                {
                    return new ast.Infix(types, operands, location);
                }
                else
                {
                    return operands[0];
                }
            }
        }
    }

    ast.Expression parsePrefix()
    {
        // prefix = prefix_token prefix | postfix
        if(isPrefixToken())
        {
            auto location = scanner.getLocation();
            auto op = cast(Prefix) token;
            nextToken(); // operator token
            auto expr = parsePrefix(); // prefix
            return new ast.Prefix(op, expr, location);
        }
        else
        {
            return parsePostfix(); // postfix
        }
    }

    ast.Expression parsePostfix()
    {
        // postfix = term postfix_token*
        auto expr = parseTerm(); // term
        while(true)
        {
            if(isPostfixToken())
            {
                auto op = cast(Postfix) token;
                expr = new ast.Postfix(op, expr, scanner.getLocation());
                nextToken(); // operator token
            }
            else
            {
                return expr;
            }
        }
    }

    ast.Expression parseAssignableTerm()
    {
        // assignable_term = term ('@' term)?
        auto location = scanner.getLocation();
        auto expr = parseTerm();
        if(token == Token.At)
        {
            nextToken(); // '@'
            return new ast.Infix(Infix.At, expr, parseTerm(), location);
        }
        else
        {
            return expr;
        }
    }

    ast.Expression parseTerm()
    {
        // term = INTEGER
        //      | HEXADECIMAL
        //      | LPAREN expression RPAREN
        //      | LBRACKET expression RBRACKET
        //      | IDENTIFIER ('.' IDENTIFIER)*
        //      | 'pop'
        auto location = scanner.getLocation();
        switch(token)
        {
            case Token.Integer:
                return parseNumber(10);
            case Token.Hexadecimal:
                return parseNumber(16);
            case Token.Binary:
                return parseNumber(2);
            case Token.String:
                ast.Expression expr = new ast.String(text, location);
                nextToken(); // STRING
                return expr;
            case Token.LParen:
                nextToken(); // (
                ast.Expression expr = parseExpression(); // expression 
                consume(Token.RParen); // )
                return new ast.Prefix(Prefix.Grouping, expr, location);
            case Token.LBracket:
                nextToken(); // [
                ast.Expression expr = parseExpression(); // expression
                
                if(token == Token.Colon)
                {
                    nextToken(); // :
                    ast.Expression index = parseExpression(); // expression
                    expr = new ast.Infix(Infix.Colon, expr, index, location);
                }
                consume(Token.RBracket); // ]
                return new ast.Prefix(Prefix.Indirection, expr, location);
            case Token.Identifier:
                if(keyword == Keyword.Pop)
                {
                    nextToken(); // IDENTIFIER
                    return new ast.Pop(location);
                }
                return parseAttribute();
            default:
                reject("expression");
                return null;
        }
    }

    auto parseNumber(uint radix)
    {
        // number = INTEGER | HEXADECIMAL | BINARY
        auto location = scanner.getLocation();
        auto numberToken = token;
        auto numberText = text;
        nextToken(); // number

        uint value;
        try
        {
            auto t = numberText;
            // prefix?
            if(radix != 10)
            {
                t = t[2..t.length];
                // A prefix with no number following isn't valid.
                if(t.length == 0)
                {
                    error(getVerboseTokenName(numberToken, numberText) ~ " is not a valid integer literal", location);
                    return null;
                }
            }
            value = std.conv.to!uint(t, radix);
        }
        catch(std.conv.ConvOverflowException e)
        {
            value = 0x10000;
        }
        if(value > 0xFFFF)
        {
            error(getVerboseTokenName(numberToken, numberText) ~ " is outside of permitted range 0..65535.", location);
            return null;
        }
        return new ast.Number(numberToken, value, location);
    }

    auto parseAttribute()
    {
        auto location = scanner.getLocation();
        string[] pieces;
        if(checkIdentifier())
        {
            pieces ~= text;
        }
        nextToken(); // IDENTIFIER
        
        // Check if we should match ('.' IDENTIFIER)*
        while(token == Token.Dot)
        {
            nextToken(); // .
            if(token == Token.Identifier)
            {
                if(checkIdentifier())
                {
                    pieces ~= text;
                }
                nextToken(); // IDENTIFIER
            }
            else
            {
                reject("identifier after '.' in term");
                break;
            }
        }

        return new ast.Attribute(pieces, location);
    }
}
