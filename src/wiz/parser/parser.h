#ifndef WIZ_PARSER_PARSER_H
#define WIZ_PARSER_PARSER_H

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <unordered_set>

#include <wiz/ast/modifiers.h>
#include <wiz/parser/token.h>
#include <wiz/utility/string_pool.h>
#include <wiz/utility/array_view.h>
#include <wiz/utility/report.h>
#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/bit_flags.h>
#include <wiz/utility/import_options.h>

namespace wiz {
    class Reader;
    class Scanner;
    class ImportManager;

    enum class Keyword;
    enum class ImportResult;
    enum class StructKind;

    struct Statement;
    struct Expression;
    struct TypeExpression;
    
    class Parser {
        public:
            Parser(StringPool* stringPool, ImportManager* importManager, Report* report);
            ~Parser();

            FwdUniquePtr<const Statement> parse(StringView path);

        private:
            Parser(const Parser&) = delete;  
            Parser& operator=(const Parser&) = delete;

            enum class ExpressionParseOption {
                Conditional,
                Parenthesized,
                AllowStructLiterals,

                Count
            };
            using ExpressionParseOptions = BitFlags<ExpressionParseOption, ExpressionParseOption::Count>;

            void nextToken();
            void peek(std::size_t count);
            void reject(Token token, StringView expectation, bool advance, ReportErrorFlags flags = ReportErrorFlags());
            bool expectTokenType(TokenType expected);
            bool expectStatementEnd(StringView description);
            void skipToNextStatement();
            bool checkIdentifier();
            ImportResult importModule(StringView originalPath, ImportOptions importOptions, StringView& displayPath, StringView& canonicalPath, std::unique_ptr<Reader>& reader);
            void pushScanner(StringView displayPath, StringView canonicalPath, std::unique_ptr<Reader> reader);
            void popScanner();

            FwdUniquePtr<const Statement> parseFile(StringView originalPath, StringView canonicalPath, SourceLocation importLocation);
            FwdUniquePtr<const Statement> parseStatement();
            FwdUniquePtr<const Statement> parseImport();
            FwdUniquePtr<const Statement> parseAttribute();
            FwdUniquePtr<const Statement> parseInStatement();
            FwdUniquePtr<const Statement> parseBlockStatement();
            FwdUniquePtr<const Statement> parseNamespaceDeclaration();
            FwdUniquePtr<const Statement> parseConfigDirective();
            FwdUniquePtr<const Statement> parseBankDeclaration();
            FwdUniquePtr<const Statement> parseLetDeclaration();
            FwdUniquePtr<const Statement> parseEnumDeclaration();
            FwdUniquePtr<const Statement> parseStructDeclaration(StructKind structType);
            FwdUniquePtr<const Statement> parseVarDeclaration(Modifiers modifiers);
            FwdUniquePtr<const Statement> parseTypeAliasDeclaration();
            FwdUniquePtr<const Statement> parseFuncDeclaration(bool inlined, bool far);
            FwdUniquePtr<const Statement> parseDistanceHintedStatement();
            FwdUniquePtr<const Statement> parseInlineStatement();
            FwdUniquePtr<const Statement> parseFarStatement();
            FwdUniquePtr<const Statement> parseBranchStatement(std::size_t distanceHint, bool far);
            FwdUniquePtr<const Statement> parseIfStatement(std::size_t distanceHint);
            FwdUniquePtr<const Statement> parseWhileStatement(std::size_t distanceHint);
            FwdUniquePtr<const Statement> parseDoWhile(std::size_t distanceHint);
            FwdUniquePtr<const Statement> parseForStatement(std::size_t distanceHint);
            FwdUniquePtr<const Statement> parseInlineForStatement();
            FwdUniquePtr<const Statement> parseExpressionStatement();
            FwdUniquePtr<const Expression> parseExpression();
            FwdUniquePtr<const Expression> parseExpressionWithOptions(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseAssignment(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseLogicalOr(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseLogicalAnd(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseRange(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseComparison(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseBitwiseOr(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseBitwiseXor(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseBitwiseAnd(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseShift(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseAddition(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseMultiplication(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseCast(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parsePrefix(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parsePostfix(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseCallOperator(bool inlined, FwdUniquePtr<const Expression> operand);
            FwdUniquePtr<const Expression> parseInlineCall();
            FwdUniquePtr<const Expression> parseTerm(ExpressionParseOptions options);
            FwdUniquePtr<const Expression> parseIntegerLiteral(int radix);
            std::vector<StringView> parseQualifiedIdentifier();
            FwdUniquePtr<const TypeExpression> parseType();
            FwdUniquePtr<const TypeExpression> parseInnerType();
            FwdUniquePtr<const TypeExpression> parseFunctionType(bool far);
            FwdUniquePtr<const TypeExpression> parsePointerType(PointerQualifiers qualifiers);

            StringPool* stringPool;
            ImportManager* importManager;
            Report* report;
            ArrayView<StringView> importDirs;

            Token token;
            std::size_t symbolIndex;

            std::unique_ptr<Scanner> scanner;
            std::vector<Token> lookaheadBuffer;
            std::vector<std::unique_ptr<Scanner>> scannerStack;
            std::unordered_set<StringView> alreadyImportedPaths;
    };
}

#endif
