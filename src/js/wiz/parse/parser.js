(function(globals) {    
'use strict';
var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
wiz.parse = typeof(wiz.parse) !== 'undefined' ? wiz.parse : {};

wiz.parse.Parser = function(scanner) {
    var that = {};
    that.scanner = scanner;
    that.includes = [];
    that.included = {};
    that.included[scanner.getLocation().file] = true;
    that.token = wiz.parse.Token.None;
    that.text = "";
    that.keyword = wiz.parse.Keyword.None;

    that.nextToken = function() {
        that.token = that.scanner.next();
        that.text = that.scanner.consumeLastText();
        that.keyword = wiz.parse.Keyword.None;
        if(that.token == wiz.parse.Token.Identifier) {
            that.keyword = wiz.parse.findKeyword(that.text);
        }
    };

    that.reject = function(token, text, expectation, advance) {
        expectation = expectation || null;
        advance = typeof(advance) !== null ? advance : true;
        if(expectation === null) {
            wiz.compile.error("unexpected " + wiz.parse.getVerboseTokenName(token, text), that.scanner.getLocation());
        } else {
            wiz.compile.error("expected " + expectation + ", but got " + wiz.parse.getVerboseTokenName(token, text) + " instead", that.scanner.getLocation());
        }
        if(advance) {
            that.nextToken();
        }
    };

    that.consume = function(expected) {
        if(that.token == expected) {
            that.nextToken();
            return true;
        } else {
            that.reject(that.token, that.text, wiz.parse.getSimpleTokenName(expected));
            return false;
        }     
    };

    that.checkIdentifier = function(permissibleKeywords) {
        if(that.token == wiz.parse.Token.Identifier) {
            if(that.keyword == wiz.parse.Keyword.None) {
                return true;
            }
            if(permissibleKeywords) {
                var i;
                for(i = 0; i < permissibleKeywords.length; i++) {
                    if(permissibleKeywords[i] === that.keyword) {
                        return true;
                    }
                }

                var keywordNames = [];
                for(i = 0; i < permissibleKeywords.length; i++) {
                    keywordNames.push("keyword '" + wiz.parse.getKeywordName(permissibleKeywords[i]) + "'");
                }
                keywordNames[keywordNames.length - 1] = "or " + keywordNames[keywordNames.length - 1];
                
                wiz.compile.error("expected identifier, " + keywordNames.join(", ") + ", but got " + wiz.parse.getVerboseTokenName(that.token, that.text) + " instead", that.scanner.getLocation());
                return false;
            }
        }
        wiz.compile.error("expected identifier, but got " + wiz.parse.getVerboseTokenName(that.token, that.text) + " instead", that.scanner.getLocation());
        return false;
    };
    
    that.parse = function() {
        that.nextToken();
        var program = that.parseProgram();
        wiz.compile.verify();
        return program;
    };
    
    that.handleEndOfFile = function() {
        if(that.token == wiz.parse.Token.EndOfFile) {
            if(that.includes.length === 0) {
                return true;
            } else {
                // Remove include guard.
                that.included.remove(that.scanner.getLocation().file);
                // Pop previous scanner off stack.
                var old = that.scanner;
                that.scanner = that.includes.pop();
                // Ready a new token for the scanner.
                that.nextToken();
                return false;
            }
        }
        return false;
    };

    that.parseProgram = function() {
        // program = (include | statement)* EOF
        var location = that.scanner.getLocation();
        var statements = [];
        while(true) {
            if(that.handleEndOfFile()) {
                break;
            }
            if(that.token == wiz.parse.Token.EndOfFile) {
                continue;
            }
            if(that.keyword == wiz.parse.Keyword.End || that.keyword == wiz.parse.Keyword.Else || that.keyword == wiz.parse.Keyword.ElseIf) {
                that.reject(that.token, that.text);
            }
            var statement = that.parseStatement();
            if(statement) {
                statements.push(statement);
            }
        }
        return wiz.ast.Block(null, statements, location);
    };
    
    that.parseCompound = function() {
        // compound = statement* 'end'
        var statements = [];
        while(true) {
            if(that.handleEndOfFile()) {
                that.reject(that.token, that.text, "'end'");
                return null;
            }
            if(that.keyword == wiz.parse.Keyword.End) {
                return statements;
            }
            if(that.keyword == wiz.parse.Keyword.Else || that.keyword == wiz.parse.Keyword.ElseIf) {
                that.reject(that.token, that.text, "'end'");
            }
            var statement = that.parseStatement();
            if(statement) {
                statements.push(statement);
            }
        }
    };

    that.parseConditionalPrelude = function() {
        // preconditional_compound = statement* 'is'
        var statements = [];
        while(true) {
            if(that.handleEndOfFile()) {
                that.reject(that.token, that.text, "'is'");
                return null;
            }
            if(that.keyword == wiz.parse.Keyword.Is) {
                return statements;
            }
            if(that.keyword == wiz.parse.Keyword.End || that.keyword == wiz.parse.Keyword.Else || that.keyword == wiz.parse.Keyword.ElseIf) {
                that.reject(that.token, that.text, "'is'");
            }
            var statement = that.parseStatement();
            if(statement) {
                statements.push(statement);
            }
        }
    };
    
    that.parseConditionalBlock = function() {
        // conditional_compound = statement* ('else' | 'elseif' | 'end')
        var statements = [];
        while(true) {
            if(that.handleEndOfFile()) {
                that.reject(that.token, that.text, "'end'");
                return null;
            }
            if(that.keyword == wiz.parse.Keyword.End || that.keyword == wiz.parse.Keyword.Else || that.keyword == wiz.parse.Keyword.ElseIf) {
                return statements;
            }
            var statement = that.parseStatement();
            if(statement) {
                statements.push(statement);
            }
        }
    };

    that.parseStatement = function() {
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
        switch(that.token) {
            case wiz.parse.Token.Identifier:
                switch(that.keyword) {
                    case wiz.parse.Keyword.Include:
                        that.parseInclude();
                        return null;
                    case wiz.parse.Keyword.Embed:
                        return that.parseEmbed();
                    case wiz.parse.Keyword.In:
                        return that.parseRelocation();
                    case wiz.parse.Keyword.Do: case wiz.parse.Keyword.Package:
                        return that.parseBlock();
                    case wiz.parse.Keyword.Bank:
                        return that.parseBankDecl();
                    case wiz.parse.Keyword.Def:
                        return that.parseLabelDecl();
                    case wiz.parse.Keyword.Let:
                        return that.parseLetDecl();
                    case wiz.parse.Keyword.Var:
                        return that.parseVarDecl();
                    case wiz.parse.Keyword.Func: case wiz.parse.Keyword.Task:
                        return that.parseFuncDecl();
                    case wiz.parse.Keyword.Inline:
                        return that.parseInline();
                    case wiz.parse.Keyword.Byte: case wiz.parse.Keyword.Word:
                        return that.parseData();
                    case wiz.parse.Keyword.Goto: case wiz.parse.Keyword.Call:
                    case wiz.parse.Keyword.Return: case wiz.parse.Keyword.Resume:
                    case wiz.parse.Keyword.Break: case wiz.parse.Keyword.Continue:
                    case wiz.parse.Keyword.While: case wiz.parse.Keyword.Until:
                    case wiz.parse.Keyword.Abort: case wiz.parse.Keyword.Sleep: case wiz.parse.Keyword.Suspend: case wiz.parse.Keyword.Nop:
                        return that.parseJump();
                    case wiz.parse.Keyword.If:
                        return that.parseConditional();
                    case wiz.parse.Keyword.Loop:
                        return that.parseLoop();
                    case wiz.parse.Keyword.Unroll:
                        return that.parseUnroll();
                    case wiz.parse.Keyword.Compare:
                        return that.parseComparison();
                    case wiz.parse.Keyword.Push:
                        return that.parsePush();
                    case wiz.parse.Keyword.None:
                        // Some unreserved identifier. Try and parse as a term in an assignment!
                        return that.parseAssignment();
                    default:
                        that.reject(that.token, that.text, "statement");
                        break;
                }
                break;
            case wiz.parse.Token.Integer:
            case wiz.parse.Token.Hexadecimal:
            case wiz.parse.Token.Binary:
            case wiz.parse.Token.String:
            case wiz.parse.Token.LParen:
                that.reject(that.token, that.text, "statement", false);
                that.skipAssignment(true);
                break;
            case wiz.parse.Token.Set:
                that.reject(that.token, that.text, "statement", false);
                that.skipAssignment(false);
                break;
            case wiz.parse.Token.LBracket:
                return that.parseAssignment();
            case wiz.parse.Token.Semi:
                // semi-colon, skip.
                that.nextToken();
                break;
            default:
                that.reject(that.token, that.text, "statement");
                break;
        }
        return null;
    };
        
    that.parseInclude = function() {
        // include = 'include' STRING
        that.nextToken(); // IDENTIFIER (keyword 'include')
        
        var filename = null;
        if(that.token == wiz.parse.Token.String) {
            // Don't call nextToken() here, we'll be doing that when the scanner's popped off later.
            filename = that.text;
        } else {
            that.consume(wiz.parse.Token.String);
            return;
        }
        
        // Make the filename relative to its current source.
        //filename = std.path.dirName(scanner.getLocation().file) ~ std.path.dirSeparator ~ filename;

        function isEmpty(obj) {
            for(var key in obj) {
                if(obj.hasOwnProperty(key))
                    return false;
            }
            return true;
        }

        // Make sure the start path is included in the list of included filenames.
        if(isEmpty(that.included)) {
            var cur = that.scanner.getLocation().file;
            //included[std.path.dirName(cur) ~ std.path.dirSeparator ~ cur] = true;
            that.included[cur] = true;
        }
        // Already included. Skip!
        if(that.included[filename]) {
            that.nextToken(); // STRING
            return;
        }
        
        // Push old scanner onto stack.
        that.includes.push(that.scanner);
        // Add include guard.
        that.included[filename] = true;
        
        // Open the new file.
        var file = wiz.fs.open(filename);
        if(file) {
            // Swap scanner.
            that.scanner = wiz.parse.Scanner(file, filename);
        } else {
            wiz.compile.error("could not include file '" + filename + "'", that.scanner.getLocation(), true);
        }
        // Now, ready the first token of the file.
        that.nextToken();
    };
    
    that.parseEmbed = function() {
        // embed = 'embed' STRING
        var location = that.scanner.getLocation();
        that.nextToken(); // IDENTIFIER (keyword 'embed')
        
        if(that.token == wiz.parse.Token.String) {
            var filename = that.text;
            that.nextToken(); // STRING
            
            // Make the filename relative to its current source.
            //filename = std.path.dirName(scanner.getLocation().file) ~ std.path.dirSeparator ~ filename;
            return wiz.ast.Embed(filename, location);
        } else {
            that.consume(wiz.parse.Token.String);
            return null;
        }
    };
    
    that.parseRelocation = function() {
        // relocation = 'in' IDENTIFIER (',' expression)? ':'
        var location = that.scanner.getLocation();
        var name;
        var dest;
        
        that.nextToken(); // IDENTIFIER (keyword 'in')
        if(that.checkIdentifier()) {
            name = that.text;
        }
        that.nextToken(); // IDENTIFIER
        // (, expr)?
        if(that.token == wiz.parse.Token.Comma) {
            that.nextToken(); // ,
            dest = that.parseExpression(); // expression
        }
        that.consume(wiz.parse.Token.Colon); // :
        
        return wiz.ast.Relocation(name, dest, location);
    };
    
    that.parseBlock = function() {
        // block = ('package' IDENTIFIER | 'do') statement* 'end'
        var location = that.scanner.getLocation();
        var statements;
        switch(that.keyword) {
            case wiz.parse.Keyword.Do:
                that.nextToken(); // IDENTIFIER (keyword 'do')
                statements = that.parseCompound(); // compound statement
                that.nextToken(); // IDENTIFIER (keyword 'end')
                return wiz.ast.Block(null, statements, location);
            case wiz.parse.Keyword.Package:
                var name;
                
                that.nextToken(); // IDENTIFIER (keyword 'package')
                if(that.checkIdentifier()) {
                    name = that.text;
                }
                that.nextToken(); // IDENTIFIER
                if(that.keyword == wiz.parse.Keyword.Do) {
                    that.nextToken(); // IDENTIFIER (keyword 'do')
                } else {
                    that.reject(that.token, that.text, "'do'");
                }

                statements = that.parseCompound(); // compound statement
                that.nextToken(); // IDENTIFIER (keyword 'end')
                return wiz.ast.Block(name, statements, location);
            default:
                throw new Error("unexpected compilation error: incorrectly classified token as start of block statement");
        }
    };
    
    that.parseBankDecl = function() {
        // bank = 'bank' IDENTIFIER (',' IDENTIFIER)* ':' IDENTIFIER '*' expression
        var location = that.scanner.getLocation();
        
        var names = [];
        var type = null;
        var size = null;
        
        that.nextToken(); // IDENTIFIER (keyword 'bank')
        
        if(that.checkIdentifier()) {
            names.push(that.text);
        }
        that.nextToken(); // IDENTIFIER
        
        // Check if we should match (',' id)*
        while(that.token == wiz.parse.Token.Comma) {
            that.nextToken(); // ,
            if(that.token == wiz.parse.Token.Identifier) {
                if(that.checkIdentifier()) {
                    // parse name
                    names.push(that.text);
                }
                that.nextToken(); // IDENTIFIER
            } else {
                that.reject(that.token, that.text, "identifier after ',' in bank declaration");
                break;
            }
        }
        
        that.consume(wiz.parse.Token.Colon); // :
        
        if(that.checkIdentifier()) {
            type = that.text;
        }
        that.nextToken(); // IDENTIFIER (bank type)
        that.consume(wiz.parse.Token.Mul); // *
        size = that.parseExpression(); // term
        
        return wiz.ast.BankDecl(names, type, size, location);
    };
    
    that.parseLabelDecl = function () {
        // label = 'def' IDENTIFIER ':'
        var location = that.scanner.getLocation();
        
        var name;
        
        that.nextToken(); // IDENTIFIER (keyword 'def')
        if(that.checkIdentifier()) {
            name = that.text;
        }
        that.nextToken(); // IDENTIFIER
        that.consume(wiz.parse.Token.Colon);
        
        return wiz.ast.LabelDecl(name, location);
    };

    that.parseStorage = function() {
        // storage = ('byte' | 'word') ('*' expression)?
        var location = that.scanner.getLocation();
        if(that.checkIdentifier([wiz.parse.Keyword.Byte, wiz.parse.Keyword.Word])) {
            var storageType;
            switch(that.keyword) {
                case wiz.parse.Keyword.Byte:
                case wiz.parse.Keyword.Word:
                    storageType = that.keyword;
                    break;
                default:
                    wiz.compile.error("invalid type specifier '" + that.text + "'. only 'byte' and 'word' are allowed.", scanner.getLocation());
            }
            that.nextToken(); // IDENTIFIER (keyword 'byte'/'word')
            // ('*' array_size)?
            var arraySize = null;
            if(that.token == wiz.parse.Token.Mul) {
                that.nextToken(); // *
                arraySize = that.parseExpression(); // expression
            } 
            return wiz.ast.Storage(storageType, arraySize, location);
        } else {
            that.reject(that.token, that.text, "type specifier");
            return null;
        }
    };
    
    that.parseLetDecl = function() {
        // let = 'let' IDENTIFIER '=' expression
        var location = that.scanner.getLocation();
        
        var name;
        var value;
        
        that.nextToken(); // IDENTIFIER (keyword 'let')
        if(that.checkIdentifier()) {
            name = that.text;
        }
        that.nextToken(); // IDENTIFIER
        that.consume(wiz.parse.Token.Set); // =
        value = that.parseExpression(); // expression
        return wiz.ast.LetDecl(name, value, 0, location);
    };
    
    that.parseVarDecl = function() {
        // var = 'var' IDENTIFIER (',' IDENTIFIER)*
        //      ':' ('byte' | 'word') '*' expression
        var location = that.scanner.getLocation();
        
        var names = [];
        that.nextToken(); // IDENTIFIER (keyword 'var')
        
        if(that.checkIdentifier()) {
            names.push(that.text);
        }
        that.nextToken(); // IDENTIFIER
        
        // Check if we should match (',' id)*
        while(that.token == wiz.parse.Token.Comma) {
            that.nextToken(); // ,
            if(that.token == wiz.parse.Token.Identifier) {
                if(that.checkIdentifier()) {
                    // parse name
                    names.push(that.text);
                }
                that.nextToken(); // IDENTIFIER
            } else {
                that.reject(that.token, that.text, "identifier after ',' in variable declaration");
                break;
            }
        }
        
        that.consume(wiz.parse.Token.Colon); // :
        var storage = that.parseStorage();
        return wiz.ast.VarDecl(names, storage, location);
    };

    that.parseFuncDecl = function() {
        // func = 'func' IDENTIFIER 'do' statement* 'end'
        var location = that.scanner.getLocation();

        var name;
        var type = that.keyword;

        that.nextToken(); // IDENTIFIER (keyword 'func')

        if(that.checkIdentifier()) {
            name = that.text;
        }
        that.nextToken(); // IDENTIFIER

        if(that.keyword == wiz.parse.Keyword.Do) {
            that.nextToken(); // IDENTIFIER (keyword 'do')
        } else {
            that.reject(that.token, that.text, "'do'");
        }
        var block = wiz.ast.Block(null, that.parseCompound(), location); // statement*
        that.nextToken(); // IDENTIFIER (keyword 'end')

        return wiz.ast.FuncDecl(type, name, block, location);   
    };

    that.parseInline = function() {
        // inline = 'inline' func | call
        var location = that.scanner.getLocation();

        var func;

        that.nextToken(); // IDENTIFIER (keyword 'inline')
        if(that.keyword == wiz.parse.Keyword.Func) {
            func = that.parseFuncDecl();
            func.inlined = true;
            return func;
        } else if(that.keyword == wiz.parse.Keyword.Task) {
            func = that.parseFuncDecl();
            wiz.compile.error("'inline task' is not a valid construct, try 'inline func' instead.", location);
            return null;
        } else if(that.keyword == wiz.parse.Keyword.Call) {
            var far = false;
            that.nextToken(); // IDENTIFIER (keyword 'call')
            if(that.token == wiz.parse.Token.Exclaim) {
                that.nextToken(); // '!'
                far = true;
            }
            var destination = that.parseExpression();
            var condition = null;
            if(that.token == wiz.parse.Token.Identifier && that.keyword == wiz.parse.Keyword.When) {
                that.nextToken(); // IDENTIFIER (keyword 'when')
                condition = that.parseJumpCondition("'when'");
            }
            return wiz.ast.Jump(wiz.parse.Keyword.Inline, far, destination, condition, location);
        } else {
            that.reject(that.token, that.text, "'func'");
            return null;
        }
    };
    
    that.parseData = function() {
        // data = ('byte' | 'word') data_item (',' data_item)*
        //      where data_item = expression | STRING
        var location = that.scanner.getLocation();
        var storage = that.parseStorage();
        that.consume(wiz.parse.Token.Colon); // :
        
        var items = [];

        // item (',' item)*
        while(true) {
            var expr = that.parseExpression(); // expression
            items.push(expr);
            // (',' item)*
            if(that.token == wiz.parse.Token.Comma) {
                that.nextToken(); // ,
                continue;
            }
            break;
        }
        return wiz.ast.Data(storage, items, location);
    };
    
    that.parseJump = function() {
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
        var location = scanner.getLocation();

        var type = that.keyword;
        var far = false;
        that.nextToken(); // IDENTIFIER (keyword)
        if(that.token == wiz.parse.Token.Exclaim) {
            that.nextToken(); // '!'
            far = true;
        }
        var condition;
        switch(type) {
            case wiz.parse.Keyword.Goto: case wiz.parse.Keyword.Call:
                var destination = that.parseExpression();
                condition = null;
                if(that.token == wiz.parse.Token.Identifier && that.keyword == wiz.parse.Keyword.When) {
                    that.nextToken(); // IDENTIFIER (keyword 'when')
                    condition = that.parseJumpCondition("'when'");
                }
                return wiz.ast.Jump(type, far, destination, condition, location);
            case wiz.parse.Keyword.Return:
            case wiz.parse.Keyword.Resume:
            case wiz.parse.Keyword.Break:
            case wiz.parse.Keyword.Continue:
                condition = null;
                if(that.token == wiz.parse.Token.Identifier && that.keyword == wiz.parse.Keyword.When) {
                    that.nextToken(); // IDENTIFIER (keyword 'when')
                    condition = that.parseJumpCondition("'when'");
                }
                return wiz.ast.Jump(type, far, null, condition, location);
            case wiz.parse.Keyword.While:
                return wiz.ast.Jump(type, far, null, that.parseJumpCondition("'while'"), location);
            case wiz.parse.Keyword.Until:
                return wiz.ast.Jump(type, far, null, that.parseJumpCondition("'until'"), location);
            default:
                return wiz.ast.Jump(type, far, null, null, location);
        }
    };
    
    that.parseJumpCondition = function(context) {
        // jump_condition = '~'* (IDENTIFIER | '~=' | '==' | '<' | '>' | '<=' | '>=')

        // '~'*
        var negated = false;
        while(that.token == wiz.parse.Token.Not) {
            that.nextToken(); // '~'
            negated = !negated;
            context = "'~'";
        }
        
        switch(that.token) {
            case wiz.parse.Token.Identifier:
                var attr = that.parseAttribute();
                return wiz.ast.JumpCondition(negated, attr, scanner.getLocation());
            case wiz.parse.Token.NotEqual:
            case wiz.parse.Token.Equal:
            case wiz.parse.Token.Less:
            case wiz.parse.Token.Greater:
            case wiz.parse.Token.LessEqual:
            case wiz.parse.Token.GreaterEqual:
                var type = that.token;
                that.nextToken(); // operator token
                return wiz.ast.JumpCondition(negated, type, scanner.getLocation());
            default:
                that.reject(that.token, that.text, "condition after " + context);
                return null;
        }        
    };
    
    that.parseConditional = function() {
        // condition = 'if' statement* 'is' condition 'then' statement*
        //      ('elseif' statement* 'is' condition 'then' statement*)*
        //      ('else' statement)? 'end'
        var first = null;
        var statement = null;
        var location;

        // 'if' condition 'then' statement* ('elseif' condition 'then' statement*)*
        do {
            location = that.scanner.getLocation();
            that.nextToken(); // IDENTIFIER (keyword 'if' / 'elseif')
            
            var far = false;
            if(that.token == wiz.parse.Token.Exclaim) {
                that.nextToken(); // '!'
                far = true;
            }
            
            var prelude = wiz.ast.Block(null, that.parseConditionalPrelude(), location); // statement*

            if(that.keyword == wiz.parse.Keyword.Is) {
                that.nextToken(); // IDENTIFIER (keyword 'is')
            } else {
                that.reject(that.token, that.text, "'is'");
            }

            var condition = that.parseJumpCondition("'is'"); // condition
            
            if(that.keyword == wiz.parse.Keyword.Then) {
                that.nextToken(); // IDENTIFIER (keyword 'then')
            } else {
                that.reject(that.token, that.text, "'then'");
            }
            
            var block = wiz.ast.Block(null, that.parseConditionalBlock(), location); // statement*
            
            // Construct if statement/
            var previous = statement;
            statement = wiz.ast.Conditional(condition, far, prelude, block, location);

            // If this is an 'elseif', join to previous 'if'/'elseif'.
            if(previous) {
                previous.alternative = statement;
            } else if(first === null) {
                first = statement;
            }
        } while(that.keyword == wiz.parse.Keyword.ElseIf);
        
        // ('else' statement*)? 'end' (with error recovery for an invalid trailing else/elseif placement)
        if(that.keyword == wiz.parse.Keyword.Else) {
            location = that.scanner.getLocation();
            that.nextToken(); // IDENTIFIER (keyword 'else')
            statement.alternative = wiz.ast.Block(null, that.parseConditionalBlock(), location); // statement*
        }
        switch(that.keyword) {
            case wiz.parse.Keyword.Else:
                wiz.compile.error("duplicate 'else' clause found.", that.scanner.getLocation());
                break;
            case wiz.parse.Keyword.ElseIf:
                // Seeing as we loop on elseif before an else/end, this must be an illegal use of elseif.
                wiz.compile.error("'elseif' can't appear after 'else' clause.", that.scanner.getLocation());
                break;
            default:
        }

        if(that.keyword == wiz.parse.Keyword.End) {
            that.nextToken(); // IDENTIFIER (keyword 'end')
        }
        else {
            that.reject(that.token, that.text, "'end'");
        }
        return first;
    };
    
    that.parseLoop = function() {
        // loop = 'loop' statement* 'end'
        var location = that.scanner.getLocation();
        that.nextToken(); // IDENTIFIER (keyword 'loop')
        var far = false;
        if(that.token == wiz.parse.Token.Exclaim)
        {
            that.nextToken(); // '!'
            far = true;
        }
        var block = wiz.ast.Block(null, that.parseCompound(), location); // statement*
        that.nextToken(); // IDENTIFIER (keyword 'end')
        return wiz.ast.Loop(block, far, location);
    };

    that.parseUnroll = function() {
        // unroll = 'unroll' '*' expression ':' statement
        var location = that.scanner.getLocation();
        that.nextToken(); // IDENTIFIER (keyword 'unroll')
        var repetitions = that.parseExpression();

        if(that.keyword == wiz.parse.Keyword.Do) {
            that.nextToken(); // IDENTIFIER (keyword 'do')
        } else {
            that.reject(that.token, that.text, "'do'");
        }
        var block = wiz.ast.Block(null, that.parseCompound(), location); // statement*
        that.nextToken(); // IDENTIFIER (keyword 'end')

        return wiz.ast.Unroll(repetitions, block, location);
    };

    that.parseComparison = function() {
        // comparison = 'compare' expression ('to' expression)?
        var location = that.scanner.getLocation();
        that.nextToken(); // IDENTIFIER (keyword 'compare')
        var term = that.parseExpression();
        if(that.keyword == wiz.parse.Keyword.To) {
            that.nextToken(); // IDENTIFIER (keyword 'to')
            var other = that.parseExpression();
            return wiz.ast.Comparison(term, other, location);
        } else {
            return wiz.ast.Comparison(term, location);
        }
    };

    that.parsePush = function() {
        // command = command_token expression
        var location = that.scanner.getLocation();
        var command = that.keyword;
        that.nextToken(); // IDENTIFIER (keyword)
        var argument = that.parseExpression();
        if(that.token == wiz.parse.Token.Identifier && that.keyword == wiz.parse.Keyword.Via) {
            that.nextToken(); // IDENTIFIER (keyword 'via')
            var intermediary = that.parseTerm(); // term
            return wiz.ast.Push(argument, intermediary, location);
        }
        return wiz.ast.Push(argument, null, location);
    };

    that.skipAssignment = function(leadingExpression) {
        // Some janky error recovery. Gobble an expression.
        if(leadingExpression) {
            that.parseExpression(); // expr
        }
        // If the expression is followed by an assignment, then gobble the assignment.
        if(that.token == wiz.parse.Token.Set) {
            that.nextToken(); // =
            // If the thing after the = is an expression, then gobble that too.
            switch(that.token) {
                case wiz.parse.Token.Integer:
                case wiz.parse.Token.Hexadecimal:
                case wiz.parse.Token.Binary:
                case wiz.parse.Token.String:
                case wiz.parse.Token.LParen:
                    that.parseExpression();
                    break;
                case wiz.parse.Token.Identifier:
                    if(that.keyword == wiz.parse.Keyword.None || that.keyword == wiz.parse.Keyword.Pop) {
                        that.parseExpression();
                    }
                    break;
                default:
            }
        }
    };

    that.parseAssignment = function() {
        // assignment = assignable_term ('=' expression ('via' term)? | postfix_token)
        var location = that.scanner.getLocation();
        var dest = that.parseAssignableTerm(); // term
        var op = that.token;
        var opText = that.text;
        if(that.token == wiz.parse.Token.Set) {
            that.nextToken(); // =
            var src = that.parseExpression(); // expression
            if(that.token == wiz.parse.Token.Identifier && that.keyword == wiz.parse.Keyword.Via) {
                that.nextToken(); // IDENTIFIER (keyword 'via')
                var intermediary = that.parseTerm(); // term
                return wiz.ast.Assignment(dest, intermediary, src, null, location);
            } else {
                return wiz.ast.Assignment(dest, null, src, null, location);
            }
        } else if(that.isPostfixToken()) {
            that.nextToken(); // postfix_token
            return wiz.ast.Assignment(dest, null, null, op, location);
        } else {
            if(that.token == wiz.parse.Token.Identifier || that.token == wiz.parse.Token.Integer || that.token == wiz.parse.Token.Hexadecimal || that.token == wiz.parse.Token.Binary) {
                that.reject(op, opText, "statement", false);
            } else {
                that.reject(that.token, that.text, "an assignment operator like '=', '++', or '--'");
            }
            that.skipAssignment(true);
            return null;
        }
    };

    that.parseExpression = function() {
        // expression = infix
        return that.parseInfix();
    };

    that.isInfixToken = function() {
        // infix_token = ...
        switch(that.token) {
            case wiz.parse.Token.At:
            case wiz.parse.Token.Add:
            case wiz.parse.Token.Sub:
            case wiz.parse.Token.Mul:
            case wiz.parse.Token.Div:
            case wiz.parse.Token.Mod:
            case wiz.parse.Token.AddC:
            case wiz.parse.Token.SubC:
            case wiz.parse.Token.ShiftL:
            case wiz.parse.Token.ShiftR:
            case wiz.parse.Token.ArithShiftL:
            case wiz.parse.Token.ArithShiftR:
            case wiz.parse.Token.RotateL:
            case wiz.parse.Token.RotateR:
            case wiz.parse.Token.RotateLC:
            case wiz.parse.Token.RotateRC:
            case wiz.parse.Token.Or:
            case wiz.parse.Token.And:
            case wiz.parse.Token.Xor:
                return true;
            default:
                return false;
        }
    };

    that.isPrefixToken = function() {
        // prefix_token = ...
        switch(that.token) {
            case wiz.parse.Token.Less:
            case wiz.parse.Token.Greater:
            case wiz.parse.Token.Swap:
            case wiz.parse.Token.Sub:
            case wiz.parse.Token.Not:
                return true;
            default:
                return false;
        }
    };

    that.isPostfixToken = function() {
        // postfix_token = ...
        switch(that.token) {
            case wiz.parse.Token.Inc:
            case wiz.parse.Token.Dec:
                return true;
            default:
                return false;
        }
    };

    that.parseInfix = function() {
        // infix = postfix (infix_token postfix)*
        var location = that.scanner.getLocation();
        var types = [];
        var operands = [];

        operands.push(that.parsePrefix()); // postfix
        while(true) {
            if(that.isInfixToken()) {
                types.push(that.token);
                that.nextToken(); // operator token
                operands.push(that.parsePrefix()); // postfix
            } else {
                if(operands.length > 1) {
                    return wiz.ast.Infix(types, operands, location);
                } else {
                    return operands[0];
                }
            }
        }
    };

    that.parsePrefix = function() {
        // prefix = prefix_token prefix | postfix
        if(that.isPrefixToken()) {
            var location = that.scanner.getLocation();
            var op = that.token;
            that.nextToken(); // operator token
            var expr = that.parsePrefix(); // prefix
            return wiz.ast.Prefix(op, expr, location);
        } else {
            return that.parsePostfix(); // postfix
        }
    };

    that.parsePostfix = function() {
        // postfix = term postfix_token*
        var expr = that.parseTerm(); // term
        while(true) {
            if(that.isPostfixToken()) {
                var op = that.token;
                expr = wiz.ast.Postfix(op, expr, that.scanner.getLocation());
                that.nextToken(); // operator token
            } else {
                return expr;
            }
        }
    };

    that.parseAssignableTerm = function() {
        // assignable_term = term ('@' term)?
        var location = that.scanner.getLocation();
        var expr = that.parseTerm();
        if(that.token == wiz.parse.Token.At) {
            that.nextToken(); // '@'
            return wiz.ast.Infix(wiz.parse.Infix.At, expr, that.parseTerm(), location);
        } else {
            return expr;
        }
    };

    that.parseTerm = function() {
        // term = INTEGER
        //      | HEXADECIMAL
        //      | LPAREN expression RPAREN
        //      | LBRACKET expression RBRACKET
        //      | IDENTIFIER ('.' IDENTIFIER)*
        //      | 'pop'
        var location = that.scanner.getLocation();
        var expr;
        switch(that.token) {
            case wiz.parse.Token.Integer:
                return that.parseNumber(10);
            case wiz.parse.Token.Hexadecimal:
                return that.parseNumber(16);
            case wiz.parse.Token.Binary:
                return that.parseNumber(2);
            case wiz.parse.Token.String:
                expr = wiz.ast.String(that.text, location);
                that.nextToken(); // STRING
                return expr;
            case wiz.parse.Token.LParen:
                that.nextToken(); // (
                expr = that.parseExpression(); // expression 
                that.consume(wiz.parse.Token.RParen); // )
                return wiz.ast.Prefix(wiz.parse.Prefix.Grouping, expr, location);
            case wiz.parse.Token.LBracket:
                that.nextToken(); // [
                expr = that.parseExpression(); // expression
                
                if(that.token == wiz.parse.Token.Colon)
                {
                    that.nextToken(); // :
                    var index = that.parseExpression(); // expression
                    expr = wiz.ast.Infix(wiz.parse.Infix.Colon, expr, index, location);
                }
                that.consume(wiz.parse.Token.RBracket); // ]
                return wiz.ast.Prefix(wiz.parse.Prefix.Indirection, expr, location);
            case wiz.parse.Token.Identifier:
                if(that.keyword == wiz.parse.Keyword.Pop) {
                    that.nextToken(); // IDENTIFIER
                    return wiz.ast.Pop(location);
                }
                return that.parseAttribute();
            default:
                that.reject(that.token, that.text, "expression");
                return null;
        }
    };

    that.parseNumber = function(radix) {
        // number = INTEGER | HEXADECIMAL | BINARY
        var location = that.scanner.getLocation();
        var numberToken = that.token;
        var numberText = that.text;
        that.nextToken(); // number

        var t = numberText;
        // prefix?
        if(radix != 10) {
            t = t.substring(2, t.length);
            // A prefix with no number following isn't valid.
            if(t.length === 0) {
                wiz.compile.error(wiz.parse.getVerboseTokenName(numberToken, numberText) + " is not a valid integer literal", location);
                return null;
            }
        }
        var value = parseInt(t, radix);

        if(value > 0xFFFF || value < 0) {
            wiz.compile.error(wiz.parse.getVerboseTokenName(numberToken, numberText) + " is outside of permitted range 0..65535.", location);
            return null;
        }
        return wiz.ast.Number(numberToken, value, location);
    };

    that.parseAttribute = function() {
        var location = that.scanner.getLocation();
        var pieces = [];
        if(that.checkIdentifier()) {
            pieces.push(that.text);
        }
        that.nextToken(); // IDENTIFIER
        
        // Check if we should match ('.' IDENTIFIER)*
        while(that.token == wiz.parse.Token.Dot) {
            that.nextToken(); // .
            if(that.token == wiz.parse.Token.Identifier) {
                if(that.checkIdentifier()) {
                    pieces.push(that.text);
                }
                that.nextToken(); // IDENTIFIER
            } else {
                that.reject(that.token, that.text, "identifier after '.' in term");
                break;
            }
        }

        return wiz.ast.Attribute(pieces, location);
    };

    return that;
};

})(this);
