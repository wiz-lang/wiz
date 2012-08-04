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
    }
    
    void nextToken()
    {
        token = scanner.next();
        text = scanner.getLastText();
        keyword = Keyword.NONE;
        if(token == Token.IDENTIFIER)
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
        if(token == Token.IDENTIFIER && (!allowKeywords && keyword == Keyword.NONE || allowKeywords))
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
        
        if(keyword == Keyword.NONE || std.algorithm.find(permissibleKeywords, keyword).length > 0)
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
    
    ast.Block parse()
    {
        nextToken();
        auto program = parseProgram();
        compile.verify();
        return program;
    }
    
    ast.Block parseProgram()
    {
        // program = (include | statement)* EOF
        compile.Location location = scanner.getLocation();
        ast.Statement[] statements;
        while(true)
        {
            if(token == Token.EOF)
            {
                if(includes.length == 0)
                {
                    break;
                }
                else
                {
                    // Remove include guard.
                    included.remove(scanner.getLocation().file);
                    // Pop previous scanner off stack.
                    Scanner old = scanner;
                    scanner = std.array.back(includes);
                    std.array.popBack(includes);
                    // Ready a new token for the scanner.
                    nextToken();
                }
            }
            if(keyword == Keyword.END || keyword == Keyword.ELSE || keyword == Keyword.ELSEIF)
            {
                reject();
            }
            if(keyword == Keyword.INCLUDE)
            {
                parseInclude();
            }
            
            ast.Statement statement = parseStatement();
            if(statement !is null)
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
            if(token == Token.EOF)
            {
                reject("'end'");
                return null;
            }
            if(keyword == Keyword.END)
            {
                return statements;
            }
            if(keyword == Keyword.ELSE || keyword == Keyword.ELSEIF || keyword == Keyword.INCLUDE)
            {
                reject("'end'");
            }
            
            ast.Statement statement = parseStatement();
            if(statement !is null)
            {
                statements ~= statement;
            }
        }
    }
    
    ast.Statement[] parseConditionalCompound()
    {
        // conditional_compound = statement* ('else' | 'elseif' | 'end')
        ast.Statement[] statements;
        while(true)
        {
            if(token == Token.EOF)
            {
                reject("'end'");
            }
            if(keyword == Keyword.END || keyword == Keyword.ELSE || keyword == Keyword.ELSEIF)
            {
                return statements;
            }
            if(keyword == Keyword.INCLUDE)
            {
                reject("'end'");
            }
            ast.Statement statement = parseStatement();
            if(statement !is null)
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
        //      | constant
        //      | enumeration
        //      | variable
        //      | data
        //      | jump
        //      | conditional
        //      | loop
        //      | comparison
        //      | command
        //      | assignment
        switch(token)
        {
            case Token.IDENTIFIER:
                switch(keyword)
                {
                    case Keyword.EMBED:
                        return parseEmbed();
                    case Keyword.IN:
                        return parseRelocation();
                    case Keyword.DO, Keyword.PACKAGE:
                        return parseBlock();
                    case Keyword.BANK:
                        return parseBankDecl();
                    case Keyword.DEF:
                        return parseLabelDecl();
                    case Keyword.LET:
                        return parseConstDecl();
                    case Keyword.ENUM:
                        return parseEnumDecl();
                    case Keyword.VAR:
                        return parseVarDecl();
                    case Keyword.BYTE, Keyword.WORD:
                        return parseData();
                    case Keyword.GOTO, Keyword.CALL,
                        Keyword.RETURN, Keyword.RESUME,
                        Keyword.BREAK, Keyword.CONTINUE,
                        Keyword.WHILE, Keyword.UNTIL,
                        Keyword.ABORT, Keyword.SLEEP, Keyword.SUSPEND, Keyword.NOP:
                        return parseJump();
                    case Keyword.IF:
                        return parseConditional();
                    case Keyword.REPEAT:
                        return parseLoop();
                    case Keyword.COMPARE:
                        return parseComparison();
                    case Keyword.BIT:
                    case Keyword.PUSH:
                        return parseCommand();
                    case Keyword.NONE:
                        // Some unreserved identifier. Try and parse as a term in an assignment!
                        return parseAssignment();
                    default:
                        reject("statement");
                        break;
                }
                break;
            case Token.INTEGER, Token.HEXADECIMAL, Token.BINARY, Token.STRING, Token.LPAREN:
                reject("statement", false);
                skipAssignment(true);
                break;
            case Token.SET:
                reject("statement", false);
                skipAssignment(false);
                break;
            case Token.LBRACKET:
                parseAssignment();
                break;
            case Token.SEMI:
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
        if(token == Token.STRING)
        {
            // Don't call nextToken() here, we'll be doing that when the scanner's popped off later.
            filename = text;
        }
        else
        {
            consume(Token.STRING);
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
    
    ast.Embed parseEmbed()
    {
        // embed = 'embed' STRING
        compile.Location location = scanner.getLocation();
        nextToken(); // IDENTIFIER (keyword 'embed')
        
        if(token == Token.STRING)
        {
            string filename = text;
            nextToken(); // STRING
            
            // Make the filename relative to its current source.
            filename = std.path.dirName(scanner.getLocation().file) ~ std.path.dirSeparator ~ filename;   
            return new ast.Embed(filename, location);
        }
        else
        {
            consume(Token.STRING);
            return null;
        }
    }
    
    ast.Relocation parseRelocation()
    {
        // relocation = 'in' IDENTIFIER (',' expression)? ':'
        compile.Location location = scanner.getLocation();
        string name;
        ast.Expression dest;
        
        nextToken(); // IDENTIFIER (keyword 'in')
        if(checkIdentifier())
        {
            name = text;
        }
        nextToken(); // IDENTIFIER
        // (, expr)?
        if(token == Token.COMMA)
        {
            nextToken(); // ,
            dest = parseExpression(); // expression
        }
        consume(Token.COLON); // :
        
        return new ast.Relocation(name, dest, location);
    }
    
    ast.Block parseBlock()
    {
        // block = ('package' IDENTIFIER | 'do') statement* 'end'
        compile.Location location = scanner.getLocation();
        switch(keyword)
        {
            case Keyword.DO:
                nextToken(); // IDENTIFIER (keyword 'do')
                ast.Statement[] statements = parseCompound(); // compound statement
                nextToken(); // IDENTIFIER (keyword 'end')
                return new ast.Block(statements, location);
            case Keyword.PACKAGE:
                string name;
                
                nextToken(); // IDENTIFIER (keyword 'package')
                if(checkIdentifier())
                {
                    name = text;
                }
                nextToken(); // IDENTIFIER
                ast.Statement[] statements = parseCompound(); // compound statement
                nextToken(); // IDENTIFIER (keyword 'end')
                return new ast.Block(name, statements, location);
            default:
                error("unexpected compilation error: incorrectly classified token as start of block statement", scanner.getLocation());
                assert(false);
        }
    }
    
    ast.BankDecl parseBankDecl()
    {
        // bank = 'bank' IDENTIFIER (',' IDENTIFIER)* ':' IDENTIFIER '*' expression
        compile.Location location = scanner.getLocation();
        
        string[] names;
        string type;
        ast.Expression size;
        
        nextToken(); // IDENTIFIER (keyword 'bank')
        
        if(checkIdentifier())
        {
            names ~= text;
        }
        nextToken(); // IDENTIFIER
        
        // Check if we should match (',' id)*
        while(token == Token.COMMA)
        {
            nextToken(); // ,
            if(token == Token.IDENTIFIER)
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
                reject("identifier after ',' in bank Defaration");
                break;
            }
        }
        
        consume(Token.COLON); // :
        
        if(checkIdentifier())
        {
            type = text;
        }
        nextToken(); // IDENTIFIER (bank type)
        consume(Token.MUL); // *
        size = parseExpression(); // term
        
        return new ast.BankDecl(names, type, size, location);
    }
    
    ast.LabelDecl parseLabelDecl()
    {
        // label = 'def' IDENTIFIER ':'
        compile.Location location = scanner.getLocation();
        
        string name;
        
        nextToken(); // IDENTIFIER (keyword 'def')
        if(checkIdentifier())
        {
            name = text;
        }
        nextToken(); // IDENTIFIER
        consume(Token.COLON);
        
        return new ast.LabelDecl(name, location);
    }

    ast.Storage parseStorage()
    {
        // storage = ('byte' | 'word') ('*' expression)?
        compile.Location location = scanner.getLocation();
        if(checkIdentifier(true))
        {
            Keyword storageType;
            switch(keyword)
            {
                case Keyword.BYTE:
                case Keyword.WORD:
                    storageType = keyword;
                    break;
                default:
                    error("invalid type specifier '" ~ text ~ "'. only 'byte' and 'word' are allowed.", scanner.getLocation());
            }
            nextToken(); // IDENTIFIER (keyword 'byte'/'word')
            // ('*' array_size)?
            ast.Expression arraySize;
            if(token == Token.MUL)
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
    
    ast.ConstDecl parseConstDecl()
    {
        // constant = 'let' IDENTIFIER '=' expression
        compile.Location location = scanner.getLocation();
        
        string name;
        ast.Storage storage;
        ast.Expression value;
        
        nextToken(); // IDENTIFIER (keyword 'let')
        if(checkIdentifier())
        {
            name = text;
        }
        nextToken(); // IDENTIFIER
        consume(Token.SET); // =
        value = parseExpression(); // expression
        return new ast.ConstDecl(name, value, location);
    }
    
    ast.EnumDecl parseEnumDecl()
    {
        // enumeration = 'enum' ':' enum_item (',' enum_item)*
        //      where enum_item = IDENTIFIER ('=' expression)?
        compile.Location enumLocation = scanner.getLocation();
        compile.Location constantLocation = enumLocation;
        string name;
        ast.Expression value;
        uint offset;
        ast.ConstDecl[] constants;
        
        nextToken(); // IDENTIFIER (keyword 'enum')
        consume(Token.COLON); // :
        
        if(checkIdentifier())
        {
            name = text;
        }
        constantLocation = scanner.getLocation();
        nextToken(); // IDENTIFIER
        // ('=' expr)?
        if(token == Token.SET)
        {
            consume(Token.SET); // =
            value = parseExpression();
        }
        else
        {
            value = new ast.Number(Token.INTEGER, 0, constantLocation);
        }

        constants ~= new ast.ConstDecl(name, value, offset, constantLocation);
        offset++;
        
        // (',' name ('=' expr)?)*
        while(token == Token.COMMA)
        {
            nextToken(); // ,
            if(token == Token.IDENTIFIER)
            {
                if(checkIdentifier())
                {
                    name = text;
                }
                constantLocation = scanner.getLocation();
                nextToken(); // IDENTIFIER
                // ('=' expr)?
                if(token == Token.SET)
                {
                    consume(Token.SET); // =
                    value = parseExpression();
                    offset = 0; // If we explicitly set a value, then we reset the enum expression offset.
                }
                
                constants ~= new ast.ConstDecl(name, value, offset, constantLocation);
                offset++;
            }
            else
            {
                reject("identifier after ',' in enum Defaration");
                break;
            }
        }
        
        return new ast.EnumDecl(constants, enumLocation);
    }
    
    ast.VarDecl parseVarDecl()
    {
        // variable = 'var' IDENTIFIER (',' IDENTIFIER)*
        //      ':' ('byte' | 'word') '*' expression
        compile.Location location = scanner.getLocation();
        
        string[] names;
        nextToken(); // IDENTIFIER (keyword 'var')
        
        if(checkIdentifier())
        {
            names ~= text;
        }
        nextToken(); // IDENTIFIER
        
        // Check if we should match (',' id)*
        while(token == Token.COMMA)
        {
            nextToken(); // ,
            if(token == Token.IDENTIFIER)
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
                reject("identifier after ',' in variable Defaration");
                break;
            }
        }
        
        consume(Token.COLON); // :
        ast.Storage storage = parseStorage();
        return new ast.VarDecl(names, storage, location);
    }
    
    ast.Data parseData()
    {
        // data = ('byte' | 'word') data_item (',' data_item)*
        //      where data_item = expression | STRING
        compile.Location location = scanner.getLocation();
        ast.Storage storage = parseStorage();
        consume(Token.COLON); // :
        
        ast.DataItem[] items;

        // item (',' item)*
        while(true)
        {
            ast.Expression expr = parseExpression(); // expression
            items ~= new ast.DataItem(expr, expr.location);
            // (',' item)*
            if(token == Token.COMMA)
            {
                nextToken(); // ,
                continue;
            }
            break;
        }
        return new ast.Data(storage, items, location);
    }
    
    ast.Jump parseJump()
    {
        // jump = 'goto' expression ('when' jump_condition)?
        //      | 'call' expression ('when' jump_condition)?
        //      | 'return' ('when' jump_condition)?
        //      | 'resume' ('when' jump_condition)?
        //      | 'break' ('when' jump_condition)?
        //      | 'continue' ('when' jump_condition)?
        //      | 'while' jump_condition
        //      | 'until' jump_condition
        //      | 'abort'
        //      | 'sleep'
        //      | 'suspend'
        //      | 'nop'
        compile.Location location = scanner.getLocation();

        Keyword type = keyword;
        nextToken(); // IDENTIFIER (keyword)
        switch(type)
        {
            case Keyword.GOTO, Keyword.CALL:
                ast.Expression destination = parseExpression();
                ast.JumpCondition condition = null;
                if(token == Token.IDENTIFIER && keyword == Keyword.WHEN)
                {
                    nextToken(); // IDENTIFIER (keyword 'when')
                    condition = parseJumpCondition("'when'");
                }
                return new ast.Jump(type, destination, condition, location);
                break;
            case Keyword.RETURN, Keyword.RESUME, Keyword.BREAK, Keyword.CONTINUE:
                ast.JumpCondition condition = null;
                if(token == Token.IDENTIFIER && keyword == Keyword.WHEN)
                {
                    nextToken(); // IDENTIFIER (keyword 'when')
                    condition = parseJumpCondition("'when'");
                }
                return new ast.Jump(type, condition, location);
            case Keyword.WHILE:
                return new ast.Jump(type, parseJumpCondition("'while'"), location);
            case Keyword.UNTIL:
                return new ast.Jump(type, parseJumpCondition("'until'"), location);
            default:
                return new ast.Jump(type, location);
        }
    }
    
    ast.JumpCondition parseJumpCondition(string context)
    {
        // jump_condition = 'not'* (IDENTIFIER | '!=' | '==' | '<' | '>=')
        ast.JumpCondition condition = null;
        
        // 'not'* (not isn't a keyword, but it has special meaning)
        bool negated = false;
        while(keyword == Keyword.NOT)
        {
            nextToken(); // IDENTIFIER (keyword 'not')
            negated = !negated;
            context = "'not'";
        }
        
        string flag = null;
        switch(token)
        {
            case Token.IDENTIFIER:
                checkIdentifier();
                flag = text;
                nextToken(); // condition
                break;
            case Token.NE:
                negated = !negated;
                flag = "zero";
                nextToken(); // !=
                break;
            case Token.EQ:
                flag = "zero";
                nextToken(); // ==
                break;
            case Token.LT:
                negated = !negated;
                flag = "carry";
                nextToken(); // <
                break;
            case Token.GE:
                flag = "carry";
                nextToken(); // >=
                break;
            default:
                reject("flag name after " ~ context);
                return null;
        }        
        return new ast.JumpCondition(negated, flag, scanner.getLocation());
    }
    
    ast.Conditional parseConditional()
    {
        // condition = 'if' condition 'then' statement*
        //      ('elseif' condition 'then' statement*)*
        //      ('else' statement)? 'end'
        ast.Conditional first = null;
        ast.Conditional statement = null;
        
        // 'if' condition 'then' statement* ('elseif' condition 'then' statement*)*
        do
        {
            compile.Location location = scanner.getLocation();
            nextToken(); // IDENTIFIER (keyword 'if' / 'elseif')
            
            ast.JumpCondition condition = parseJumpCondition("'if'");
            
            if(keyword == Keyword.THEN)
            {
                nextToken(); // IDENTIFIER (keyword 'then')
            }
            else
            {
                reject("'then'");
            }
            
            // statement*
            ast.Block block = new ast.Block(parseConditionalCompound(), location);
            
            // Construct if statement, which is either static or runtime depending on argument before 'then'.
            ast.Conditional previous = statement;
            statement = new ast.Conditional(condition, block, location);

            // If this is an 'elseif', join to previous 'if'/'elseif'.
            if(previous !is null)
            {
                previous.alternative = statement;
            }
            else if(first is null)
            {
                first = statement;
            }
        } while(keyword == Keyword.ELSEIF);
        
        // ('else' statement*)? 'end' (with error recovery for an invalid trailing else/elseif placement)
        if(keyword == Keyword.ELSE)
        {
            compile.Location location = scanner.getLocation();
            nextToken(); // IDENTIFIER (keyword 'else')
            statement.alternative = new ast.Block(parseConditionalCompound(), location); // statement*
        }
        switch(keyword)
        {
            case Keyword.ELSE:
                error("duplicate 'else' clause found.", scanner.getLocation());
                break;
            case Keyword.ELSEIF:
                // Seeing as we loop on elseif before an else/end, this must be an illegal use of elseif.
                error("'elseif' can't appear after 'else' clause.", scanner.getLocation());
                break;
            default:
        }

        if(keyword == Keyword.END)
        {
            nextToken(); // IDENTIFIER (keyword 'end')
        }
        else
        {
            reject("'end'");
        }
        return first;
    }
    
    ast.Loop parseLoop()
    {
        // loop = 'repeat' statement* 'end'
        compile.Location location = scanner.getLocation();
        nextToken(); // IDENTIFIER (keyword 'repeat')
        ast.Block block = new ast.Block(parseCompound(), location); // statement*
        nextToken(); // IDENTIFIER (keyword 'end')
        return new ast.Loop(block, location);
    }

    ast.Comparison parseComparison()
    {
        // comparison = 'cmp' expression ('to' expression)?
        compile.Location location = scanner.getLocation();
        nextToken(); // IDENTIFIER (keyword 'cmp')
        ast.Expression term = parseAssignableTerm();
        if(keyword == Keyword.TO)
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

    ast.Command parseCommand()
    {
        // command = command_token expression
        compile.Location location = scanner.getLocation();
        Keyword command = keyword;
        nextToken(); // IDENTIFIER (keyword)
        ast.Expression argument = parseExpression();
        return new ast.Command(command, argument, location);
    }

    void skipAssignment(bool leadingExpression)
    {
        // Some janky error recovery. Gobble an expression.
        if(leadingExpression)
        {
            parseExpression(); // expr
        }
        // If the expression is followed by an assignment, then gobble the assignment.
        if(token == Token.SET)
        {
            nextToken(); // =
            // If the thing after the = is an expression, then gobble that too.
            switch(token)
            {
                case Token.INTEGER, Token.HEXADECIMAL, Token.BINARY, Token.STRING, Token.LPAREN:
                    parseExpression();
                    break;
                case Token.IDENTIFIER:
                    if(keyword == Keyword.NONE || keyword == Keyword.POP)
                    {
                        parseExpression();
                    }
                    break;
                default:
            }
        }
    }

    ast.Assignment parseAssignment()
    {
        // assignment = assignable_term ('=' expression ('via' term)? | postfix_token)
        compile.Location location = scanner.getLocation();
        Token lead = token;
        string leadText = text;
        ast.Expression dest = parseAssignableTerm(); // term
        Token op = token;
        if(token == Token.SET)
        {
            nextToken(); // =
            ast.Expression src = parseExpression(); // expression
            if(token == Token.IDENTIFIER && keyword == Keyword.VIA)
            {
                nextToken(); // IDENTIFIER (keyword 'via')
                ast.Expression intermediary = parseTerm(); // term
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
            return new ast.Assignment(dest, op, location);
        }
        else
        {
            if(token == Token.IDENTIFIER || token == Token.INTEGER || token == Token.HEXADECIMAL || token == Token.BINARY)
            {
                reject(lead, leadText, "statement", false);
            }
            else
            {
                reject("an assignment operator like '=', '++', '--' or '<>'");
            }
            skipAssignment(true);
            return null;
        }
    }

    ast.Expression parseExpression()
    {
        // expression = infix
        return parseInfix();
    }

    bool isInfixToken()
    {
        // infix_token = ...
        switch(token)
        {
            case Token.AT:
            case Token.ADD:
            case Token.SUB:
            case Token.MUL:
            case Token.DIV:
            case Token.MOD:
            case Token.ADDC:
            case Token.SUBC:
            case Token.SHL:
            case Token.SHR:
            case Token.ASHL:
            case Token.ASHR:
            case Token.ROL:
            case Token.ROR:
            case Token.ROLC:
            case Token.RORC:
            case Token.OR:
            case Token.AND:
            case Token.XOR:
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
            case Token.LT:
            case Token.GT:
            case Token.SWAP:
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
            case Token.INC:
            case Token.DEC:
                return true;
            default:
                return false;
        }
    }

    ast.Expression parseInfix()
    {
        // infix = postfix (infix_token postfix)*
        ast.Expression left = parsePrefix(); // postfix
        while(true)
        {
            compile.Location location = scanner.getLocation();
            if(isInfixToken())
            {
                Token op = token;
                nextToken(); // operator token
                ast.Expression right = parsePrefix(); // postfix
                left = new ast.Infix(op, left, right, location);
            }
            else
            {
                return left;
            }
        }
    }

    ast.Expression parsePrefix()
    {
        // prefix = prefix_token prefix | postfix
        if(isPrefixToken())
        {
            compile.Location location = scanner.getLocation();
            Token op = token;
            nextToken(); // operator token
            ast.Expression expr = parsePrefix(); // prefix
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
        ast.Expression expr = parseTerm(); // term
        while(true)
        {
            if(isPostfixToken())
            {
                Token op = token;
                expr = new ast.Postfix(op, expr, scanner.getLocation());
                nextToken(); // operator token
            }
            else
            {
                return expr;
            }
        }
    }

    ast.Expression parseNumber(uint radix)
    {
        // number = INTEGER | HEXADECIMAL | BINARY
        compile.Location location = scanner.getLocation();
        Token numberToken = token;
        string numberText = text;
        nextToken(); // number

        uint value;
        try
        {
            string t = numberText;
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

    ast.Expression parseAssignableTerm()
    {
        // assignable_term = term ('@' term)?
        compile.Location location = scanner.getLocation();
        ast.Expression expr = parseTerm();
        if(token == Token.AT)
        {
            nextToken(); // '@'
            return new ast.Infix(Token.AT, expr, parseTerm(), location);
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
        compile.Location location = scanner.getLocation();
        switch(token)
        {
            case Token.INTEGER:
                return parseNumber(10);
            case Token.HEXADECIMAL:
                return parseNumber(16);
            case Token.BINARY:
                return parseNumber(2);
            case Token.STRING:
                ast.Expression expr = new ast.String(text, location);
                nextToken(); // STRING
                return expr;
            case Token.LPAREN:
                nextToken(); // (
                ast.Expression expr = parseExpression(); // expression 
                consume(Token.RPAREN); // )
                return new ast.Prefix(Token.LPAREN, expr, location);
            case Token.LBRACKET:
                nextToken(); // [
                ast.Expression expr = parseExpression(); // expression
                consume(Token.RBRACKET); // ]
                return new ast.Prefix(Token.LBRACKET, expr, location);
            case Token.IDENTIFIER:
                if(keyword == Keyword.POP)
                {
                    nextToken(); // IDENTIFIER
                    return new ast.Pop(location);
                }

                string[] pieces;
                if(checkIdentifier())
                {
                    pieces ~= text;
                }
                nextToken(); // IDENTIFIER
                
                // Check if we should match ('.' IDENTIFIER)*
                while(token == Token.DOT)
                {
                    nextToken(); // .
                    if(token == Token.IDENTIFIER)
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
            default:
                reject("expression");
                return null;
        }
    }
}
