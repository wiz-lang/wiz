#include <algorithm>
#include <stdexcept>
#include <utility>
#include <cinttypes>
#include <cstdlib>
#include <cassert>

#include <wiz/ast/expression.h>
#include <wiz/ast/statement.h>
#include <wiz/ast/type_expression.h>
#include <wiz/parser/parser.h>
#include <wiz/parser/scanner.h>
#include <wiz/utility/path.h>
#include <wiz/utility/text.h>
#include <wiz/utility/reader.h>
#include <wiz/utility/import_manager.h>
#include <wiz/utility/source_location.h>

namespace wiz {
    Parser::Parser(
        StringPool* stringPool,
        ImportManager* importManager,
        Report* report)
    : stringPool(stringPool), 
    importManager(importManager), 
    report(report),
    token(TokenType::None),
    symbolIndex(0) {}

    Parser::~Parser() {}    

    void Parser::nextToken() {
        if (lookaheadBuffer.size() > 0) {
            token = lookaheadBuffer.back();
            lookaheadBuffer.pop_back();
        } else {
            token = scanner->next();
        }
    }

    void Parser::peek(std::size_t count) {
        for (std::size_t i = 0; i != count; ++i) {
            lookaheadBuffer.push_back(scanner->next());
        }
    }

    void Parser::reject(Token token, StringView expectation, bool advance, ReportErrorFlags flags) {
        if (expectation.getLength() == 0) {
            report->error("unexpected " + getVerboseTokenName(token), scanner->getLocation());
        } else {
            report->error("expected " + expectation.toString() + ", but got " + getVerboseTokenName(token) + " instead", scanner->getLocation(), flags);
        }
        if (advance) {
            nextToken();
        }
    }

    bool Parser::expectTokenType(TokenType expected) {
        if (token.type == expected) {
            nextToken();
            return true;
        } else {
            reject(token, getSimpleTokenName(expected), true);
            return false;
        }     
    }

    bool Parser::expectStatementEnd(StringView description) {
        if (token.type != TokenType::Semicolon) {
            reject(token, StringView(getSimpleTokenName(TokenType::Semicolon).toString() + " after previous " + description.toString()), false);
            skipToNextStatement();
            return false;
        } else {
            nextToken(); // `;`
            return true;
        }
    }    
    
    void Parser::skipToNextStatement() {
        // Recover to the next statement/block.
        while (report->alive()) {
            switch (token.type) {
                case TokenType::LeftBrace:
                case TokenType::RightBrace:
                case TokenType::Semicolon:
                case TokenType::EndOfFile:
                    return;
                case TokenType::Identifier:
                    switch (token.keyword) {
                        case Keyword::If:
                        case Keyword::Do:
                        case Keyword::While:
                        case Keyword::For:
                        case Keyword::Var:
                        case Keyword::Const:
                        case Keyword::WriteOnly:
                        case Keyword::Let:
                        case Keyword::Enum:
                        case Keyword::Struct:
                        case Keyword::Func:
                        case Keyword::Bank:
                        case Keyword::In:
                        case Keyword::Import:
                        case Keyword::Goto:
                        case Keyword::IrqReturn:
                        case Keyword::NmiReturn:
                        case Keyword::Return:
                        case Keyword::Break:
                        case Keyword::Continue:
                        case Keyword::Inline:
                        case Keyword::Namespace:
                            return;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }

            // Advance token only if it can't be the start of the next statement.
            nextToken();
        }
    }

    bool Parser::checkIdentifier() {
        if (token.type == TokenType::Identifier && token.keyword == Keyword::None) {
            return true; 
        } else {
            report->error("expected identifier but got " + getVerboseTokenName(token) + " instead", scanner->getLocation());
            return false;
        }
    }

    ImportResult Parser::importModule(StringView originalPath, ImportOptions importOptions, StringView& displayPath, StringView& canonicalPath, std::unique_ptr<Reader>& reader) {
        if (scanner == nullptr) {
            const auto result = importManager->attemptAbsoluteImport(originalPath, originalPath, importOptions, displayPath, canonicalPath, reader);
            if (result != ImportResult::Failed) {
                return result;
            }
        } else {
            const auto result = importManager->importModule(originalPath, importOptions, displayPath, canonicalPath, reader);        
            if (result != ImportResult::Failed) {
                return result;
            }
        }

        report->error("could not open module \"" + text::escape(originalPath, '\"') + "\"",
            scanner != nullptr ? scanner->getLocation() : SourceLocation(StringView(), 0));
        return ImportResult::Failed;
    }

    void Parser::pushScanner(StringView displayPath, StringView canonicalPath, std::unique_ptr<Reader> reader) {
        scannerStack.push_back(std::move(scanner));
        scanner = std::make_unique<Scanner>(std::move(reader), displayPath, canonicalPath, stringPool, report);
        // Now, prepare the first token of the next file.
        nextToken();

        importManager->setCurrentPath(scanner->getLocation().canonicalPath);
    }

    void Parser::popScanner() {
        if (scannerStack.size() > 1) {
            // Pop previous scanner off stack.
            scanner = std::move(scannerStack.back());
            scannerStack.pop_back();

            importManager->setCurrentPath(scanner->getLocation().canonicalPath);
        }
    }

    FwdUniquePtr<const Statement> Parser::parse(StringView path) {
        StringView displayPath;
        StringView canonicalPath;
        std::unique_ptr<Reader> reader;

        if (importModule(path, ImportOptions { ImportOptionType::AllowShellResources }, displayPath, canonicalPath, reader) != ImportResult::Failed) {
            pushScanner(displayPath, canonicalPath, std::move(reader));
            importManager->setCurrentPath(scanner->getLocation().canonicalPath);
            importManager->setStartPath(importManager->getCurrentPath());

            FwdUniquePtr<const Statement> file(parseFile(displayPath, canonicalPath, SourceLocation(stringPool->intern("<commandline>"))));
            if (report->validate()) {
                return file;
            }
        }

        report->validate();
        return nullptr;
    }   

    FwdUniquePtr<const Statement> Parser::parseFile(StringView originalPath, StringView canonicalPath, SourceLocation importLocation) {
        // main_block = (include | statement)* EOF
        std::vector<FwdUniquePtr<const Statement>> statements;
        while (report->alive()) {
            if (token.type == TokenType::EndOfFile) {
                popScanner();
                break;
            }
            if (token.type == TokenType::RightBrace) {
                reject(token, StringView(), true);
            }
            if (auto statement = parseStatement()) {
                statements.push_back(std::move(statement));
            }
        }
        return makeFwdUnique<const Statement>(Statement::File(std::move(statements), originalPath, canonicalPath, stringPool->intern("file \"" + originalPath.toString() + "\"")), importLocation);
    }  

    FwdUniquePtr<const Statement> Parser::parseStatement() {
        // statement_inner =
        //      | directive
        //      | include
        //      | relocation_statement
        //      | namespace_declaration
        //      | bank_declaration
        //      | let_declaration
        //      | var_declaration
        //      | func_declaration
        //      | inline_declaration
        //      | const_declaration
        //      | branch_statement
        //      | if_statement
        //      | while_statement
        //      | do_while_statement
        //      | assignment_statement
        //      | expression_statement
        //      | block_statement
        switch (token.type) {
            case TokenType::Identifier:
                switch (token.keyword) {
                    case Keyword::Import: return parseImport();
                    case Keyword::In: return parseInStatement();
                    case Keyword::Namespace: return parseNamespaceDeclaration();
                    case Keyword::Bank: return parseBankDeclaration();
                    case Keyword::Let: return parseLetDeclaration();
                    case Keyword::Enum: return parseEnumDeclaration();
                    case Keyword::Struct: return parseStructDeclaration(StructKind::Struct);
                    case Keyword::Union: return parseStructDeclaration(StructKind::Union);
                    case Keyword::Var: return parseVarDeclaration(Modifiers {});
                    case Keyword::Const: return parseVarDeclaration(Modifiers { Modifier::Const });
                    case Keyword::WriteOnly: return parseVarDeclaration(Modifiers { Modifier::WriteOnly });
                    case Keyword::Extern: {
                        nextToken(); // IDENTIFIER (keyword `extern`)

                        switch (token.keyword) {
                            case Keyword::Var: return parseVarDeclaration(Modifiers { Modifier::Extern });
                            case Keyword::Const: return parseVarDeclaration(Modifiers { Modifier::Extern, Modifier::Const });
                            case Keyword::WriteOnly: return parseVarDeclaration(Modifiers { Modifier::Extern, Modifier::WriteOnly });
                            default: {
                                reject(token, "declaration after `extern`"_sv, false);
                                skipToNextStatement();
                                break;
                            }
                        }
                        break;
                    }
                    case Keyword::TypeAlias: return parseTypeAliasDeclaration();
                    case Keyword::Func: return parseFuncDeclaration(false, false);
                    case Keyword::Inline: return parseInlineStatement();
                    case Keyword::Goto:
                    case Keyword::IrqReturn:
                    case Keyword::NmiReturn:
                    case Keyword::Return:
                    case Keyword::Break:
                    case Keyword::Continue: {
                        return parseBranchStatement(0, false);
                    }
                    case Keyword::If: return parseIfStatement(0);
                    case Keyword::While: return parseWhileStatement(0);
                    case Keyword::Do: return parseDoWhile(0);
                    case Keyword::For: return parseForStatement(0);               
                    case Keyword::Far: return parseFarStatement();               
                    case Keyword::None: {
                        // label = IDENTIFIER `:`
                        // Without a token of lookahead, this would be ambiguous and require a leading keyword in the grammar.
                        peek(1);
                        if (lookaheadBuffer.size() > 0) {
                            const auto& lookahead = lookaheadBuffer.back();
                            if (lookahead.type == TokenType::Colon) {
                                const auto location = scanner->getLocation();
                                StringView name = token.text;

                                nextToken(); // IDENTIFIER
                                expectTokenType(TokenType::Colon);
                                
                                return makeFwdUnique<const Statement>(Statement::Label(false, name), location);
                            } else if (lookahead.type == TokenType::LeftBrace && token.text == "config"_sv) {
                                return parseConfigDirective();
                            }
                        }
                        // Some unreserved identifier. Try and parse as a term in an assignment!
                        return parseExpressionStatement();
                    }
                    default: {
                        reject(token, "statement"_sv, false);
                        skipToNextStatement();
                        break;
                    }
                }
                break;
            case TokenType::HashBracket: return parseAttribute();
            case TokenType::Caret: return parseDistanceHintedStatement();
            case TokenType::DoublePlus:
            case TokenType::DoubleMinus:
            case TokenType::Asterisk:
            case TokenType::LessColon:
            case TokenType::GreaterColon:
            case TokenType::LeftParenthesis: {
                return parseExpressionStatement();
            }
            case TokenType::LeftBrace: return parseBlockStatement();
            case TokenType::Semicolon: {
                // Empty statement, skip.
                nextToken(); // `;`
                break;
            }
            default: {
                reject(token, "statement"_sv, false);
                skipToNextStatement();
                break;
            }
        }
        return nullptr;
    }
        
    FwdUniquePtr<const Statement> Parser::parseImport() {
        // include = `include` STRING
        const auto location = scanner->getLocation();
        nextToken(); // IDENTIFIER (keyword `include`)
                
        if (token.type == TokenType::String) {
            const auto originalPath = token.text;

            StringView displayPath;
            StringView canonicalPath;
            std::unique_ptr<Reader> reader;

            FwdUniquePtr<const Statement> statement;
            const auto result = importModule(originalPath, ImportOptions { ImportOptionType::AppendExtension }, displayPath, canonicalPath, reader);
            switch (result) {           
                case ImportResult::JustImported: {
                    pushScanner(displayPath, canonicalPath, std::move(reader));
                    statement = parseFile(originalPath, canonicalPath, location);
                    break;
                }
                case ImportResult::AlreadyImported: {
                    statement = makeFwdUnique<const Statement>(Statement::ImportReference(originalPath, canonicalPath, stringPool->intern("`import \"" + originalPath.toString() + "\";`")), location);                    
                    break;
                }
                default:
                case ImportResult::Failed: {
                    statement = nullptr;
                    break;
                }
            }

            nextToken(); // STRING
            expectStatementEnd("`import` statement"_sv);
            return statement;
        } else {
            reject(token, "path string"_sv, false);
            skipToNextStatement();
        }
        return nullptr;
    }

  FwdUniquePtr<const Statement> Parser::parseAttribute() {
        // attributed_statement = `#[` attribute (`,` attribute)* `]` statement
        // attribute = IDENTIFIER (`(` (expression (`,` expression)*)? `)`)?
        std::vector<std::unique_ptr<const Statement::Attribution::Attribute>> attributes;
        if (token.type == TokenType::HashBracket) {
            bool err = false;
            bool done = false;

            nextToken(); // `[`

            // attribute (`,` attribute)*
            while (!done) {
                std::vector<FwdUniquePtr<const Expression>> arguments;
                const auto location = scanner->getLocation();

                if (token.type == TokenType::RightBracket) {
                    done = true;
                } else if (token.type == TokenType::Comma) {
                    reject(token, "attribute"_sv, true);
                    err = true;
                    done = true;
                } else {
                    StringView name;
                    if (token.type == TokenType::Identifier) {
                        if (checkIdentifier()) {
                            name = token.text;
                            nextToken(); // IDENTIFIER
                        }
                        if (token.type == TokenType::LeftParenthesis) {
                            nextToken(); // `(`
                            if (token.type != TokenType::RightParenthesis) {
                                // expression (`,` expression)*
                                while (report->alive()) {
                                    auto expr = parseExpression(); // expression
                                    arguments.push_back(std::move(expr));

                                    // (`,` expression)*
                                    if (token.type == TokenType::Comma) {
                                        nextToken(); // `,`
                                        if (token.type != TokenType::RightParenthesis) {
                                            continue;
                                        }
                                    }

                                    break;
                                }
                            }
                            // `)`
                            if (!expectTokenType(TokenType::RightParenthesis)) {
                                skipToNextStatement();
                                return nullptr;
                            }
                        }

                        attributes.push_back(std::make_unique<const Statement::Attribution::Attribute>(name, std::move(arguments), location));
                    } else {
                        reject(token, "attribute name"_sv, true);
                        err = true;
                        done = true;
                    }
                }

                if (!done) {
                    // (`,` attribute)*
                    if (token.type == TokenType::Comma) {
                        nextToken(); // `,`
                    } else if (token.type == TokenType::RightBracket) {
                        done = true;
                    } else {
                        reject(token, "`,` or `]`"_sv, true);
                        err = true;
                        done = true;
                    }
                }
            }

            if (err) {
                while (token.type != TokenType::RightBracket && token.type != TokenType::EndOfFile) {
                    nextToken(); // !(`[` | eof)*
                }
                nextToken(); // (`]` | eof)
                skipToNextStatement();

                return nullptr;
            } else {
                expectTokenType(TokenType::RightBracket); // `]`
            }
        }

        auto statement = parseStatement();
        if (statement != nullptr && attributes.size() > 0) {
            const auto location = statement->location;
            return makeFwdUnique<const Statement>(Statement::Attribution(std::move(attributes), std::move(statement)), location);
        } else {
            return statement;
        }
    }

    FwdUniquePtr<const Statement> Parser::parseInStatement() {
        // relocation = `in` (IDENTIFIER (`.` IDENTIFIER)*) (`,` expression)? block
        const auto location = scanner->getLocation();
        FwdUniquePtr<const Expression> dest;
        
        nextToken(); // IDENTIFIER (keyword `in`)

        // identifier = IDENTIFIER (`.` IDENTIFIER)*
        std::vector<StringView> pieces;
        if (checkIdentifier()) {
            pieces.push_back(token.text);
        }
        nextToken(); // IDENTIFIER

        // (`.` IDENTIFIER)*
        while (token.type == TokenType::Dot) {
            nextToken(); // `.`

            if (token.type == TokenType::Identifier && token.keyword == Keyword::None) {
                pieces.push_back(token.text);
                nextToken(); // IDENTIFIER
            } else {
                reject(token, "identifier after `.` in qualified name"_sv, true);
                return nullptr;
            }                
        }

        // (@ expr)?
        if (token.type == TokenType::At) {
            nextToken(); // @
            dest = parseExpression(); // expression
        }
        
        auto block = parseBlockStatement(); // block
        return makeFwdUnique<const Statement>(Statement::In(pieces, std::move(dest), std::move(block)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseBlockStatement() {
        // block = `{` statement* `}`
        const auto location = scanner->getLocation();
        std::vector<FwdUniquePtr<const Statement>> statements;

        expectTokenType(TokenType::LeftBrace);

        while (report->alive()) {
            if (token.type == TokenType::EndOfFile) {
                reject(token, "`}` to close block `{`"_sv, true, ReportErrorFlags(ReportErrorFlagType::Continued));
                report->error("block `{` started here", location);
                popScanner();
                return nullptr;
            }
            if (token.type == TokenType::RightBrace) {
                nextToken(); // `}`
                return makeFwdUnique<const Statement>(Statement::Block(std::move(statements)), location);
            }
            if (auto statement = parseStatement()) {
                statements.push_back(std::move(statement));
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Statement> Parser::parseNamespaceDeclaration() {
        // namespace = `namespace` IDENTIFIER block
        const auto location = scanner->getLocation();
        StringView name;

        nextToken(); // IDENTIFIER (keyword `namespace`)
        
        if (checkIdentifier()) {
            name = token.text;
        }
        nextToken(); // IDENTIFIER

        auto block = parseBlockStatement(); // block
        return makeFwdUnique<const Statement>(Statement::Namespace(name, std::move(block)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseConfigDirective() {
        // config = `config` `{` IDENTIFIER `=` expression (','? | (',' (IDENTIFIER `=` expression)* ','?) '}'
        const auto location = scanner->getLocation();

        nextToken(); // IDENTIFIER (contextual keyword `config`)

        expectTokenType(TokenType::LeftBrace);
        std::vector<std::unique_ptr<const Statement::Config::Item>> items;

        // IDENTIFIER `=` expression (','? | (',' (IDENTIFIER `=` expression)* ','?) '}'
        do {
            if (token.type == TokenType::RightBrace) {
                break;
            }

            StringView name;
            if (checkIdentifier()) {
                name = token.text;
                nextToken(); // IDENTIFIER
            } else {
                skipToNextStatement();
                return nullptr;
            }
            if (!expectTokenType(TokenType::Equals)) {
                skipToNextStatement();
                return nullptr;
            }
            auto expr = parseExpression(); // expression

            items.push_back(std::make_unique<Statement::Config::Item>(name, std::move(expr)));

            if (token.type == TokenType::RightBrace) {
                break;
            } else if (token.type == TokenType::Comma) {
                nextToken();
            } else {
                reject(token, "`,` or `}`"_sv, false);
                skipToNextStatement();
                return nullptr;
            }
        } while (true);

        if (!expectTokenType(TokenType::RightBrace)) {
            // `}`
            skipToNextStatement();
            return nullptr;
        }      
        
        return makeFwdUnique<const Statement>(Statement::Config(std::move(items)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseBankDeclaration() {
        // bank = `bank` IDENTIFIER (`,` IDENTIFIER)* `:` IDENTIFIER type `;`
        const auto location = scanner->getLocation();
        
        std::vector<StringView> names;
        std::vector<FwdUniquePtr<const Expression>> addresses;
        
        nextToken(); // IDENTIFIER (keyword `bank`)
        
        if (checkIdentifier()) {
            names.push_back(token.text);
        }
        nextToken(); // IDENTIFIER

        if (token.type == TokenType::At) {
            nextToken(); // `@`            
            addresses.push_back(parseLogicalOr(ExpressionParseOptions())); // expression
        } else {
            addresses.push_back(nullptr);
        }

        // Check if we should match (`,` id)*
        while (token.type == TokenType::Comma) {
            nextToken(); // `,`
            if (token.type == TokenType::Identifier) {
                if (checkIdentifier()) {
                    // parse name
                    names.push_back(token.text);
                }
                nextToken(); // IDENTIFIER

                if (token.type == TokenType::At) {
                    nextToken(); // `@`
                    addresses.push_back(parseLogicalOr(ExpressionParseOptions())); // expression
                } else {
                    addresses.push_back(nullptr);
                }
            } else {
                reject(token, "identifier after `,` in bank declaration"_sv, true);
                break;
            }
        }

        expectTokenType(TokenType::Colon); // :
        auto type = parseType();
        expectStatementEnd("`bank` declaration"_sv);

        return makeFwdUnique<const Statement>(Statement::Bank(names, std::move(addresses), std::move(type)), location);
    }
    
    FwdUniquePtr<const Statement> Parser::parseLetDeclaration() {
        // let = `let` IDENTIFIER (`(` (IDENTIFIER (`,` IDENTIFIER))? `)`)? `=` expression `;`
        const auto location = scanner->getLocation();
        
        StringView name;     
        nextToken(); // IDENTIFIER (keyword `let`)
        if (checkIdentifier()) {
            name = token.text;
        }
        nextToken(); // IDENTIFIER

        bool isFunction = false;
        std::vector<StringView> parameters;
        if (token.type == TokenType::LeftParenthesis) {
            bool done = false;
            bool err = false;

            isFunction = true;

            nextToken(); // `(`

            // item (`,` item)*
            while (!done) {
                StringView parameter;

                if (token.type == TokenType::RightParenthesis) {
                    done = true;
                } else if (token.type == TokenType::Comma) {
                    reject(token, "parameter"_sv, true);
                    err = true;
                    done = true;
                } else {
                    if (token.type == TokenType::Identifier) {
                        if (checkIdentifier()) {
                            parameter = token.text;
                            nextToken(); // IDENTIFIER
                        }
                        parameters.push_back(parameter);
                    } else {
                        reject(token, "parameter"_sv, true);
                        err = true;
                        done = true;
                    }
                }

                if (!done) {
                    // (`,` item)*
                    if (token.type == TokenType::Comma) {
                        nextToken(); // `,`
                    } else if (token.type == TokenType::RightParenthesis) {
                        done = true;
                    } else {
                        reject(token, "`,` or `)`"_sv, true);
                        err = true;
                        done = true;
                    }
                }
            }

            if (err) {
                while (token.type != TokenType::RightParenthesis && token.type != TokenType::EndOfFile) {
                    nextToken(); // !(`)` | eof)*
                }
                nextToken(); // (`)` | eof)
                skipToNextStatement();

                return nullptr;
            } else {
                expectTokenType(TokenType::RightParenthesis); // `)`
            }
        }

        expectTokenType(TokenType::Equals); // =
        auto value = parseExpressionWithOptions(ExpressionParseOptions { ExpressionParseOption::AllowStructLiterals }); // expression(allow_struct_literals)
        expectStatementEnd("`let` declaration"_sv);

        return makeFwdUnique<const Statement>(Statement::Let(name, isFunction, parameters, std::move(value)), location);
    }
    
    FwdUniquePtr<const Statement> Parser::parseEnumDeclaration() {
        // enum = `enum` IDENTIFIER : type `{` IDENTIFIER (`=` expression)? (','? | (',' (IDENTIFIER (`=` expression)?) ','?)+) '}'
        const auto location = scanner->getLocation();

        nextToken(); // IDENTIFIER (keyword `enum`)

        StringView name;
        if (checkIdentifier()) {
            name = token.text;
        }
        nextToken(); // IDENTIFIER

        FwdUniquePtr<const TypeExpression> typeExpression;
        if (token.type == TokenType::Colon) {
            nextToken(); // `:`
            typeExpression = parseType();
        } else {
            bool wasLeftBrace = token.type == TokenType::LeftBrace;
            expectTokenType(TokenType::Colon);
            if (wasLeftBrace) {
                skipToNextStatement();
                nextToken();
            } else {
                skipToNextStatement();
            }
            return nullptr;
        }

        expectTokenType(TokenType::LeftBrace);
        std::vector<std::unique_ptr<const Statement::Enum::Item>> items;

        // IDENTIFIER `=` expression (','? | (',' (IDENTIFIER `=` expression) ','?)+) '}'
        do {
            if (token.type == TokenType::RightBrace) {
                break;
            }

            const auto itemLocation = scanner->getLocation();

            StringView itemName;
            if (checkIdentifier()) {
                itemName = token.text;
                nextToken(); // IDENTIFIER
            } else {
                skipToNextStatement();
                return nullptr;
            }

            FwdUniquePtr<const Expression> expression;

            if (token.type == TokenType::Equals) {
                nextToken(); // `=`
                expression = parseExpressionWithOptions(ExpressionParseOptions { ExpressionParseOption::AllowStructLiterals }); // expression(allow_struct_literals)
            }

            items.push_back(std::make_unique<Statement::Enum::Item>(itemName, std::move(expression), itemLocation));

            if (token.type == TokenType::RightBrace) {
                break;
            } else if (token.type == TokenType::Comma) {
                nextToken();
            } else {
                reject(token, "`,` or `}`"_sv, false);
                skipToNextStatement();
                return nullptr;
            }
        } while (true);

        if (!expectTokenType(TokenType::RightBrace)) {
            // `}`
            skipToNextStatement();
            return nullptr;
        }      
        
        return makeFwdUnique<const Statement>(Statement::Enum(name, std::move(typeExpression), std::move(items)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseStructDeclaration(StructKind structKind) {
        // struct = (`struct`|`union`) IDENTIFIER `{` IDENTIFIER `:` type (','? | (',' (IDENTIFIER `:` type ','?)+) '}'
        const auto location = scanner->getLocation();

        nextToken(); // IDENTIFIER (keyword `struct` or `union`)

        StringView name;
        if (checkIdentifier()) {
            name = token.text;
        }
        nextToken(); // IDENTIFIER

        expectTokenType(TokenType::LeftBrace);
        std::vector<std::unique_ptr<const Statement::Struct::Item>> items;

        // IDENTIFIER `:` type (','? | (',' (IDENTIFIER `:` type ','?)+) '}'
        do {
            if (token.type == TokenType::RightBrace) {
                break;
            }

            const auto itemLocation = scanner->getLocation();

            StringView itemName;
            if (checkIdentifier()) {
                itemName = token.text;
                nextToken(); // IDENTIFIER
            } else {
                skipToNextStatement();
                return nullptr;
            }

            expectTokenType(TokenType::Colon);
            auto itemType = parseType(); // type

            items.push_back(std::make_unique<Statement::Struct::Item>(itemName, std::move(itemType), itemLocation));

            if (token.type == TokenType::RightBrace) {
                break;
            } else if (token.type == TokenType::Comma) {
                nextToken();
            } else {
                reject(token, "`,` or `}`"_sv, false);
                skipToNextStatement();
                return nullptr;
            }
        } while (true);

        if (!expectTokenType(TokenType::RightBrace)) {
            // `}`
            skipToNextStatement();
            return nullptr;
        }      
        
        return makeFwdUnique<const Statement>(Statement::Struct(structKind, name, std::move(items)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseVarDeclaration(Modifiers modifiers) {
        // var = `var` IDENTIFIER (`,` IDENTIFIER)* `:` type `;`
        const auto location = scanner->getLocation();        

        const char* description = "`var` declaration";
        if (modifiers.contains<Modifier::Const>()) {
            description = "`const` declaration";
        } else if (modifiers.contains<Modifier::WriteOnly>()) {
            description = "`writeonly` declaration";
        }

        std::vector<StringView> names;
        std::vector<FwdUniquePtr<const Expression>> addresses;
        nextToken(); // IDENTIFIER (keyword `var`)
        
        if (!modifiers.contains<Modifier::Const>() || token.type == TokenType::Identifier) {
            if (checkIdentifier()) {
                names.push_back(token.text);
            }
            nextToken(); // IDENTIFIER
        } else {
            names.push_back(stringPool->intern("$const" + std::to_string(symbolIndex++)));
        }

        if (token.type == TokenType::At) {
            nextToken(); // `@`
            addresses.push_back(parseLogicalOr(ExpressionParseOptions())); // expression
        } else {
            addresses.push_back(nullptr);
        }        

        // Check if we should match (`,` id)*
        while (!modifiers.contains<Modifier::Const>() && token.type == TokenType::Comma) {
            nextToken(); // `,`
            if (token.type == TokenType::Identifier) {
                if (checkIdentifier()) {
                    // parse name
                    names.push_back(token.text);
                }
                nextToken(); // IDENTIFIER

                if (token.type == TokenType::At) {
                    nextToken(); // `@`
                    addresses.push_back(parseLogicalOr(ExpressionParseOptions())); // expression
                } else {
                    addresses.push_back(nullptr);
                }
            } else {
                reject(token, stringPool->intern(std::string() + "identifier after `,` in " + description), true);
                skipToNextStatement();
                return nullptr;
            }
        }

        if (token.type != TokenType::Colon && token.type != TokenType::Equals) {
            reject(token, stringPool->intern(std::string() + "type `:` or initializer `=` for " + description), true);
            skipToNextStatement();
            return nullptr;
        }

        FwdUniquePtr<const TypeExpression> type;
        if (token.type == TokenType::Colon) {
            nextToken(); // `:`
            type = parseType();
        }

        FwdUniquePtr<const Expression> value;
        if (token.type == TokenType::Equals) { 
            nextToken(); // `=`
            value = parseExpressionWithOptions(ExpressionParseOptions { ExpressionParseOption::AllowStructLiterals }); // expression(allow_struct_literals)
        }
        expectStatementEnd(StringView(description));

        return makeFwdUnique<const Statement>(Statement::Var(modifiers, names, std::move(addresses), std::move(type), std::move(value)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseTypeAliasDeclaration() {
        // alias = `alias` IDENTIFIER `=` type `;`
        const auto location = scanner->getLocation();
        
        StringView name;
        nextToken(); // IDENTIFIER (keyword `typealias`)
        
        if (checkIdentifier()) {
            name = token.text;
        }
        nextToken(); // IDENTIFIER

        expectTokenType(TokenType::Colon); // `:`
        auto type = parseType();
        expectStatementEnd("`alias` declaration"_sv);

        return makeFwdUnique<const Statement>(Statement::TypeAlias(name, std::move(type)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseFuncDeclaration(bool inlined, bool far) {
        // function_declaration = `func` IDENTIFIER parameter_list? (`:` type)? block
        // parameter_list = `(` parameter (`,` parameter)* `)`
        const auto location = scanner->getLocation();

        const auto parseFunctionParameter = [&]() {
            // parameter = IDENTIFIER `:` type
            const auto location = scanner->getLocation();

            StringView name;
            if (checkIdentifier()) {
                name = token.text;
            }
            nextToken(); // IDENTIFIER

            expectTokenType(TokenType::Colon); // `:`
            auto type = parseType();

            return std::make_unique<const Statement::Func::Parameter>(name, std::move(type), location);
        };

        StringView name;
        std::vector<std::unique_ptr<const Statement::Func::Parameter>> parameters;
        FwdUniquePtr<const TypeExpression> returnType;
        nextToken(); // IDENTIFIER (keyword `func`)

        if (checkIdentifier()) {
            name = token.text;
        }
        nextToken(); // IDENTIFIER

        // parameter_list?
        if (token.type == TokenType::LeftParenthesis) {
            nextToken(); // `(`
            if (token.type == TokenType::Identifier) {
                parameters.push_back(parseFunctionParameter());

                // Check if we should match (`,` id)*
                while (token.type == TokenType::Comma) {
                    nextToken(); // `,`
                    if (token.type == TokenType::Identifier) {
                        parameters.push_back(parseFunctionParameter());
                    } else {
                        reject(token, "another parameter after `,` in `func` parameter list"_sv, true);
                        break;
                    }
                }
            } else if (token.type != TokenType::RightParenthesis) {
                reject(token, "parameter after `(` in `func` definition"_sv, true);
            }

            expectTokenType(TokenType::RightParenthesis); // `)`
        }


        // (`:` type)?
        if (token.type == TokenType::Colon) {
            nextToken(); // `:`
            returnType = parseType();
        } else {
            returnType = makeFwdUnique<const TypeExpression>(TypeExpression::Tuple({}), location);
        }

        auto block = parseBlockStatement();

        return makeFwdUnique<const Statement>(Statement::Func(inlined, far, name, std::move(parameters), std::move(returnType), std::move(block)), location);   
    }

    FwdUniquePtr<const Statement> Parser::parseDistanceHintedStatement() {
        std::size_t distanceHint = 0;
        while (token.type == TokenType::Caret) {
            nextToken(); // `^`
            ++distanceHint;
        }
        switch (token.keyword) {
            case Keyword::If:
                return parseIfStatement(distanceHint);
            case Keyword::While:
                return parseWhileStatement(distanceHint);
            case Keyword::Do:
                return parseDoWhile(distanceHint);
            case Keyword::For:
                return parseForStatement(distanceHint);
            case Keyword::Goto:
            case Keyword::IrqReturn:
            case Keyword::NmiReturn:
            case Keyword::Return:
            case Keyword::Break:
            case Keyword::Continue:
                return parseBranchStatement(distanceHint, false);
            default:
                reject(token, stringPool->intern("control-flow statement after `" + std::string(distanceHint, '^') + "`"), false);
                skipToNextStatement();
                return nullptr;
        }
    }

    FwdUniquePtr<const Statement> Parser::parseInlineStatement() {
        // inline = `inline` (function_declaration | inline_for_statement)
        const auto location = scanner->getLocation();

        nextToken(); // IDENTIFIER (keyword `inline`)
        if (token.keyword == Keyword::Func) {
            return parseFuncDeclaration(true, false);
        } else if (token.keyword == Keyword::For) {
            return parseInlineForStatement();
        } else if (token.type == TokenType::Identifier && token.keyword == Keyword::None) {
            if (auto expression = parseInlineCall())  {
                return makeFwdUnique<const Statement>(Statement::ExpressionStatement(std::move(expression)), location);
            }
            return nullptr;
        } else {
            reject(token, "valid inline statement after the `inline` keyword"_sv, false);
            skipToNextStatement();
            return nullptr;
        }
    }
    
    FwdUniquePtr<const Statement> Parser::parseFarStatement() {
        // inline = `inline` (function_declaration | inline_for_statement)
        const auto location = scanner->getLocation();

        nextToken(); // IDENTIFIER (keyword `far`)
        if (token.keyword == Keyword::Func) {
            return parseFuncDeclaration(false, true);
        } else if (token.keyword == Keyword::Return) {
            return parseBranchStatement(0, true);
        } else if (token.type == TokenType::Identifier && token.keyword == Keyword::None) {
            StringView name = token.text;

            nextToken(); // IDENTIFIER
            expectTokenType(TokenType::Colon);
                
            return makeFwdUnique<const Statement>(Statement::Label(true, name), location);
        } else {
            reject(token, "valid far statement after the `far` keyword"_sv, false);
            skipToNextStatement();
            return nullptr;
        }
    }

    FwdUniquePtr<const Statement> Parser::parseBranchStatement(std::size_t distanceHint, bool far) {
        // branch_statement = `goto` expression (`if` condition)? `;`
        //      | `return` expression? (`if` condition)? `;`
        //      | (`break` | `continue`) (`if` condition)? `;`
        const auto location = scanner->getLocation();
        const auto branchKeyword = token.keyword;
        nextToken(); // IDENTIFIER (keyword)

        switch (branchKeyword) {
            case Keyword::Goto: {
                auto destination = parseExpression();

                FwdUniquePtr<const Expression> condition;
                if (token.keyword == Keyword::If) {
                    nextToken(); // IDENTIFIER (keyword `if`)
                    condition = parseExpression();
                    if (condition == nullptr) {
                        skipToNextStatement();
                        return nullptr;
                    }
                }

                expectStatementEnd("`goto` statement"_sv);
                return makeFwdUnique<const Statement>(Statement::Branch(distanceHint, BranchKind::Goto, std::move(destination), nullptr, std::move(condition)), location);
            }
            case Keyword::Return: 
            case Keyword::IrqReturn:
            case Keyword::NmiReturn: {
                const auto kind = [&]() {
                    switch (branchKeyword) {
                        case Keyword::Return: return far ? BranchKind::FarReturn : BranchKind::Return;
                        case Keyword::IrqReturn: return BranchKind::IrqReturn;
                        case Keyword::NmiReturn: return BranchKind::NmiReturn;
                        default: std::abort(); break;
                    }
                }();

                FwdUniquePtr<const Expression> returnValue;
                if (token.type != TokenType::Semicolon
                && token.keyword != Keyword::If) {
                    returnValue = parseExpression();
                }

                FwdUniquePtr<const Expression> condition;
                if (token.keyword == Keyword::If) {
                    nextToken(); // IDENTIFIER (keyword `if`)
                    condition = parseExpression();
                    if (condition == nullptr) {
                        skipToNextStatement();
                        return nullptr;
                    }
                }

                expectStatementEnd(stringPool->intern("`" + getKeywordName(branchKeyword).toString() + "` statement"));
                return makeFwdUnique<const Statement>(Statement::Branch(distanceHint, kind, nullptr, std::move(returnValue), std::move(condition)), location);
            }
            case Keyword::Break:
            case Keyword::Continue: {
                const auto kind = [&]() {
                    switch (branchKeyword) {
                        case Keyword::Break: return BranchKind::Break;
                        case Keyword::Continue: return BranchKind::Continue;
                        default: std::abort(); break;
                    }
                }();

                FwdUniquePtr<const Expression> condition;
                if (token.keyword == Keyword::If) {
                    nextToken(); // IDENTIFIER (keyword `if`)
                    condition = parseExpression();
                    if (condition == nullptr) {
                        skipToNextStatement();
                        return nullptr;
                    }
                }

                expectStatementEnd(stringPool->intern("`" + getKeywordName(branchKeyword).toString() + "` statement"));
                return makeFwdUnique<const Statement>(Statement::Branch(distanceHint, kind, nullptr, nullptr, std::move(condition)), location);
            }
            default:
                reject(token, "branch statement"_sv, true);
                return nullptr;
        }
    }

    FwdUniquePtr<const Statement> Parser::parseIfStatement(std::size_t distanceHint) {
        // if_statement = `if` expression(conditional) block (`else` (if_statement | block))?        
        const auto location = scanner->getLocation();
        
        nextToken(); // IDENTIFIER (keyword `if`)
        auto condition = parseExpressionWithOptions(ExpressionParseOptions(ExpressionParseOption::Conditional)); // expression(conditional)
        auto body = parseBlockStatement(); // block
        FwdUniquePtr<const Statement> alternative;

        // (`else` (if_statement | block))?
        if (token.keyword == Keyword::Else) {
            nextToken(); // IDENTIFIER (keyword `else`)

            if (token.keyword == Keyword::If) {
                alternative = parseIfStatement(distanceHint); // conditional
            } else {
                alternative = parseBlockStatement(); // block
            }
        }

        return makeFwdUnique<const Statement>(Statement::If(distanceHint, std::move(condition), std::move(body), std::move(alternative)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseWhileStatement(std::size_t distanceHint) {
        // while_statement = `while` `(` condition `)` block
        const auto location = scanner->getLocation();

        nextToken(); // IDENTIFIER (keyword `while`)
        auto condition = parseExpressionWithOptions(ExpressionParseOptions(ExpressionParseOption::Conditional)); // expression(conditional)
        auto body = parseBlockStatement(); // block

        return makeFwdUnique<const Statement>(Statement::While(distanceHint, std::move(condition), std::move(body)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseDoWhile(std::size_t distanceHint) {
        // do_while_statement = `do` block `while` `(` condition `)` `;`
        const auto location = scanner->getLocation();

        nextToken(); // IDENTIFIER (keyword `do`)
        auto body = parseBlockStatement(); // block

        FwdUniquePtr<const Expression> condition;
        if (token.keyword == Keyword::While) {
            nextToken(); // IDENTIFIER (keyword `while`)
            condition = parseExpressionWithOptions(ExpressionParseOptions(ExpressionParseOption::Conditional)); // expression(conditional)
        } else {
            reject(token, "`while`"_sv, true);
        }

        expectStatementEnd("`do` ... `while` statement"_sv);

        return makeFwdUnique<const Statement>(Statement::DoWhile(distanceHint, std::move(body), std::move(condition)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseForStatement(std::size_t distanceHint) {
        // for_statement = `for` `(` identifier `in` expression `)` block
        const auto location = scanner->getLocation();
        nextToken(); // IDENTIFIER (keyword `for`)

        auto counter = parseExpression();
        if (token.keyword == Keyword::In) {
            nextToken(); // IDENTIFIER (keyword `in`)
        } else {
            reject(token, "`in`"_sv, true);
        }

        auto sequence = parseExpression(); // expression
        auto block = parseBlockStatement(); // block

        return makeFwdUnique<const Statement>(Statement::For(distanceHint, std::move(counter), std::move(sequence), std::move(block)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseInlineForStatement() {
        // inline_for_statement = `inline` `for` `(` identifier `in` expression `)` block
        const auto location = scanner->getLocation();
        nextToken(); // IDENTIFIER (keyword `for`)

        StringView name;
        if (token.keyword == Keyword::Let) {
            nextToken(); // IDENTIFIER (keyword `let`)
            if (checkIdentifier()) {
                name = token.text;
            }
            nextToken(); // IDENTIFIER
        } else if (token.keyword == Keyword::In) {
            name = stringPool->intern("$let" + std::to_string(symbolIndex++));
        } else {
            reject(token, "`let` or `in` after `inline for`"_sv, true);
        }

        if (token.keyword == Keyword::In) {
            nextToken(); // IDENTIFIER (keyword `in`)
        } else {
            reject(token, "`in`"_sv, true);
        }

        auto sequence = parseExpression(); // expression
        auto block = parseBlockStatement(); // block

        return makeFwdUnique<const Statement>(Statement::InlineFor(name, std::move(sequence), std::move(block)), location);
    }

    FwdUniquePtr<const Statement> Parser::parseExpressionStatement() {
        // expression_statement = expression `;`
        const auto location = scanner->getLocation();

        auto expression = parseExpression(); // term

        if (expression) {
            bool badExpressionStatement = false;
            StringView statementDescription = "statement"_sv;
            StringView expressionDescription = "expression"_sv;

            if (const auto unary = expression->variant.tryGet<Expression::UnaryOperator>()) {
                expressionDescription = getUnaryOperatorName(unary->op);

                switch (unary->op) {
                    case UnaryOperatorKind::PostDecrement:
                    case UnaryOperatorKind::PostIncrement:
                    case UnaryOperatorKind::PreDecrement:
                    case UnaryOperatorKind::PreIncrement:
                        break;
                    default:
                        badExpressionStatement = true;
                        break;
                }
            } else if (const auto binary = expression->variant.tryGet<Expression::BinaryOperator>()) {
                expressionDescription = getBinaryOperatorName(binary->op);

                switch (binary->op) {
                    case BinaryOperatorKind::Assignment:
                        break;
                    default:
                        badExpressionStatement = true;
                        break;
                }
            } else if (!expression->variant.is<Expression::Call>()) {
                badExpressionStatement = true;
            }

            if (badExpressionStatement) {
                if (const auto identifier = expression->variant.tryGet<Expression::Identifier>()) {
                    report->error("expected " + statementDescription.toString() + ", but got identifier `" + text::join(identifier->pieces.begin(), identifier->pieces.end(), ".") + "`", expression->location);
                } else {
                    report->error("expected " + statementDescription.toString() + ", but got " + expressionDescription.toString(), expression->location);                
                }
                skipToNextStatement();
                return nullptr;
            } else {
                expectStatementEnd("statement"_sv);
                return makeFwdUnique<const Statement>(Statement::ExpressionStatement(std::move(expression)), location);
            }
        } else {
            skipToNextStatement();
            return nullptr;
        }
    }

    FwdUniquePtr<const Expression> Parser::parseExpression() {
        return parseExpressionWithOptions(ExpressionParseOptions());
    }

    FwdUniquePtr<const Expression> Parser::parseExpressionWithOptions(ExpressionParseOptions options) {
        // expression = assignment
        return parseAssignment(options);
    }
    
    FwdUniquePtr<const Expression> Parser::parseAssignment(ExpressionParseOptions options) {
        // assignment = logical_or ((compound_assignment_op | `=`) assignment)*
        FwdUniquePtr<const Expression> left;

        left = parseLogicalOr(options); // logical_or
        if (report->alive()) {
            BinaryOperatorKind op = BinaryOperatorKind::None;
            switch (token.type) {
                case TokenType::PlusEquals: op = BinaryOperatorKind::Addition; break;
                case TokenType::PlusHashEquals: op = BinaryOperatorKind::AdditionWithCarry; break;
                case TokenType::MinusEquals: op = BinaryOperatorKind::Subtraction; break;
                case TokenType::MinusHashEquals: op = BinaryOperatorKind::SubtractionWithCarry; break;
                case TokenType::AsteriskEquals: op = BinaryOperatorKind::Multiplication; break;
                case TokenType::SlashEquals: op = BinaryOperatorKind::Division; break;
                case TokenType::PercentEquals: op = BinaryOperatorKind::Modulo; break;
                case TokenType::AmpersandEquals: op = BinaryOperatorKind::BitwiseAnd; break;
                case TokenType::CaretEquals: op = BinaryOperatorKind::BitwiseXor; break;
                case TokenType::BarEquals: op = BinaryOperatorKind::BitwiseOr; break;
                case TokenType::DoubleLessThanEquals: op = BinaryOperatorKind::LeftShift; break;
                case TokenType::DoubleGreaterThanEquals: op = BinaryOperatorKind::RightShift; break;
                case TokenType::TripleLessThanEquals: op = BinaryOperatorKind::LogicalLeftShift; break;
                case TokenType::TripleGreaterThanEquals: op = BinaryOperatorKind::LogicalRightShift; break;
                case TokenType::QuadrupleLessThanEquals: op = BinaryOperatorKind::LeftRotate; break;
                case TokenType::QuadrupleGreaterThanEquals: op = BinaryOperatorKind::RightRotate; break;
                case TokenType::QuadrupleLessThanHashEquals: op = BinaryOperatorKind::LeftRotateWithCarry; break;
                case TokenType::QuadrupleGreaterThanHashEquals: op = BinaryOperatorKind::RightRotateWithCarry; break;
                default: op = BinaryOperatorKind::None; break;
            }
            if (op != BinaryOperatorKind::None) {
                const auto location = scanner->getLocation();
                nextToken(); // operator token
                auto right = parseAssignment(options | ExpressionParseOptions(ExpressionParseOption::AllowStructLiterals)); // assignment
                right = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    op, left->clone(), std::move(right)), location, Optional<ExpressionInfo>());
                return makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    BinaryOperatorKind::Assignment, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());
            } else if (token.type == TokenType::Equals) {
                const auto location = scanner->getLocation();
                nextToken(); // `=`
                auto right = parseAssignment(options | ExpressionParseOptions(ExpressionParseOption::AllowStructLiterals)); // assignment
                return makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    BinaryOperatorKind::Assignment, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());                
            } else {
                return left;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseLogicalOr(ExpressionParseOptions options) {
        // logical_or = logical_and (`||` logical_and)*
        FwdUniquePtr<const Expression> left;

        left = parseLogicalAnd(options); // logical_and
        while (report->alive()) {
            if (token.type == TokenType::DoubleBar) {
                const auto location = scanner->getLocation();
                nextToken(); // operator token
                auto right = parseLogicalAnd(options); // logical_and
                left = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    BinaryOperatorKind::LogicalOr, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());
            } else {
                return left;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseLogicalAnd(ExpressionParseOptions options) {
        // logical_conjunction = comparison (`&&` comparison)*
        FwdUniquePtr<const Expression> left;

        left = parseComparison(options); // comparison
        while (report->alive()) {
            if (token.type == TokenType::DoubleAmpersand) {
                const auto location = scanner->getLocation();
                nextToken(); // operator token
                auto right = parseComparison(options); // comparison
                left = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    BinaryOperatorKind::LogicalAnd, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());
            } else {
                return left;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseComparison(ExpressionParseOptions options) {
        // comparison = range (cmp_op range)*
        FwdUniquePtr<const Expression> left;

        left = parseRange(options); // range
        while (report->alive()) {
            BinaryOperatorKind op = BinaryOperatorKind::None;
            switch (token.type) {
                case TokenType::ExclamationEquals: op = BinaryOperatorKind::NotEqual; break;
                case TokenType::DoubleEquals: op = BinaryOperatorKind::Equal; break;
                case TokenType::LessThan: op = BinaryOperatorKind::LessThan; break;
                case TokenType::GreaterThan: op = BinaryOperatorKind::GreaterThan; break;
                case TokenType::LessThanEquals: op = BinaryOperatorKind::LessThanOrEqual; break;
                case TokenType::GreaterThanEquals: op = BinaryOperatorKind::GreaterThanOrEqual; break;
                default: op = BinaryOperatorKind::None; break;
            }
            if (op != BinaryOperatorKind::None) {
                const auto location = scanner->getLocation();
                nextToken(); // operator token
                auto right = parseRange(options); // range
                left = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    op, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());
            } else {
                return left;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseRange(ExpressionParseOptions options) {
        // range = bitwise_or ((`...` | `..<` | `>..`) bitwise_or (`by` bitwise_or)?)?
        FwdUniquePtr<const Expression> left;

        left = parseBitwiseOr(options); // bitwise_or
        if (report->alive()) {
            if (token.type == TokenType::DotDot) {
                const auto location = scanner->getLocation();

                nextToken(); // operator token
                auto right = parseBitwiseOr(options); // bitwise_or

                FwdUniquePtr<const Expression> step;
                if (token.type == TokenType::Identifier && token.keyword == Keyword::By) {
                    nextToken(); // IDENTIFIER (keyword `by`)
                    step = parseBitwiseOr(options); // bitwise_or
                }

                return makeFwdUnique<const Expression>(Expression::RangeLiteral(std::move(left), std::move(right), std::move(step)), location, Optional<ExpressionInfo>());
            } else {
                return left;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseBitwiseOr(ExpressionParseOptions options) {
        // bitwise_or = bitwise_xor (`|` bitwise_xor)*
        FwdUniquePtr<const Expression> left;

        left = parseBitwiseXor(options); // bitwise_xor
        while (report->alive()) {
            if (token.type == TokenType::Bar) {
                const auto location = scanner->getLocation();
                nextToken(); // operator token
                auto right = parseBitwiseXor(options); // bitwise_xor
                left = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    BinaryOperatorKind::BitwiseOr, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());
            } else {
                return left;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseBitwiseXor(ExpressionParseOptions options) {
        // bitwise_xor = bitwise_and (`^` bitwise_and)*
        FwdUniquePtr<const Expression> left;

        left = parseBitwiseAnd(options); // bitwise_and
        while (report->alive()) {
            if (token.type == TokenType::Caret) {
                const auto location = scanner->getLocation();
                nextToken(); // operator token
                auto right = parseBitwiseAnd(options); // bitwise_and
                left = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    BinaryOperatorKind::BitwiseXor, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());
            } else {
                return left;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseBitwiseAnd(ExpressionParseOptions options) {
        // bitwise_and = shift (`&` shift)*
        FwdUniquePtr<const Expression> left;

        left = parseShift(options); // shift
        while (report->alive()) {
            if (token.type == TokenType::Ampersand) {
                const auto location = scanner->getLocation();
                nextToken(); // operator token
                auto right = parseShift(options); // shift
                left = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    BinaryOperatorKind::BitwiseAnd, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());
            } else {
                return left;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseShift(ExpressionParseOptions options) {
        // shift = addition (shift_op addition)*
        FwdUniquePtr<const Expression> left;

        left = parseAddition(options); // addition
        while (report->alive()) {
            BinaryOperatorKind op = BinaryOperatorKind::None;
            switch (token.type) {
                case TokenType::DoubleLessThan: op = BinaryOperatorKind::LeftShift; break;
                case TokenType::DoubleGreaterThan: op = BinaryOperatorKind::RightShift; break;
                case TokenType::TripleLessThan: op = BinaryOperatorKind::LogicalLeftShift; break;
                case TokenType::TripleGreaterThan: op = BinaryOperatorKind::LogicalRightShift; break;
                case TokenType::QuadrupleLessThan: op = BinaryOperatorKind::LeftRotate; break;
                case TokenType::QuadrupleGreaterThan: op = BinaryOperatorKind::RightRotate; break;
                case TokenType::QuadrupleLessThanHash: op = BinaryOperatorKind::LeftRotateWithCarry; break;
                case TokenType::QuadrupleGreaterThanHash: op = BinaryOperatorKind::RightRotateWithCarry; break;
                default: op = BinaryOperatorKind::None; break;
            }

            if (op != BinaryOperatorKind::None) {
                const auto location = scanner->getLocation();
                nextToken(); // operator token
                auto right = parseAddition(options); // addition
                left = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    op, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());
            } else {
                return left;
            }
        }
        return nullptr;
    }


    FwdUniquePtr<const Expression> Parser::parseAddition(ExpressionParseOptions options) {
        // addition = multiplication (add_op multiplication)*
        FwdUniquePtr<const Expression> left;

        left = parseMultiplication(options); // multiplication
        while (report->alive()) {
            BinaryOperatorKind op = BinaryOperatorKind::None;
            switch (token.type) {
                case TokenType::Plus: op = BinaryOperatorKind::Addition; break;
                case TokenType::Minus: op = BinaryOperatorKind::Subtraction; break;
                case TokenType::PlusHash: op = BinaryOperatorKind::AdditionWithCarry; break;
                case TokenType::MinusHash: op = BinaryOperatorKind::SubtractionWithCarry; break;
                default: op = BinaryOperatorKind::None; break;
            }

            if (op != BinaryOperatorKind::None) {
                const auto location = scanner->getLocation();
                nextToken(); // operator token
                auto right = parseMultiplication(options); // multiplication
                left = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    op, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());
            } else {
                return left;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseMultiplication(ExpressionParseOptions options) {
        // multiplication = cast (Multiply_op cast)*
        FwdUniquePtr<const Expression> left;

        left = parseCast(options); // prefix
        while (report->alive()) {
            BinaryOperatorKind op = BinaryOperatorKind::None;
            switch (token.type) {
                case TokenType::Asterisk: op = BinaryOperatorKind::Multiplication; break;
                case TokenType::Slash: op = BinaryOperatorKind::Division; break;
                case TokenType::Percent: op = BinaryOperatorKind::Modulo; break;
                default: op = BinaryOperatorKind::None; break;
            }

            if (op != BinaryOperatorKind::None) {
                const auto location = scanner->getLocation();
                nextToken(); // operator token
                auto right = parseCast(options); // prefix
                left = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                    op, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());
            } else {
                return left;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseCast(ExpressionParseOptions options) {
        // casting = prefix ('as' prefix)*
        auto operand = parsePrefix(options); // prefix
        while (report->alive()) {
            if (token.keyword == Keyword::As) {
                const auto location = scanner->getLocation();
                nextToken(); // IDENTIFIER (keyword `as`)
                auto type = parseType(); // type
                operand = makeFwdUnique<const Expression>(Expression::Cast(
                    std::move(operand), std::move(type)), location, Optional<ExpressionInfo>());
            } else {
                return operand;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parsePrefix(ExpressionParseOptions options) {
        // prefix = prefix_op prefix | postfix
        UnaryOperatorKind op = UnaryOperatorKind::None;
        switch (token.type) {
            case TokenType::Minus: op = UnaryOperatorKind::SignedNegation; break;
            case TokenType::Tilde: op = UnaryOperatorKind::BitwiseNegation; break;
            case TokenType::DoublePlus: op = UnaryOperatorKind::PreIncrement; break;
            case TokenType::DoubleMinus: op = UnaryOperatorKind::PreDecrement; break;
            case TokenType::Asterisk: op = UnaryOperatorKind::Indirection; break;
            case TokenType::Ampersand: op = UnaryOperatorKind::AddressOf; break;
            case TokenType::Exclamation: op = UnaryOperatorKind::LogicalNegation; break;
            case TokenType::LessColon: op = UnaryOperatorKind::LowByte; break;
            case TokenType::GreaterColon: op = UnaryOperatorKind::HighByte; break;
            case TokenType::Identifier: {
                if (token.keyword == Keyword::Far) {
                    nextToken(); // `far`

                    if (token.type == TokenType::Ampersand) {
                        op = UnaryOperatorKind::FarAddressOf;
                    } else {
                        expectTokenType(TokenType::Ampersand);
                        return nullptr;
                    }
                }
                break;
            }
            default: op = UnaryOperatorKind::None; break;
        }

        if (op != UnaryOperatorKind::None) {
            const auto location = scanner->getLocation();
            nextToken(); // operator token
            auto operand = parsePrefix(options); // prefix
            return makeFwdUnique<const Expression>(Expression::UnaryOperator(op, std::move(operand)), location, Optional<ExpressionInfo>());
        } else {
            return parsePostfix(options); // postfix
        }
    }

    FwdUniquePtr<const Expression> Parser::parsePostfix(ExpressionParseOptions options) {
        // postfix = term (`@` term)? (postfix_op | `(` expr* `)`)?
        auto operand = parseTerm(options); // term
        if (!operand) {
            return operand;
        }

        while (report->alive()) {
            switch (token.type) {
                case TokenType::Dot: {
                    // (`.` IDENTIFIER)
                    const auto location = scanner->getLocation();
                    nextToken(); // `.`
                    if (token.type == TokenType::Identifier && token.keyword == Keyword::None) {
                        StringView name = token.text;
                        nextToken(); // IDENTIFIER
                        operand = makeFwdUnique<const Expression>(Expression::FieldAccess(std::move(operand), name), location, Optional<ExpressionInfo>());
                    } else {
                        reject(token, "identifier after `.`"_sv, true);
                        return nullptr;
                    }
                    break;
                }
                case TokenType::Dollar: {
                    const auto location = scanner->getLocation();
                    nextToken(); // `$`
                    auto index = parseTerm(options); // term
                    operand = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                        BinaryOperatorKind::BitIndexing, std::move(operand), std::move(index)), location, Optional<ExpressionInfo>());
                    break;
                }
                case TokenType::Tilde: {
                    const auto location = scanner->getLocation();
                    nextToken(); // operator token
                    auto index = parseExpressionWithOptions(options); // expression
                    operand = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                        BinaryOperatorKind::Concatenation, std::move(operand), std::move(index)), location, Optional<ExpressionInfo>());
                    break;
                }
                case TokenType::DoublePlus: {
                    const auto location = scanner->getLocation();
                    nextToken(); // operator token
                    operand = makeFwdUnique<const Expression>(Expression::UnaryOperator(
                        UnaryOperatorKind::PostIncrement, std::move(operand)), location, Optional<ExpressionInfo>());
                    break;
                }
                case TokenType::DoubleMinus: {
                    const auto location = scanner->getLocation();
                    nextToken(); // operator token
                    operand = makeFwdUnique<const Expression>(Expression::UnaryOperator(
                        UnaryOperatorKind::PostDecrement, std::move(operand)), location, Optional<ExpressionInfo>());
                    break;
                }
                case TokenType::Comma: {
                    if (options.contains<ExpressionParseOption::Parenthesized>()) {
                        const auto location = scanner->getLocation();
                        nextToken(); // `,`

                        std::vector<FwdUniquePtr<const Expression>> arguments;
                        arguments.push_back(std::move(operand)); // expression
                        if (token.type != TokenType::RightParenthesis) {
                            // expression (`,` expression)* `)`
                            while (report->alive()) {
                                auto expr = parseExpression(); // expression
                                arguments.push_back(std::move(expr));

                                // (`,` expression)*
                                if (token.type == TokenType::Comma) {
                                    nextToken(); // `,`
                                    if (token.type != TokenType::RightParenthesis) {
                                        continue;
                                    }
                                }

                                break;
                            }
                        }
                        operand = makeFwdUnique<const Expression>(Expression::TupleLiteral(std::move(arguments)), location, Optional<ExpressionInfo>());
                    } else {
                        return operand;
                    }
                    break;
                }
                case TokenType::LeftParenthesis: {
                    operand = parseCallOperator(false, std::move(operand));
                    break;
                }
                case TokenType::LeftBracket: {
                    const auto location = scanner->getLocation();
                    nextToken(); // `[`
                    auto index = parseExpression(); // expression
                    // `]`
                    if (!expectTokenType(TokenType::RightBracket)) {
                        return nullptr;
                    }
                    operand = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                        BinaryOperatorKind::Indexing, std::move(operand), std::move(index)), location, Optional<ExpressionInfo>());
                    break;
                }
                default:
                    return operand;
            }
        }
        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseCallOperator(bool inlined, FwdUniquePtr<const Expression> operand) {
        const auto location = scanner->getLocation();
        nextToken(); // `(`
        std::vector<FwdUniquePtr<const Expression>> arguments;
        if (token.type != TokenType::RightParenthesis) {
            // expression (`,` expression)* `)`
            while (report->alive()) {
                auto expr = parseExpressionWithOptions(ExpressionParseOptions { ExpressionParseOption::AllowStructLiterals }); // expression
                arguments.push_back(std::move(expr));

                // (`,` expression)*
                if (token.type == TokenType::Comma) {
                    nextToken(); // `,`
                    if (token.type != TokenType::RightParenthesis) {
                        continue;
                    }                                
                }

                break;
            }
        }
        // `)`
        if (!expectTokenType(TokenType::RightParenthesis)) {
            return nullptr;
        }

        return makeFwdUnique<const Expression>(Expression::Call(inlined, std::move(operand), std::move(arguments)), location, Optional<ExpressionInfo>());
    }

    FwdUniquePtr<const Expression> Parser::parseInlineCall() {
        const auto location = scanner->getLocation();

        auto qualifiedIdentifier = parseQualifiedIdentifier();
        if (qualifiedIdentifier.size() != 0) {
            return parseCallOperator(true, makeFwdUnique<const Expression>(Expression::Identifier(std::move(qualifiedIdentifier)), location, Optional<ExpressionInfo>()));
        }

        return nullptr;
    }

    FwdUniquePtr<const Expression> Parser::parseTerm(ExpressionParseOptions options) {
        // term(general) =
        //      INTEGER
        //      | HEXADECIMAL
        //      | BINARY
        //      | STRING
        //      | `(` expression(options)? `)`
        //      | list_expression
        //      | IDENTIFIER (`.` IDENTIFIER)*
        //
        // term(conditional) =
        //      term(general)
        //      | block `;`? expr
        const auto location = scanner->getLocation();
        if (options.contains<ExpressionParseOption::Conditional>()) {
            if (token.type == TokenType::LeftBrace) {
                auto block = parseBlockStatement();

                if (token.type == TokenType::DoubleAmpersand) {
                    nextToken(); // `&&`
                    auto comparison = parseComparison(options); // comparison
                    auto left = makeFwdUnique<const Expression>(Expression::SideEffect(std::move(block), std::move(comparison)), location, Optional<ExpressionInfo>());
                    while (report->alive()) {
                        if (token.type == TokenType::DoubleAmpersand) {
                            const auto location = scanner->getLocation();
                            nextToken(); // operator token
                            auto right = parseComparison(options); // comparison
                            left = makeFwdUnique<const Expression>(Expression::BinaryOperator(
                                BinaryOperatorKind::LogicalAnd, std::move(left), std::move(right)), location, Optional<ExpressionInfo>());
                        } else {
                            return left;
                        }
                    }
                } else {
                    reject(token, "`&&` to appear after end-of-block `}` in conditional"_sv, false);
                    return nullptr;
                }
            }
        }

        switch (token.type) {
            case TokenType::Integer:
                return parseIntegerLiteral(10);
            case TokenType::Hexadecimal:
                return parseIntegerLiteral(16);
            case TokenType::Octal:
                return parseIntegerLiteral(8);
            case TokenType::Binary:
                return parseIntegerLiteral(2);
            case TokenType::Character: {
                auto expr = makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(token.text.getLength() == 1 ? token.text[0] : 0), "u8"_sv), location, Optional<ExpressionInfo>());
                nextToken(); // CHARACTER
                return expr;
            }
            case TokenType::String: {
                auto expr = makeFwdUnique<const Expression>(Expression::StringLiteral(token.text), location, Optional<ExpressionInfo>());
                nextToken(); // STRING
                return expr;
            }
            case TokenType::LeftParenthesis: {
                nextToken(); // `(`
                if (token.type == TokenType::RightParenthesis) {
                    nextToken(); // `)`
                    return makeFwdUnique<const Expression>(Expression::TupleLiteral({}), location, Optional<ExpressionInfo>());
                } else {
                    auto expr = parseExpressionWithOptions(options | ExpressionParseOptions { ExpressionParseOption::Parenthesized, ExpressionParseOption::AllowStructLiterals }); // expression 
                    expectTokenType(TokenType::RightParenthesis); // `)`
                    if (expr && expr->variant.is<Expression::TupleLiteral>()) {
                        return expr;
                    } else {
                        return makeFwdUnique<const Expression>(Expression::UnaryOperator(
                            UnaryOperatorKind::Grouping, std::move(expr)), location, Optional<ExpressionInfo>());
                    }
                }
            }
            case TokenType::LeftBracket: {
                // list_expression = `[` (`]` | nonempty_list)
                const auto location = scanner->getLocation();
                nextToken(); // `[`
                if (token.type == TokenType::RightBracket) {
                    nextToken(); // `]`
                    return makeFwdUnique<const Expression>(Expression::ArrayLiteral(), location, Optional<ExpressionInfo>());
                } else {
                    // nonempty_list = expression (list_comprehension | list_padding_specifier | list_remainder)
                    auto head = parseExpressionWithOptions(ExpressionParseOptions { ExpressionParseOption::AllowStructLiterals }); // expression

                    if (token.keyword == Keyword::For) {
                        // list_comprehension = `for` `let` IDENTIFIER `in` expression `]`
                        nextToken(); // IDENTIFIER (keyword `for`)

                        StringView name;
                        bool failed = false;
                        if (token.keyword == Keyword::Let) {
                            nextToken(); // IDENTIFIER (keyword `let`)

                            if (checkIdentifier()) {
                                name = token.text;
                            }

                            nextToken(); // IDENTIFIER
                        } else if (token.keyword == Keyword::In) {
                            name = stringPool->intern("$let" + std::to_string(symbolIndex++));
                        } else {
                            reject(token, "`let` or `in` after `for` in list comprehension"_sv, false);
                            failed = true;
                        }

                        if (token.keyword == Keyword::In) {
                            nextToken(); // IDENTIFIER (keyword `in`)
                        } else {
                            reject(token, "`in`"_sv, true);
                            return nullptr;
                        }

                        auto sequence = parseExpression(); // expression

                        // `]`
                        if (!expectTokenType(TokenType::RightBracket) || failed) {
                            return nullptr;
                        }

                        return makeFwdUnique<const Expression>(Expression::ArrayComprehension(std::move(head), name, std::move(sequence)), location, Optional<ExpressionInfo>());
                    } else if (token.type == TokenType::Semicolon) {
                        nextToken(); // `;`

                        auto sizeExpression = parseExpression(); // expression

                        if (!expectTokenType(TokenType::RightBracket)) {
                            // `]`
                            return nullptr;
                        }
                        return makeFwdUnique<const Expression>(Expression::ArrayPadLiteral(std::move(head), std::move(sizeExpression)), location, Optional<ExpressionInfo>());                    
                    } else {
                        // list_remainder = (`,` expression)* (`,`?) `]`
                        std::vector<FwdUniquePtr<const Expression>> items;
                        items.push_back(std::move(head));

                        // (`,` expression)* `,`?
                        while (token.type == TokenType::Comma) {
                            nextToken(); // `,`
                            if (token.type == TokenType::RightBracket) {
                                break;
                            }

                            auto expr = parseExpressionWithOptions(ExpressionParseOptions { ExpressionParseOption::AllowStructLiterals }); // expression
                            items.push_back(std::move(expr));
                        }
                        if (!expectTokenType(TokenType::RightBracket)) {
                            // `]`
                            return nullptr;
                        }
                        return makeFwdUnique<const Expression>(Expression::ArrayLiteral(std::move(items)), location, Optional<ExpressionInfo>());
                    }
                }
            }            
            case TokenType::Identifier: {
                switch (token.keyword) {
                    case Keyword::None: {
                        auto qualifiedIdentifier = parseQualifiedIdentifier();

                        if (options.contains<ExpressionParseOption::AllowStructLiterals>()
                        && token.type == TokenType::LeftBrace) {
                            nextToken(); // `{`

                            std::unordered_map<StringView, std::unique_ptr<const Expression::StructLiteral::Item>> items;
                            while (token.type != TokenType::RightBrace) {
                                const auto itemLocation = scanner->getLocation();

                                StringView name;
                                if (checkIdentifier()) {
                                    name = token.text;
                                }
                                nextToken(); // IDENTIFIER

                                expectTokenType(TokenType::Equals); // `=`

                                auto value = parseExpressionWithOptions(ExpressionParseOptions { ExpressionParseOption::AllowStructLiterals }); // expression(allow_struct_literals)

                                const auto match = items.find(name);
                                if (match != items.end()) {
                                    report->error("duplicate `" + name.toString() + "` field in struct literal", itemLocation, ReportErrorFlags { ReportErrorFlagType::Continued });
                                    report->error("field `" + name.toString() + "` was previously specified here", match->second->location);
                                } else {
                                    items[name] = std::make_unique<Expression::StructLiteral::Item>(std::move(value), itemLocation);
                                }

                                if (token.type == TokenType::Comma) {
                                    nextToken(); // `,`
                                } else {
                                    break;
                                }
                            }

                            expectTokenType(TokenType::RightBrace);

                            auto type = makeFwdUnique<const TypeExpression>(TypeExpression::Identifier(qualifiedIdentifier), location);
                            return makeFwdUnique<const Expression>(Expression::StructLiteral(std::move(type), std::move(items)), location, Optional<ExpressionInfo>());
                        } else {
                            return makeFwdUnique<const Expression>(Expression::Identifier(std::move(qualifiedIdentifier)), location, Optional<ExpressionInfo>());
                        }
                    }
                    case Keyword::True: {
                        nextToken(); // IDENTIFIER (keyword `true`)
                        return makeFwdUnique<const Expression>(Expression::BooleanLiteral(true), location, Optional<ExpressionInfo>());
                    }
                    case Keyword::False: {
                        nextToken(); // IDENTIFIER (keyword `false`)
                        return makeFwdUnique<const Expression>(Expression::BooleanLiteral(false), location, Optional<ExpressionInfo>());
                    }
                    case Keyword::TypeOf: {
                        nextToken(); // IDENTIFIER (keyword `typeof`)
                        if (expectTokenType(TokenType::LeftParenthesis)) { // `(`
                            auto expression = parseExpression();
                            expectTokenType(TokenType::RightParenthesis);
                            return makeFwdUnique<const Expression>(Expression::TypeOf(std::move(expression)), location, Optional<ExpressionInfo>());        
                        }
                        return nullptr;
                    }
                    case Keyword::SizeOf: {
                        nextToken(); // IDENTIFIER (keyword `sizeof`)
                        if (expectTokenType(TokenType::LeftParenthesis)) { // `(`
                            auto type = parseType();
                            expectTokenType(TokenType::RightParenthesis);
                            return makeFwdUnique<const Expression>(Expression::TypeQuery(TypeQueryKind::SizeOf, std::move(type)), location, Optional<ExpressionInfo>());
                        }
                        return nullptr;
                    }
                    case Keyword::AlignOf: {
                        nextToken(); // IDENTIFIER (keyword `alignof`)
                        if (expectTokenType(TokenType::LeftParenthesis)) { // `(`
                            auto type = parseType();
                            expectTokenType(TokenType::RightParenthesis);
                            return makeFwdUnique<const Expression>(Expression::TypeQuery(TypeQueryKind::AlignOf, std::move(type)), location, Optional<ExpressionInfo>());
                        }
                        return nullptr;
                    }
                    case Keyword::OffsetOf: {
                        nextToken(); // IDENTIFIER (keyword `offsetof`)
                        if (expectTokenType(TokenType::LeftParenthesis)) { // `(`
                            auto type = parseType();
                            if (expectTokenType(TokenType::Comma)) { // `,`
                                // IDENTIFIER
                                if (token.type == TokenType::Identifier && token.keyword == Keyword::None) {
                                    StringView name = token.text;
                                    nextToken();
                                    expectTokenType(TokenType::RightParenthesis);
                                    return makeFwdUnique<const Expression>(Expression::OffsetOf(std::move(type), name), location, Optional<ExpressionInfo>());
                                } else {
                                    reject(token, "identifier after `,`"_sv, true);
                                    expectTokenType(TokenType::RightParenthesis);
                                    return nullptr;
                                }
                            }
                        }
                        return nullptr;
                    }
                    case Keyword::Embed: {
                        nextToken(); // IDENTIFIER (keyword `embed`)
                        if (token.type == TokenType::String) {
                            StringView originalPath = token.text;

                            nextToken(); // STRING
                            return makeFwdUnique<const Expression>(Expression::Embed(originalPath), location, Optional<ExpressionInfo>());
                        } else {
                            expectTokenType(TokenType::String);
                            return nullptr;
                        }
                    }
                    case Keyword::Inline: {
                        nextToken(); // IDENTIFIER (keyword `inline`)
                        return parseInlineCall();
                    }
                    default:
                        reject(token, "expression"_sv, false);
                        return nullptr;
                }
            }
            default:
                reject(token, "expression"_sv, false);
                return nullptr;
        }
    }

    FwdUniquePtr<const Expression> Parser::parseIntegerLiteral(int radix) {
        // number = INTEGER | HEXADECIMAL | BINARY
        const auto location = scanner->getLocation();
        const auto numberToken = token;
        nextToken(); // number

        bool negative = false;
        if (numberToken.text.getLength() > 0 && numberToken.text[0] == '-') {
            negative = true;
        }

        auto t = radix != 10
            ? numberToken.text.sub(2 + (negative ? 1 : 0))
            : numberToken.text.sub(negative ? 1 : 0);
        auto suffix = StringView();

        const auto suffixOffset = t.findFirstOf("ui"_sv);
        if (suffixOffset != SIZE_MAX) {
            suffix = t.sub(suffixOffset);
            t = t.sub(0, suffixOffset);
        }

        if (t.getLength() == 0) {
            report->error(getVerboseTokenName(numberToken) + " is not a valid integer literal", location);
            return nullptr;
        }

        const auto result = Int128::parse(t.getData(), t.getData() + t.getLength(), radix, negative);

        switch (result.first) {
            default:
            case Int128::ParseResult::InvalidArgument: {
                std::abort();
                return nullptr;
            }
            case Int128::ParseResult::FormatError: {
                report->error(getVerboseTokenName(numberToken) + " is not a valid integer literal", location);
                return nullptr;
            }
            case Int128::ParseResult::RangeError: {
                report->error(getVerboseTokenName(numberToken) + " is outside the supported integer range of `" + Int128::minValue().toString() + "` .. `" + Int128::maxValue().toString() + "`", location);
                return nullptr;
            }
            case Int128::ParseResult::Success: {
                return makeFwdUnique<const Expression>(Expression::IntegerLiteral(result.second, suffix), location, Optional<ExpressionInfo>());
            }
        }
    }

    std::vector<StringView> Parser::parseQualifiedIdentifier() {
        // identifier = IDENTIFIER (`.` IDENTIFIER)*

        std::vector<StringView> pieces;
        if (checkIdentifier()) {
            pieces.push_back(token.text);
        }
        nextToken(); // IDENTIFIER

        // (`.` IDENTIFIER)*
        while (token.type == TokenType::Dot) {
            nextToken(); // `.`

            if (token.type == TokenType::Identifier && token.keyword == Keyword::None) {
                pieces.push_back(token.text);
                nextToken(); // IDENTIFIER
            } else {
                reject(token, "identifier after `.`"_sv, true);
                return {};
            }                
        }

        return pieces;
    }

    FwdUniquePtr<const TypeExpression> Parser::parseType() {
        const auto location = scanner->getLocation();

        // type = ((`[` type (`;` expr)? `]`) | (IDENTIFIER (`.` IDENTIFIER)*)) `*`* (`in` expr)?
        FwdUniquePtr<const TypeExpression> type;
        
        if (token.keyword == Keyword::Func) {
            type = parseFunctionType(false);
        } else if (token.keyword == Keyword::TypeOf) {
            nextToken(); // IDENTIFIER (keyword `typeof`)
            if (!expectTokenType(TokenType::LeftParenthesis)) { // `(`
                return nullptr;
            }

            auto expression = parseExpression();
            expectTokenType(TokenType::RightParenthesis);

            type = makeFwdUnique<const TypeExpression>(TypeExpression::TypeOf(std::move(expression)), location);            
        } else if (token.keyword == Keyword::Void) {
            nextToken(); // IDENTIFIER (keyword `void`)
            return makeFwdUnique<const TypeExpression>(TypeExpression::Tuple({}), location);
        } else if (token.keyword == Keyword::Far) {
            nextToken(); // `far`           

            if (token.keyword == Keyword::Func) {
                type = parseFunctionType(true);
            } else if (token.type == TokenType::Asterisk) {    
                type = parsePointerType(PointerQualifiers { PointerQualifier::Far });
            } else {
                reject(token, "pointer `*` type or function `func` type after `far`"_sv, true);
                return nullptr;
            }
        } else if (token.keyword == Keyword::None && token.type == TokenType::Identifier) {
            auto qualifiedIdentifier = parseQualifiedIdentifier();
            type = makeFwdUnique<const TypeExpression>(TypeExpression::Identifier(qualifiedIdentifier), location);
        } else if (token.type == TokenType::LeftParenthesis) {
            // `(` (type `,`)* `)`
            nextToken(); // `(`

            if (token.type == TokenType::RightParenthesis) {
                nextToken(); // `)`
                type = makeFwdUnique<const TypeExpression>(TypeExpression::Tuple({}), location);
            } else {
                type = parseType();  // type

                if (token.type == TokenType::Comma) {
                    nextToken(); // `,`

                    std::vector<FwdUniquePtr<const TypeExpression>> tupleTypes;
                    tupleTypes.push_back(std::move(type));
                    if (token.type != TokenType::RightParenthesis) {
                        // type (`,` type )* `)`
                        while (report->alive()) {
                            tupleTypes.push_back(parseType()); // type

                            // (`,` type)*
                            if (token.type == TokenType::Comma) {
                                nextToken(); // `,`
                                if (token.type != TokenType::RightParenthesis) {
                                    continue;
                                }                                
                            }

                            break;
                        }
                    }

                    type = makeFwdUnique<const TypeExpression>(TypeExpression::Tuple(std::move(tupleTypes)), location);
                }
                expectTokenType(TokenType::RightParenthesis); // `)`
            }
        } else if (token.type == TokenType::LeftBracket) {
            // `[` type (`;` expr)? `]`
            nextToken(); // `[`

            auto elementType = parseType();

            FwdUniquePtr<const Expression> size;
            if (token.type == TokenType::Semicolon) {
                nextToken(); // `;`
                // expr
                size = parseExpression();
            }

            expectTokenType(TokenType::RightBracket); // `]`
            type = makeFwdUnique<const TypeExpression>(TypeExpression::Array(std::move(elementType), std::move(size)), location);
        } else if (token.type == TokenType::Asterisk) {
            type = parsePointerType(PointerQualifiers {});
        } else {
            reject(token, "type name"_sv, true);
            return nullptr;
        }

        if (report->alive()) {
            if (token.keyword == Keyword::In) {
                // (`in` expr)?
                nextToken(); // IDENTIFIER (keyword `in`)
                auto holder = parseLogicalOr(ExpressionParseOptions());
                type = makeFwdUnique<const TypeExpression>(TypeExpression::DesignatedStorage(std::move(type), std::move(holder)), location);
            }
        }
        return type;
    }

    FwdUniquePtr<const TypeExpression> Parser::parseFunctionType(bool far) {
        const auto location = scanner->getLocation();

        std::vector<FwdUniquePtr<const TypeExpression>> parameterTypes;
        FwdUniquePtr<const TypeExpression> returnType;
        nextToken(); // IDENTIFIER (keyword `func`)

        // parameter_list?
        if (token.type == TokenType::LeftParenthesis) {
            nextToken(); // `(`

            bool first = true;
            // Check if we should match (`,` id)*
            while (first || token.type == TokenType::Comma) {
                if (!first) {
                    nextToken(); // ,
                }

                if (token.type == TokenType::Identifier) {
                    // named parameter = IDENTIFIER `:` type
                    if (token.keyword == Keyword::None) {
                        // A token of lookahead is needed to read the name and skip it, otherwise the identifier would be ambiguous as a type
                        peek(1);
                        if (lookaheadBuffer.size() > 0) {
                            const auto& lookahead = lookaheadBuffer.back();
                            if (lookahead.type == TokenType::Colon) {
                                nextToken(); // IDENTIFIER
                                expectTokenType(TokenType::Colon);
                            }
                        }
                    }                    

                    auto parameterType = parseType();
                    parameterTypes.push_back(std::move(parameterType));
                } else if (token.type != TokenType::RightParenthesis) {
                    if (first) {
                        reject(token, "parameter after `(` in `func` definition"_sv, true);
                    } else {
                        reject(token, "another parameter after `,` in `func` parameter list"_sv, true);
                    }
                    break;
                }

                first = false;
            }

            expectTokenType(TokenType::RightParenthesis); // `)`
        }

        // (`:` type)?
        if (token.type == TokenType::Colon) {
            nextToken();
            returnType = parseType();
        } else {
            returnType = makeFwdUnique<const TypeExpression>(TypeExpression::Tuple({}), location);
        }

        return makeFwdUnique<const TypeExpression>(TypeExpression::Function(far, std::move(parameterTypes), std::move(returnType)), location);
    }

    FwdUniquePtr<const TypeExpression> Parser::parsePointerType(PointerQualifiers qualifiers) {
        const auto location = scanner->getLocation();

        nextToken(); // `*`

        switch (token.keyword) {
            case Keyword::Const: nextToken(); qualifiers |= PointerQualifiers { PointerQualifier::Const }; break;
            case Keyword::WriteOnly: nextToken(); qualifiers |= PointerQualifiers { PointerQualifier::WriteOnly }; break;
            default: break;
        }

        auto elementType = parseType();
        return makeFwdUnique<const TypeExpression>(TypeExpression::Pointer(std::move(elementType), qualifiers), location);
    }
}


