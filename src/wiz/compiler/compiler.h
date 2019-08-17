#ifndef WIZ_COMPILER_COMPILER_H
#define WIZ_COMPILER_COMPILER_H

#include <set>
#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <utility>
#include <unordered_map>

#include <wiz/compiler/instruction.h>
#include <wiz/compiler/builtins.h>
#include <wiz/utility/string_pool.h>
#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/int128.h>
#include <wiz/utility/optional.h>
#include <wiz/utility/source_location.h>
#include <wiz/utility/ptr_pool.h>
#include <wiz/utility/array_view.h>

namespace wiz {
    enum class BranchKind;
    enum class UnaryOperatorKind;
    enum class BinaryOperatorKind;
    enum class EvaluationContext;

    class Bank;
    class Config;
    class Report;
    class Platform;
    class SymbolTable;
    class ImportManager;

    struct IrNode;
    struct Attribute;
    struct Statement;
    struct Definition;
    struct Expression;
    struct TypeExpression;
    struct PlatformTestAndBranch;

    class Compiler {
        public:
            Compiler(
                FwdUniquePtr<const Statement> program,
                Platform* platform,
                StringPool* stringPool,
                Config* config,
                ImportManager* importManager,
                Report* report,
                std::unordered_map<StringView, FwdUniquePtr<const Expression>> defines);
            ~Compiler();

            bool compile();

            Report* getReport() const;
            const Statement* getProgram() const;
            ArrayView<UniquePtr<Bank>> getRegisteredBanks() const;
            const Builtins& getBuiltins() const;
            std::uint32_t getModeFlags() const;

            FwdUniquePtr<InstructionOperand> createOperandFromExpression(const Expression* expression, bool quiet) const;
        private:
            Compiler(const Compiler&) = delete;  
            Compiler& operator=(const Compiler&) = delete;

            SymbolTable* getOrCreateStatementScope(StringView name, const Statement* statement, SymbolTable* parentScope);
            SymbolTable* findStatementScope(const Statement* statement) const;
            SymbolTable* bindStatementScope(const Statement* statement, SymbolTable* scope);
            SymbolTable* findModuleScope(StringView path) const;
            SymbolTable* bindModuleScope(StringView path, SymbolTable* scope);
            void enterScope(SymbolTable* nextScope);
            void exitScope();

            struct InlineSite;
            void enterInlineSite(InlineSite* inlineSite);
            void exitInlineSite();

            bool enterLetExpression(StringView name, SourceLocation location);
            void exitLetExpression();

            Definition* createAnonymousLabelDefinition();

            void raiseUnresolvedIdentifierError(const std::vector<StringView>& pieces, std::size_t pieceIndex, SourceLocation location);
            std::pair<Definition*, std::size_t> resolveIdentifier(const std::vector<StringView>& pieces, SourceLocation location);
            FwdUniquePtr<const TypeExpression> reduceTypeExpression(const TypeExpression* typeExpression);
            FwdUniquePtr<const Expression> reduceExpression(const Expression* expression);
            Optional<std::size_t> tryGetSequenceLiteralLength(const Expression* expression) const;
            FwdUniquePtr<const Expression> getSequenceLiteralItem(const Expression* expression, std::size_t index) const;
            FwdUniquePtr<const Expression> createStringLiteralExpression(StringView data, SourceLocation location) const;
            FwdUniquePtr<const Expression> createArrayLiteralExpression(std::vector<FwdUniquePtr<const Expression>> items, const TypeExpression* elementType, SourceLocation location) const;
            std::string getResolvedIdentifierName(Definition* definition, const std::vector<StringView>& pieces) const;
            FwdUniquePtr<const Expression> resolveDefinitionExpression(Definition* definition, const std::vector<StringView>& pieces, SourceLocation location);
            FwdUniquePtr<const Expression> resolveTypeMemberExpression(const TypeExpression* typeExpression, StringView name);
            FwdUniquePtr<const Expression> resolveValueMemberExpression(const Expression* expression, StringView name);
            FwdUniquePtr<const Expression> simplifyIndirectionOffsetExpression(FwdUniquePtr<const TypeExpression> resultType, const Expression* expression, EvaluationContext context, Optional<Int128> absolutePosition, Int128 offset);
            FwdUniquePtr<const Expression> simplifyLogicalNotExpression(const Expression* expression, FwdUniquePtr<const Expression> operand);
            FwdUniquePtr<const Expression> simplifyBinaryArithmeticExpression(const Expression* expression, BinaryOperatorKind op, FwdUniquePtr<const Expression> left, FwdUniquePtr<const Expression> right);
            FwdUniquePtr<const Expression> simplifyBinaryLogicalExpression(const Expression* expression, BinaryOperatorKind op, FwdUniquePtr<const Expression> left, FwdUniquePtr<const Expression> right);
            FwdUniquePtr<const Expression> simplifyBinaryRotateExpression(const Expression* expression, BinaryOperatorKind op, FwdUniquePtr<const Expression> left, FwdUniquePtr<const Expression> right);
            FwdUniquePtr<const Expression> simplifyBinaryComparisonExpression(const Expression* expression, BinaryOperatorKind op, FwdUniquePtr<const Expression> left, FwdUniquePtr<const Expression> right);
            bool isSimpleCast(const Expression* expression) const;
            bool isTypeDefinition(const Definition* definition) const;
            Definition* tryGetResolvedIdentifierTypeDefinition(const TypeExpression* typeExpression) const;
            bool isIntegerType(const TypeExpression* typeExpression) const;
            bool isBooleanType(const TypeExpression* typeExpression) const;
            bool isEmptyTupleType(const TypeExpression* typeExpression) const;
            bool isEnumType(const TypeExpression* typeExpression) const;
            bool isPointerLikeType(const TypeExpression* typeExpression) const;
            bool isFarType(const TypeExpression* typeExpression) const;
            const TypeExpression* findCompatibleBinaryArithmeticExpressionType(const Expression* left, const Expression* right) const;
            const TypeExpression* findCompatibleConcatenationExpressionType(const Expression* left, const Expression* right) const;
            const TypeExpression* findCompatibleAssignmentType(const Expression* initializer, const TypeExpression* declarationType) const;
            bool canNarrowExpression(const Expression* sourceExpression, const TypeExpression* destinationType) const;
            bool canNarrowIntegerExpression(const Expression* expression, const Definition* integerTypeDefinition) const;
            FwdUniquePtr<const Expression> createConvertedExpression(const Expression* sourceExpression, const TypeExpression* destinationType) const;
            bool isTypeEquivalent(const TypeExpression* leftTypeExpression, const TypeExpression* rightTypeExpression) const;
            std::string getTypeName(const TypeExpression* typeExpression) const;
            Optional<std::size_t> calculateStorageSize(const TypeExpression* typeExpression, StringView description) const;
            Optional<std::size_t> resolveExplicitAddressExpression(const Expression* expression);
            bool serializeInteger(Int128 value, std::size_t size, std::vector<std::uint8_t>& result) const;
            bool serializeConstantInitializer(const Expression* expression, std::vector<std::uint8_t>& result) const;
            std::pair<bool, Optional<std::size_t>> handleInStatement(const std::vector<StringView>& bankIdentifierPieces, const Expression* dest, SourceLocation location);

            struct CompiledAttributeList;
            void pushAttributeList(CompiledAttributeList* attributeList);
            void popAttributeList();
            bool checkConditionalCompilationAttributes();

            bool reserveDefinitions(const Statement* statement);

            bool resolveDefinitionTypes();

            bool reserveStorage(const Statement* statement);
            bool resolveVariableInitializer(Definition* definition, const Expression* initializer, StringView description, SourceLocation location);
            bool reserveVariableStorage(Definition* definition, StringView description, SourceLocation location);

            FwdUniquePtr<InstructionOperand> createPlaceholderFromResolvedTypeDefinition(const Definition* resolvedTypeDefinition) const;
            FwdUniquePtr<InstructionOperand> createPlaceholderFromTypeExpression(const TypeExpression* typeExpression) const;
            FwdUniquePtr<InstructionOperand> createOperandFromResolvedIdentifier(const Expression* expression, const Definition* definition) const;
            FwdUniquePtr<InstructionOperand> createOperandFromLinkTimeExpression(const Expression* expression, bool quiet) const;
            FwdUniquePtr<InstructionOperand> createOperandFromRunTimeExpression(const Expression* expression, bool quiet) const;
            bool isLeafExpression(const Expression* expression) const;
            bool emitLoadExpressionIr(const Expression* dest, const Expression* source, SourceLocation location);
            bool emitUnaryExpressionIr(const Expression* dest, UnaryOperatorKind op, const Expression* source, SourceLocation location);
            bool emitBinaryExpressionIr(const Expression* dest, BinaryOperatorKind op, const Expression* left, const Expression* right, SourceLocation location);
            bool emitArgumentPassIr(const TypeExpression* functionTypeExpression, const std::vector<Definition*>& parameters, const std::vector<FwdUniquePtr<const Expression>>& arguments, SourceLocation location);
            bool emitCallExpressionIr(bool inlined, bool tailCall, const Expression* resultDestination, const Expression* function, const std::vector<FwdUniquePtr<const Expression>>& arguments, SourceLocation location);

            void raiseEmitLoadError(const Expression* dest, const Expression* source, SourceLocation location);
            void raiseEmitUnaryExpressionError(const Expression* dest, UnaryOperatorKind op, const Expression* source, SourceLocation location);
            void raiseEmitBinaryExpressionError(const Expression* dest, BinaryOperatorKind op, const Expression* left, const Expression* right, SourceLocation location);
            void raiseEmitIntrinsicError(const InstructionType& instructionType, const std::vector<InstructionOperandRoot>& operandRoots, SourceLocation location);

            bool emitAssignmentExpressionIr(const Expression* dest, const Expression* source, SourceLocation location);
            bool emitExpressionStatementIr(const Expression* expression, SourceLocation location);
            bool emitReturnAssignmentIr(const TypeExpression* returnType, const Expression* returnValue, SourceLocation location);

            std::unique_ptr<PlatformTestAndBranch> getTestAndBranch(BinaryOperatorKind op, const Expression* left, const Expression* right, std::size_t distanceHint) const;
            bool emitBranchIr(std::size_t distanceHint, BranchKind kind, const Expression* destination, const Expression* returnValue, bool negated, const Expression* condition, SourceLocation location);
            bool hasUnconditionalReturn(const Statement* statement) const;
            bool emitFunctionIr(Definition* definition, SourceLocation location);
            bool emitStatementIr(const Statement* statement);
            bool generateCode();

            FwdUniquePtr<const Statement> program;
            Platform* platform = nullptr;
            StringPool* stringPool = nullptr;
            Config* config = nullptr;
            ImportManager* importManager = nullptr;
            Report* report = nullptr;
            Builtins builtins;

            std::unordered_map<StringView, SymbolTable*> moduleScopes;

            PtrPool<SymbolTable> registeredScopes;
            SymbolTable* currentScope = nullptr;
            std::vector<SymbolTable*> scopeStack;

            struct ResolveIdentifierState {
                std::set<Definition*> previousResults;
                std::set<Definition*> results;
            } resolveIdentifierTempState;

            std::set<Definition*> tempImportedDefinitions;

            struct InlineSite {
                std::unordered_map<const Statement*, SymbolTable*> statementScopes;
            };

            InlineSite defaultInlineSite;
            PtrPool<InlineSite> registeredInlineSites;
            InlineSite* currentInlineSite = nullptr;
            std::vector<InlineSite*> inlineSiteStack;

            std::vector<Definition*> definitionsToResolve;

            static const std::size_t MaxLetRecursionDepth = 128;

            struct LetExpressionStackItem {
                LetExpressionStackItem(
                    StringView name,
                    SourceLocation location)
                : name(name), location(location) {}

                StringView name;
                SourceLocation location;
            };

            std::vector<LetExpressionStackItem> letExpressionStack;

            bool allowReservedConstants = false;
            std::vector<Definition*> reservedConstants;

            struct CompiledAttribute {
                CompiledAttribute(
                    const Statement* statement,
                    StringView name,
                    std::vector<FwdUniquePtr<const Expression>> arguments,
                    SourceLocation sourceLocation)
                : statement(statement),
                name(name),
                arguments(std::move(arguments)),
                sourceLocation(sourceLocation) {}

                const Statement* statement;
                StringView name;
                std::vector<FwdUniquePtr<const Expression>> arguments;
                SourceLocation sourceLocation;
            };

            struct CompiledAttributeList {
                PtrPool<CompiledAttribute> attributes;
            };

            PtrPool<CompiledAttributeList> attributeLists;
            std::unordered_map<const Statement*, CompiledAttributeList*> statementAttributeLists;
            std::vector<CompiledAttributeList*> attributeListStack;
            std::vector<CompiledAttribute*> attributeStack;
            std::uint32_t modeFlags = 0;
            std::vector<std::uint32_t> modeFlagsStack;

            Bank* currentBank = nullptr;
            std::vector<Bank*> bankStack;
            PtrPool<Bank> registeredBanks;

            Definition* currentFunction = nullptr;
            Definition* breakLabel = nullptr;
            Definition* continueLabel = nullptr;
            Definition* returnLabel = nullptr;

            std::unordered_map<StringView, StringView> embedCache;

            FwdPtrPool<Definition> definitionPool;
            FwdPtrPool<const Statement> statementPool;
            FwdPtrPool<const Expression> expressionPool;
            FwdPtrPool<IrNode> irNodes;
    };
}

#endif
