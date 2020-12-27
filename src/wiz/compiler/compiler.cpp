#include <cassert>
#include <algorithm>
#include <set>

#include <wiz/compiler/compiler.h>

#include <wiz/ast/expression.h>
#include <wiz/ast/statement.h>
#include <wiz/ast/type_expression.h>
#include <wiz/compiler/bank.h>
#include <wiz/compiler/config.h>
#include <wiz/compiler/ir_node.h>
#include <wiz/compiler/builtins.h>
#include <wiz/compiler/definition.h>
#include <wiz/compiler/symbol_table.h>
#include <wiz/compiler/operations.h>
#include <wiz/parser/token.h>
#include <wiz/utility/misc.h>
#include <wiz/utility/text.h>
#include <wiz/utility/reader.h>
#include <wiz/utility/report.h>
#include <wiz/utility/writer.h>
#include <wiz/utility/overload.h>
#include <wiz/utility/scope_guard.h>
#include <wiz/utility/import_manager.h>

namespace wiz {
    Compiler::Compiler(
        FwdUniquePtr<const Statement> program,
        Platform* platform,
        StringPool* stringPool,
        Config* config,
        ImportManager* importManager,
        Report* report,
        std::unordered_map<StringView, FwdUniquePtr<const Expression>> defines)
    : program(std::move(program)),
    platform(platform),
    stringPool(stringPool),
    config(config),
    importManager(importManager),
    report(report),
    builtins(stringPool, platform, std::move(defines)) {
        currentInlineSite = &defaultInlineSite;
    }

    Compiler::~Compiler() {}

    bool Compiler::compile() {
        return reserveDefinitions(program.get())
        && resolveDefinitionTypes()
        && reserveStorage(program.get())
        && emitStatementIr(program.get())
        && generateCode();
    }

    Report* Compiler::getReport() const {
        return report;
    }

    const Statement* Compiler::getProgram() const {
        return program.get();
    }

    std::vector<const Bank*> Compiler::getRegisteredBanks() const {
        std::vector<const Bank*> results;
        results.reserve(registeredBanks.size());

        for (const auto& bank : registeredBanks) {
            results.push_back(bank.get());
        }
        return results;
    }

    std::vector<const Definition*> Compiler::getRegisteredDefinitions() const {
        std::vector<const Definition*> results;

        results.reserve(definitionPool.size());
        for (const auto& definition : definitionPool) {
            results.push_back(definition.get());
        }

        for (const auto& scope : registeredScopes) {
            scope->getDefinitions(results);
        }

        return results;
    }

    const Builtins& Compiler::getBuiltins() const {
        return builtins;
    }

    std::uint32_t Compiler::getModeFlags() const {
        return modeFlags;
    }

    SymbolTable* Compiler::getOrCreateStatementScope(StringView name, const Statement* statement, SymbolTable* parentScope) {
        auto& statementScopes = currentInlineSite->statementScopes;
        const auto match = statementScopes.find(statement);
        if (match == statementScopes.end()) {
            const auto scope = registeredScopes.addNew(parentScope, name);
            statementScopes[statement] = scope;
            return scope;
        } else {
            return match->second;
        }
    }

    SymbolTable* Compiler::findStatementScope(const Statement* statement) const {
        return currentInlineSite->statementScopes.find(statement)->second;
    }

    SymbolTable* Compiler::bindStatementScope(const Statement* statement, SymbolTable* scope) {
        currentInlineSite->statementScopes[statement] = scope;
        return scope;
    }

    SymbolTable* Compiler::findModuleScope(StringView path) const {
        const auto scope = moduleScopes.find(path);
        if (scope != moduleScopes.end()) {
            return scope->second;
        }
        return nullptr;
    }

    SymbolTable* Compiler::bindModuleScope(StringView path, SymbolTable* scope) {
        moduleScopes[path] = scope;
        return scope;
    }

    void Compiler::enterScope(SymbolTable* nextScope) {
        scopeStack.push_back(currentScope);
        currentScope = nextScope;
    }

    void Compiler::exitScope() {
        if (scopeStack.size() > 0) {
            currentScope = scopeStack.back();
            scopeStack.pop_back();
        } else {
            currentScope = nullptr;
        }
    }

    void Compiler::enterInlineSite(InlineSite* nextInlineSite) {
        inlineSiteStack.push_back(currentInlineSite);
        currentInlineSite = nextInlineSite;
    }

    void Compiler::exitInlineSite() {
        if (inlineSiteStack.size() > 0) {
            currentInlineSite = inlineSiteStack.back();
            inlineSiteStack.pop_back();
        } else {
            currentInlineSite = &defaultInlineSite;
        }
    }

    bool Compiler::enterLetExpression(StringView name, SourceLocation location) {  
        if (letExpressionStack.size() >= MaxLetRecursionDepth) {
            report->error("stack overflow encountered during `let` expression evaluation", location, ReportErrorFlags::Continued);
            report->error("internal recursion limit is " + std::to_string(MaxLetRecursionDepth), location, ReportErrorFlags::Continued);
            report->error("stack trace:", location, ReportErrorFlags::Continued);
            for (std::size_t i = 0; i != letExpressionStack.size(); ++i) {
                const auto& item = letExpressionStack[i];
                report->log("#" + std::to_string(i + 1) + " - " + item.location.toString() + " in expression `" + item.name.toString() + "`");
            }
            report->error("stopping compilation due to previous error", location, ReportErrorFlags::Fatal);
            return false;
        }

        letExpressionStack.emplace_back(name, location);
        return true;
    }

    void Compiler::exitLetExpression() {
        letExpressionStack.pop_back();    
    }

    Definition* Compiler::createAnonymousLabelDefinition(StringView prefix) {
        const auto suffix = ++labelSuffixes[prefix];
        const auto labelId = stringPool->intern(prefix.toString() + std::to_string(suffix));
        const auto result = definitionPool.addNew(Definition::Func(true, false, false, BranchKind::None, builtins.getUnitTuple(), currentScope, nullptr), labelId, nullptr);
        auto& func = result->variant.get<Definition::Func>();
        func.resolvedSignatureType = makeFwdUnique<const TypeExpression>(TypeExpression::Function(false, {}, func.returnTypeExpression->clone()), func.returnTypeExpression->location);
        return result;
    }

    void Compiler::raiseUnresolvedIdentifierError(const std::vector<StringView>& pieces, std::size_t pieceIndex, SourceLocation location) {
        report->error(
            "could not resolve identifier `" + text::join(pieces.begin(), pieces.begin() + pieceIndex + 1, ".") + "`"
            + (pieceIndex < pieces.size() - 1
                ? " (of `" + text::join(pieces.begin(), pieces.end(), ".") + "`)"
                : ""),
            location);
    }
    
    std::pair<Definition*, std::size_t> Compiler::resolveIdentifier(const std::vector<StringView>& pieces, SourceLocation location) {
        if (pieces.empty()) {
            raiseUnresolvedIdentifierError(pieces, 0, location);
            return {nullptr, 0};
        }

        auto& previousResults = resolveIdentifierTempState.previousResults;
        auto& results = resolveIdentifierTempState.results;
        previousResults.clear();
        results.clear();

        std::size_t pieceIndex;
        for (pieceIndex = 0; pieceIndex != pieces.size(); ++pieceIndex) {
            const auto piece = pieces[pieceIndex];

            if (previousResults.empty()) {
                currentScope->findUnqualifiedDefinitions(piece, results);
            } else {
                for (const auto definition : previousResults) {
                    if (const auto ns = definition->variant.tryGet<Definition::Namespace>()) {
                        ns->environment->findMemberDefinitions(piece, results);
                    }
                }
            }

            if (results.size() == 0) {
                break;
            }

            const auto firstMatch = *results.begin();

            if (pieceIndex == pieces.size() - 1 || !firstMatch->variant.is<Definition::Namespace>()) {
                if (results.size() == 1) {
                    return {firstMatch, pieceIndex};
                } else {
                    const auto partiallyQualifiedName = text::join(pieces.begin(), pieces.begin() + pieceIndex + 1, ".");

                    report->error(
                        "identifier `" + partiallyQualifiedName + "`"
                        + (pieceIndex < pieces.size() - 1
                            ? " (of `" + text::join(pieces.begin(), pieces.end(), ".") + "`)"
                            : "")
                        + " is ambiguous",
                        location,
                        ReportErrorFlags::Continued);

                    for (const auto result : results) {
                        report->error("`" + partiallyQualifiedName + "` is defined here, by " + result->declaration->getDescription().toString(), result->declaration->location, ReportErrorFlags::Continued);
                    }

                    report->error("identifier must be manually disambiguated\n", location);
                    return {nullptr, pieceIndex};
                }
            }

            previousResults.swap(results);
            results.clear();
        }

        raiseUnresolvedIdentifierError(pieces, pieceIndex, location);
        return {nullptr, pieceIndex};
    }

    FwdUniquePtr<const TypeExpression> Compiler::reduceTypeExpression(const TypeExpression* typeExpression) {
        switch (typeExpression->kind) {
            case TypeExpressionKind::Array: {
                const auto& arrayType = typeExpression->array;
                auto reducedElementType = reduceTypeExpression(arrayType.elementType.get());
                auto reducedSize = arrayType.size != nullptr ? reduceExpression(arrayType.size.get()) : nullptr;
                if (reducedElementType != nullptr && (arrayType.size == nullptr || reducedSize != nullptr)) {
                    if (reducedSize != nullptr) {
                        if (const auto sizeLiteral = reducedSize->tryGet<Expression::IntegerLiteral>()) {
                            if (sizeLiteral->value.isNegative()) {
                                report->error("array size must be a non-negative integer, but got size of `" + sizeLiteral->value.toString() + "` instead", reducedSize->location);
                                return nullptr;
                            }
                        } else {
                            report->error("array size must be a compile-time integer literal", reducedSize->location);
                            return nullptr;
                        }
                    }
                    return makeFwdUnique<const TypeExpression>(TypeExpression::Array(std::move(reducedElementType), std::move(reducedSize)), typeExpression->location);
                } else {
                    return nullptr;
                }
            }
            case TypeExpressionKind::DesignatedStorage: {
                const auto& designatedStorageType = typeExpression->designatedStorage;
                auto reducedElementType = reduceTypeExpression(designatedStorageType.elementType.get());
                auto reducedHolder = reduceExpression(designatedStorageType.holder.get());

                if (reducedElementType != nullptr && reducedHolder != nullptr) {
                    const auto elementSize = calculateStorageSize(reducedElementType.get(), "designated storage element type"_sv);
                    const auto holderSize = calculateStorageSize(reducedHolder->info->type.get(), "designated storage holder"_sv);

                    if (!elementSize.hasValue() || !holderSize.hasValue() || *elementSize != *holderSize) {
                        report->error("holder expression of type `"
                            + getTypeName(reducedHolder->info->type.get())
                            + "` is not compatible with element type `"
                            + getTypeName(reducedElementType.get())
                            + "` of designated storage type `"
							+ getTypeName(typeExpression) + "`",
                            reducedHolder->location);
                        return nullptr;
                    }

                    if ((reducedHolder->info->qualifiers & Qualifiers::LValue) == Qualifiers::None) {
                        report->error("holder for designated storage type must be valid L-value", reducedHolder->location);
                        return nullptr;
                    }
                    if ((reducedHolder->info->qualifiers & Qualifiers::Const) != Qualifiers::None) {
                        report->error("holder for designated storage type cannot be `const`", reducedHolder->location);
                        return nullptr;
                    }
                    if ((reducedHolder->info->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) {
                        report->error("holder for designated storage type cannot be `writeonly`", reducedHolder->location);
                        return nullptr;
                    }

                    return makeFwdUnique<const TypeExpression>(TypeExpression::DesignatedStorage(std::move(reducedElementType), std::move(reducedHolder)), typeExpression->location);
                }
                return nullptr;
            }
            case TypeExpressionKind::Function: {
                const auto& funcType = typeExpression->function;
                auto reducedReturnType = reduceTypeExpression(funcType.returnType.get());
                if (reducedReturnType == nullptr) {
                    return nullptr;
                }

                std::vector<FwdUniquePtr<const TypeExpression>> reducedParameterTypes;
                reducedParameterTypes.reserve(funcType.parameterTypes.size());
                for (const auto& parameterType : funcType.parameterTypes) {
                    auto reducedParameterType = reduceTypeExpression(parameterType.get());
                    if (reducedParameterType == nullptr) {
                        return nullptr;
                    }                    
                    reducedParameterTypes.push_back(std::move(reducedParameterType));
                }
                return makeFwdUnique<const TypeExpression>(TypeExpression::Function(funcType.far, std::move(reducedParameterTypes), std::move(reducedReturnType)), typeExpression->location);
            }
            case TypeExpressionKind::Identifier: {
                const auto& identifierType = typeExpression->identifier;
                const auto& pieces = identifierType.pieces;
                const auto resolveResult = resolveIdentifier(pieces, typeExpression->location);

                if (resolveResult.first == nullptr) {
                    return nullptr;
                }
                if (resolveResult.second < pieces.size() - 1) {
                    raiseUnresolvedIdentifierError(pieces, resolveResult.second, typeExpression->location);
                    return nullptr;
                }

                const auto definition = resolveResult.first;

                if (isTypeDefinition(definition)) {
                    return makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(definition, pieces), typeExpression->location); 
                } else if (const auto typeAlias = definition->variant.tryGet<Definition::TypeAlias>()) {                  
                    if (typeAlias->resolvedType == nullptr) {
                        report->error("encountered a reference to typealias `" + text::join(pieces.begin(), pieces.end(), ".") + "` before its underlying type was known", typeExpression->location);
                        return nullptr;
                    } else {
                        return typeAlias->resolvedType->clone(); 
                    }
                } else {
                    report->error("`" + text::join(pieces.begin(), pieces.end(), ".") + "` is not a valid type", typeExpression->location);
                    return nullptr;
                }
            }
            case TypeExpressionKind::Pointer: {
                const auto& pointerType = typeExpression->pointer;
                auto reducedElementType = reduceTypeExpression(pointerType.elementType.get());
                if (reducedElementType != nullptr) { 
                    return makeFwdUnique<const TypeExpression>(TypeExpression::Pointer(std::move(reducedElementType), pointerType.qualifiers), typeExpression->location);
                } else {
                    return nullptr;
                }
            }
            case TypeExpressionKind::ResolvedIdentifier: {
                const auto& resolvedIdentifier = typeExpression->resolvedIdentifier;
                return makeFwdUnique<const TypeExpression>(resolvedIdentifier, typeExpression->location);
            }
            case TypeExpressionKind::Tuple: {
                const auto& tupleType = typeExpression->tuple;
                std::vector<FwdUniquePtr<const TypeExpression>> reducedElementTypes;
                reducedElementTypes.reserve(tupleType.elementTypes.size());
                for (const auto& elementType : tupleType.elementTypes) {
                    auto reducedElementType = reduceTypeExpression(elementType.get());
                    if (reducedElementType == nullptr) {
                        return nullptr;
                    }                    
                    reducedElementTypes.push_back(std::move(reducedElementType));
                }
                return makeFwdUnique<const TypeExpression>(TypeExpression::Tuple(std::move(reducedElementTypes)), typeExpression->location);            
            }
            case TypeExpressionKind::TypeOf: {
                const auto& typeOf = typeExpression->typeOf;
                auto reducedExpression = reduceExpression(typeOf.expression.get());    
                if (reducedExpression == nullptr) {
                    return nullptr;
                }
                return reducedExpression->info->type->clone();
            }
            default: std::abort(); return nullptr;
        }
    }

    FwdUniquePtr<const Expression> Compiler::reduceExpression(const Expression* expression) {
        switch (expression->kind) {
            case ExpressionKind::ArrayComprehension: {
                const auto& arrayComprehension = expression->arrayComprehension;
                auto reducedSequence = reduceExpression(arrayComprehension.sequence.get());
                if (reducedSequence == nullptr) {
                    return nullptr;
                }

                const auto length = tryGetSequenceLiteralLength(reducedSequence.get());
                if (!length.hasValue()) {
                    report->error("source for array comprehension must be a valid compile-time sequence", expression->location);
                    return nullptr;
                }

                std::vector<FwdUniquePtr<const Expression>> computedItems;
                computedItems.reserve(*length);

                auto scope = std::make_unique<SymbolTable>(currentScope, StringView());
                auto tempDeclaration = statementPool.addNew(Statement::InternalDeclaration(), expression->location);
                auto tempDefinition = scope->createDefinition(report, Definition::Let({}, nullptr), arrayComprehension.name, tempDeclaration);
                auto& tempLetDefinition = tempDefinition->variant.get<Definition::Let>();

                const TypeExpression* elementType = nullptr;

                for (std::size_t i = 0; i != *length; ++i) {
                    auto sourceItem = getSequenceLiteralItem(reducedSequence.get(), i);
                    tempLetDefinition.expression = sourceItem.get();

                    enterScope(scope.get());
                    auto computedItem = reduceExpression(arrayComprehension.expression.get());
                    exitScope();

                    if (computedItem != nullptr) {
                        if (elementType == nullptr) {
                            elementType = computedItem->info->type.get();
                        } else if (!isTypeEquivalent(computedItem->info->type.get(), elementType)) {
                            if (!canNarrowExpression(computedItem.get(), elementType)) {
                                report->error("array element of type `" + getTypeName(computedItem->info->type.get()) + "` at iteration " + std::to_string(i) + " does not match first element type `" + getTypeName(elementType) + "` in comprehension", computedItem->location);
                                return nullptr;
                            }

                            computedItem = createConvertedExpression(computedItem.get(), elementType);
                        }

                        computedItems.push_back(std::move(computedItem));
                    } else {
                        return nullptr;
                    }
                }

                return createArrayLiteralExpression(std::move(computedItems), elementType, expression->location);
            }
            case ExpressionKind::ArrayPadLiteral: {
                const auto& arrayPadLiteral = expression->arrayPadLiteral;
                auto reducedValueExpression = reduceExpression(arrayPadLiteral.valueExpression.get());
                auto reducedSizeExpression = reduceExpression(arrayPadLiteral.sizeExpression.get());

                if (reducedValueExpression == nullptr || reducedSizeExpression == nullptr) {
                    return nullptr;
                }

                const auto reducedSizeLiteral = reducedSizeExpression->tryGet<Expression::IntegerLiteral>();
                if (reducedSizeLiteral == nullptr) {
                    report->error("array pad size must be a compile-time integer literal", expression->location);
                    return nullptr;
                }
                if (reducedSizeLiteral->value >= Int128(SIZE_MAX)) {
                    report->error("array pad size of `" + reducedSizeLiteral->value.toString() + "` is too big.", expression->location);
                    return nullptr;
                }

                const auto length = static_cast<std::size_t>(reducedSizeLiteral->value);
                std::vector<FwdUniquePtr<const Expression>> items;
                items.reserve(length);

                const TypeExpression* elementType = reducedValueExpression->info->type.get();
                for (std::size_t i = 0; i != length; ++i) {
                    if (i == length - 1) {
                        items.push_back(std::move(reducedValueExpression));
                    } else {
                        items.push_back(reducedValueExpression->clone());
                    }
                }

                return createArrayLiteralExpression(std::move(items), elementType, expression->location);
            }
            case ExpressionKind::ArrayLiteral: {
                const auto& arrayLiteral = expression->arrayLiteral;
                const auto& items = arrayLiteral.items;

                std::vector<FwdUniquePtr<const Expression>> reducedItems;
                reducedItems.reserve(items.size());

                const TypeExpression* elementType = nullptr;

                for (std::size_t i = 0; i != items.size(); ++i) {
                    const auto& item = items[i];
                    auto reducedItem = reduceExpression(item.get());

                    if (reducedItem != nullptr) {
                        if (elementType == nullptr) {
                            elementType = reducedItem->info->type.get();
                        } else if (!isTypeEquivalent(reducedItem->info->type.get(), elementType)) {
                            if (!canNarrowExpression(reducedItem.get(), elementType)) {
                                report->error("array element of type `" + getTypeName(reducedItem->info->type.get()) + "` at index " + std::to_string(i) + " does not match first element type `" + getTypeName(elementType) + "`", reducedItem->location);
                                return nullptr;
                            }

                            reducedItem = createConvertedExpression(reducedItem.get(), elementType);
                        }

                        reducedItems.push_back(std::move(reducedItem));
                    } else {
                        return nullptr;
                    }
                }

                return createArrayLiteralExpression(std::move(reducedItems), elementType, expression->location);
            }
            case ExpressionKind::BinaryOperator: {
                const auto& binaryOperator = expression->binaryOperator;
                const auto op = binaryOperator.op;
                auto left = reduceExpression(binaryOperator.left.get());
                auto right = reduceExpression(binaryOperator.right.get());
                if (!left || !right) {
                    return nullptr;
                }

                if (op == BinaryOperatorKind::Indexing || op == BinaryOperatorKind::BitIndexing) {
                    if ((right->info->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) {
                        report->error("subscript of " + getBinaryOperatorName(op).toString() + " cannot be `writeonly`", right->location);
                        return nullptr;
                    }
                } else if (op == BinaryOperatorKind::Assignment) {
                    if ((right->info->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) {
                        report->error("right-hand side of assignment `=` cannot be `writeonly`", right->location);
                        return nullptr;
                    }
                } else {
                    if ((left->info->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) {
                        report->error("left operand to " + getBinaryOperatorName(op).toString() + " cannot be `writeonly`", expression->location);                                               
                        return nullptr;
                    } else if ((right->info->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) {
                        report->error("right operand to " + getBinaryOperatorName(op).toString() + " cannot be `writeonly`", expression->location);
                        return nullptr;
                    }
                }

                // attempt to reduce expression if possible, and return expression if typed.
                switch (op) {
                    default: case BinaryOperatorKind::None: std::abort(); return nullptr;

                    // Run-time assignment. (T, T) -> T (returns left-hand side lvalue)
                    case BinaryOperatorKind::Assignment: {
                        if ((left->info->qualifiers & Qualifiers::LValue) == Qualifiers::None) {
                            report->error("left-hand side of assignment `=` must be valid L-value", expression->location);
                            return nullptr;
                        }
                        if ((left->info->qualifiers & Qualifiers::Const) != Qualifiers::None) {
                            report->error("left-hand side of assignment `=` cannot be `const`", expression->location);
                            return nullptr;
                        }

                        if (const auto resultType = findCompatibleAssignmentType(right.get(), left->info->type.get())) {
                            const auto qualifiers = left->info->qualifiers;
                            return makeFwdUnique<const Expression>(
                                Expression::BinaryOperator(op, std::move(left), createConvertedExpression(right.get(), resultType)), expression->location,
                                ExpressionInfo(EvaluationContext::RunTime, resultType->clone(), qualifiers));
                        }

                        report->error("left-hand side of type `" + getTypeName(left->info->type.get()) + "` cannot be assigned `" + getTypeName(right->info->type.get()) + "` expression", expression->location);
                        return nullptr;
                    }

                    // Run-time arithmetic. (integer, integer) -> integer
                    case BinaryOperatorKind::AdditionWithCarry:
                    case BinaryOperatorKind::SubtractionWithCarry:
                    case BinaryOperatorKind::LeftRotateWithCarry:
                    case BinaryOperatorKind::RightRotateWithCarry: {
                        if (const auto resultType = findCompatibleBinaryArithmeticExpressionType(left.get(), right.get())) {
                            return makeFwdUnique<const Expression>(
                                Expression::BinaryOperator(op, std::move(left), std::move(right)),
                                expression->location,
                                ExpressionInfo(EvaluationContext::RunTime, resultType->clone(), Qualifiers::None));
                        } else {
                            report->error(getBinaryOperatorName(op).toString() + " is not defined between provided operand types `" + getTypeName(left->info->type.get()) + "` and `" + getTypeName(right->info->type.get()) + "`", expression->location);
                            return nullptr;
                        }
                    }
    
                    // Arithmetic.
                    // (integer, integer) -> integer
                    // (bool, bool) -> bool
                    case BinaryOperatorKind::Addition:
                    case BinaryOperatorKind::BitwiseAnd:
                    case BinaryOperatorKind::BitwiseOr:
                    case BinaryOperatorKind::BitwiseXor:
                    case BinaryOperatorKind::Division:
                    case BinaryOperatorKind::Modulo:
                    case BinaryOperatorKind::Multiplication:
                    case BinaryOperatorKind::LeftShift:
                    case BinaryOperatorKind::RightShift:
                    case BinaryOperatorKind::Subtraction:
                    case BinaryOperatorKind::LogicalLeftShift:
                    case BinaryOperatorKind::LogicalRightShift: {
                        if (isBooleanType(left->info->type.get()) && isBooleanType(right->info->type.get())) {
                            return simplifyBinaryLogicalExpression(expression, op, std::move(left), std::move(right));
                        }

                        return simplifyBinaryArithmeticExpression(expression, op, std::move(left), std::move(right));
                    }

                    // Array concatenation. ([T; m], [T; n]) -> [T; m + n]
                    case BinaryOperatorKind::Concatenation: {
                        if (const auto resultType = findCompatibleConcatenationExpressionType(left.get(), right.get())) {
                            bool isLeftArray = left->kind == ExpressionKind::ArrayLiteral;
                            bool isLeftString = left->kind == ExpressionKind::StringLiteral;
                            bool isRightArray = right->kind == ExpressionKind::ArrayLiteral;
                            bool isRightString = right->kind == ExpressionKind::StringLiteral;

                            // NOTE: Assumes if compatible type was found, it must be [u8], because string literals are [u8]. Hopefully this is always true!
                            if (isLeftString && isRightArray) {
                                const auto leftValue = left->stringLiteral.value;
                                const auto leftLength = leftValue.getLength();
                                const auto& rightItems = right->arrayLiteral.items;
                                const auto rightLength = rightItems.size();

                                std::string result(leftLength + rightLength, 0);
                                for (std::size_t i = 0; i != leftLength; ++i) {
                                    result[i] = leftValue.getData()[i];
                                }
                                for (std::size_t i = 0; i != rightLength; ++i) {
                                    const auto& rightItem = rightItems[i];
                                    result[leftLength + i] = static_cast<std::uint8_t>(rightItem->integerLiteral.value);
                                }

                                return createStringLiteralExpression(stringPool->intern(result), expression->location);
                            } else if (isLeftArray && isRightString) {
                                const auto& leftItems = left->arrayLiteral.items;
                                const auto leftLength = leftItems.size();
                                const auto rightValue = right->stringLiteral.value;
                                const auto rightLength = rightValue.getLength();

                                std::string result(leftLength + rightLength, 0);
                                for (std::size_t i = 0; i != leftLength; ++i) {
                                    const auto& leftItem = leftItems[i];
                                    result[i] = static_cast<std::uint8_t>(leftItem->integerLiteral.value);
                                }
                                for (std::size_t i = 0; i != rightLength; ++i) {
                                    result[leftLength + i] = rightValue.getData()[i];
                                }

                                return createStringLiteralExpression(stringPool->intern(result), expression->location);
                            }
                            
                            if (isLeftString && isRightString) {
                                const auto result = stringPool->intern(
                                    left->stringLiteral.value.toString()
                                    + right->stringLiteral.value.toString());

                                return createStringLiteralExpression(result, expression->location);
                            } else if (isLeftArray && isRightArray) {
                                const auto& leftItems = left->arrayLiteral.items;
                                const auto& rightItems = right->arrayLiteral.items;
                                const auto elementType = resultType->array.elementType.get();

                                std::vector<FwdUniquePtr<const Expression>> reducedItems;
                                reducedItems.reserve(leftItems.size() + rightItems.size());

                                for (const auto& item : leftItems) {
                                    reducedItems.push_back(createConvertedExpression(item.get(), elementType));
                                }
                                for (const auto& item : rightItems) {
                                    reducedItems.push_back(createConvertedExpression(item.get(), elementType));
                                }

                                return createArrayLiteralExpression(std::move(reducedItems), elementType, expression->location);
                            } else {
                                report->error(getBinaryOperatorName(op).toString() + " is only defined between compile-time array literals", expression->location);
                                return nullptr;
                            }
                        }
                        
                        report->error(getBinaryOperatorName(op).toString() + " is not defined between provided operand types `" + getTypeName(left->info->type.get()) + "` and `" + getTypeName(right->info->type.get()) + "`", expression->location);
                        return nullptr;
                    }

                    // Fixed bit-width arithmetic. (integer, integer) -> integer, where integer bit count is bounded.
                    case BinaryOperatorKind::LeftRotate:
                    case BinaryOperatorKind::RightRotate:{
                         return simplifyBinaryRotateExpression(expression, op, std::move(left), std::move(right));
                    }

                    // Indexing.
                    // ([T; n], integer i) -> T
                    // ((T_1, T_2, ... T_n), integer i) -> T_i
                    case BinaryOperatorKind::Indexing: {
                        if (isIntegerType(right->info->type.get())) {
                            const auto qualifiers = left->info->qualifiers & (Qualifiers::LValue | Qualifiers::Const | Qualifiers::WriteOnly | Qualifiers::Far);

                            if (left->info->type->kind == TypeExpressionKind::Array) {
                                if (right->kind == ExpressionKind::IntegerLiteral) {
                                    const auto indexValue = right->integerLiteral.value;

                                    if (left->kind == ExpressionKind::ArrayLiteral) {
                                        const auto& items = left->arrayLiteral.items;

                                        if (indexValue.isNegative()) {
                                            report->error("indexing by negative integer `" + indexValue.toString() + "`", expression->location);
                                            return nullptr;
                                        }
                                        if (indexValue >= Int128(items.size())) {
                                            report->error("indexing by `" + indexValue.toString() + "` exceeds array length of `" + std::to_string(items.size()) + "`", expression->location);
                                            return nullptr;
                                        }

                                        std::size_t index = static_cast<std::size_t>(indexValue);
                                        return items[index]->clone();
                                    } else if (left->kind == ExpressionKind::StringLiteral) {
                                        const auto stringLiteral = left->stringLiteral.value;

                                        if (indexValue.isNegative()) {
                                            report->error("indexing by negative integer `" + indexValue.toString() + "`", expression->location);
                                            return nullptr;
                                        }
                                        if (indexValue >= Int128(stringLiteral.getLength())) {
                                            report->error("indexing by `" + indexValue.toString() + "` exceeds array length of `" + std::to_string(stringLiteral.getLength()) + "`", expression->location);
                                            return nullptr;
                                        }

                                        std::size_t index = static_cast<std::size_t>(indexValue);
                                        return makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(static_cast<std::uint8_t>(stringLiteral[index]))), expression->location,
                                            ExpressionInfo(EvaluationContext::CompileTime,
                                                makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), expression->location),
                                                Qualifiers::None));
                                    } else if (const auto resolvedIdentifier = left->tryGet<Expression::ResolvedIdentifier>()) {
                                        if (const auto varDefinition = resolvedIdentifier->definition->variant.tryGet<Definition::Var>()) {
                                            if (varDefinition->address.hasValue() && varDefinition->address->absolutePosition.hasValue()) {
                                                auto resultType = left->info->type->array.elementType->clone();
                                                auto addressType = makeFwdUnique<const TypeExpression>(
                                                    TypeExpression::Pointer(
                                                        left->info->type->array.elementType->clone(),
                                                        left->info->qualifiers & (Qualifiers::Const | Qualifiers::WriteOnly | Qualifiers::Far)),
                                                    resultType->location);
                                                const auto pointerSizedType = (left->info->qualifiers & Qualifiers::Far) != Qualifiers::None ? platform->getFarPointerSizedType() : platform->getPointerSizedType();
                                                const auto mask = Int128((1U << (8U * pointerSizedType->variant.get<Definition::BuiltinIntegerType>().size)) - 1);

                                                if (const auto elementSize = calculateStorageSize(resultType.get(), "operand"_sv)) {
                                                    return makeFwdUnique<const Expression>(
                                                        Expression::UnaryOperator(
                                                            UnaryOperatorKind::Indirection,
                                                            makeFwdUnique<const Expression>(
                                                                Expression::IntegerLiteral((Int128(varDefinition->address->absolutePosition.get()) + indexValue * Int128(*elementSize)) & mask), expression->location,
                                                                ExpressionInfo(EvaluationContext::CompileTime, std::move(addressType), Qualifiers::None))),
                                                        expression->location,
                                                        ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), qualifiers));
                                                }
                                            }
                                        }
                                    } else if (const auto addressLiteral = left->tryGet<Expression::IntegerLiteral>()) {
                                        auto resultType = left->info->type->array.elementType->clone();
                                        auto addressType = makeFwdUnique<const TypeExpression>(
                                            TypeExpression::Pointer(
                                                left->info->type->array.elementType->clone(),
                                                left->info->qualifiers & (Qualifiers::Const | Qualifiers::WriteOnly | Qualifiers::Far)),
                                            resultType->location);
                                        const auto pointerSizedType = (left->info->qualifiers & Qualifiers::Far) != Qualifiers::None ? platform->getFarPointerSizedType() : platform->getPointerSizedType();
                                        const auto mask = Int128((1U << (8U * pointerSizedType->variant.get<Definition::BuiltinIntegerType>().size)) - 1);

                                        if (const auto elementSize = calculateStorageSize(resultType.get(), "operand"_sv)) {
                                            return makeFwdUnique<const Expression>(
                                                Expression::UnaryOperator(
                                                    UnaryOperatorKind::Indirection,
                                                    makeFwdUnique<const Expression>(
                                                        Expression::IntegerLiteral((addressLiteral->value + indexValue * Int128(*elementSize)) & mask), expression->location,
                                                        ExpressionInfo(EvaluationContext::CompileTime, std::move(addressType), Qualifiers::None))),
                                                expression->location,
                                                ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), qualifiers));
                                        }
                                    }
                                }

                                if (left->info->type->array.elementType == nullptr) {
                                    report->error("array has unknown element type", expression->location);
                                    return nullptr;
                                }

                                auto resultType = left->info->type->array.elementType->clone();

                                return makeFwdUnique<const Expression>(
                                    Expression::BinaryOperator(op, std::move(left), std::move(right)),
                                    expression->location,
                                    ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), qualifiers));                            
                            } else if (left->info->type->kind == TypeExpressionKind::Tuple) {
                                if (left->kind == ExpressionKind::TupleLiteral && right->kind == ExpressionKind::IntegerLiteral) {
                                    const auto& items = left->tupleLiteral.items;
                                    const auto indexValue = right->integerLiteral.value;

                                    if (indexValue.isNegative()) {
                                        report->error("indexing by negative integer `" + indexValue.toString() + "`", expression->location);
                                        return nullptr;
                                    }
                                    if (indexValue >= Int128(items.size())) {
                                        report->error("indexing by `" + indexValue.toString() + "` exceeds tuple length of `" + std::to_string(items.size()) + "`", expression->location);
                                        return nullptr;
                                    }

                                    std::size_t index = static_cast<std::size_t>(indexValue);
                                    return items[index]->clone();
                                }

                                report->error("tuple index must be a compile-time integer literal", expression->location);
                                return nullptr;
                            } else if (const auto pointerType = left->info->type->tryGet<TypeExpression::Pointer>()) {
                                const auto qualifiers = Qualifiers::LValue | (pointerType->qualifiers & (Qualifiers::Const | Qualifiers::WriteOnly | Qualifiers::Far));
                                auto resultType = pointerType->elementType->clone();

                                return makeFwdUnique<const Expression>(
                                    Expression::BinaryOperator(op, std::move(left), std::move(right)),
                                    expression->location,
                                    ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), qualifiers));
                            } else if (const auto typeDefinition = tryGetResolvedIdentifierTypeDefinition(left->info->type.get())) {
                                if (typeDefinition->variant.is<Definition::BuiltinRangeType>()) {
                                    if (const auto length = tryGetSequenceLiteralLength(left.get())) {
                                        if (const auto indexLiteral = right->tryGet<Expression::IntegerLiteral>()) {
                                            const auto indexValue = indexLiteral->value;
                                            if (indexValue.isNegative()) {
                                                report->error("indexing by negative integer `" + indexValue.toString() + "`", expression->location);
                                                return nullptr;
                                            }
                                            if (indexValue >= Int128(*length)) {
                                                report->error("indexing by `" + indexValue.toString() + "` exceeds range length of `" + std::to_string(*length) + "`", expression->location);
                                                return nullptr;
                                            }
                                            return getSequenceLiteralItem(left.get(), static_cast<std::size_t>(indexValue));
                                        } else {
                                            report->error("range index must be a compile-time integer literal", expression->location);
                                            return nullptr;
                                        }
                                    } else {
                                        report->error("range must known at compile-time", expression->location);
                                        return nullptr;
                                    }
                                }
                            }
                        }
                           
                        report->error(getBinaryOperatorName(op).toString() + " is not defined between provided operand types `" + getTypeName(left->info->type.get()) + "` and `" + getTypeName(right->info->type.get()) + "`", expression->location);
                        return nullptr;
                    }

                    // Bit Indexing. (integer, integer) -> bool
                    case BinaryOperatorKind::BitIndexing: {
                        if (const auto compatibleType = findCompatibleBinaryArithmeticExpressionType(left.get(), right.get())) {
                            static_cast<void>(compatibleType);

                            const auto leftContext = left->info->context;
                            const auto rightContext = right->info->context;

                            auto resultType = makeFwdUnique<const TypeExpression>(
                                TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)),
                                expression->location);

                            if (leftContext == EvaluationContext::RunTime || rightContext == EvaluationContext::RunTime) {
                                const auto qualifiers = left->info->qualifiers;
                                return makeFwdUnique<const Expression>(
                                    Expression::BinaryOperator(op, std::move(left), std::move(right)),
                                    expression->location,
                                    ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), qualifiers));
                            } else if (leftContext == EvaluationContext::LinkTime || rightContext == EvaluationContext::LinkTime) {
                                const auto qualifiers = left->info->qualifiers;
                                return makeFwdUnique<const Expression>(
                                    Expression::BinaryOperator(op, std::move(left), std::move(right)),
                                    expression->location,
                                    ExpressionInfo(EvaluationContext::LinkTime, std::move(resultType), qualifiers));
                            } else {
                                const auto leftValue = left->integerLiteral.value;
                                const auto rightValue = right->integerLiteral.value;
                                std::size_t bits = rightValue > Int128(SIZE_MAX) ? SIZE_MAX : static_cast<std::size_t>(rightValue);
                                return makeFwdUnique<const Expression>(
                                    Expression::BooleanLiteral(leftValue.getBit(bits)),
                                    expression->location,
                                    ExpressionInfo(EvaluationContext::CompileTime, std::move(resultType), Qualifiers::None));
                            }
                        }
                           
                        report->error(getBinaryOperatorName(op).toString() + " is not defined between provided operand types `" + getTypeName(left->info->type.get()) + "` and `" + getTypeName(right->info->type.get()) + "`", expression->location);
                        return nullptr;
                    }

                    // Comparisons. (T, T) -> bool
                    case BinaryOperatorKind::Equal:
                    case BinaryOperatorKind::GreaterThan:
                    case BinaryOperatorKind::GreaterThanOrEqual:
                    case BinaryOperatorKind::NotEqual:
                    case BinaryOperatorKind::LessThan:
                    case BinaryOperatorKind::LessThanOrEqual: {
                        return simplifyBinaryComparisonExpression(expression, op, std::move(left), std::move(right));
                    }

                    // Logical operators.
                    // (bool, bool) -> bool                    
                    case BinaryOperatorKind::LogicalAnd:
                    case BinaryOperatorKind::LogicalOr: {
                        return simplifyBinaryLogicalExpression(expression, op, std::move(left), std::move(right));
                    }
                }
            }
            case ExpressionKind::BooleanLiteral: {
                const auto& booleanLiteral = expression->booleanLiteral;
                return makeFwdUnique<const Expression>(Expression::BooleanLiteral(booleanLiteral.value), expression->location,
                    ExpressionInfo(EvaluationContext::CompileTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                        Qualifiers::None));
            }
            case ExpressionKind::Call: {
                const auto& call = expression->call;
                auto function = reduceExpression(call.function.get());
                if (!function) {
                    return nullptr;
                }

                if ((function->info->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) {
                    report->error("operand of function call cannot be `writeonly`", function->location);
                    return nullptr;
                }

                std::vector<FwdUniquePtr<const Expression>> reducedArguments;
                reducedArguments.reserve(call.arguments.size());

                auto& callArguments = call.arguments;
                for (std::size_t i = 0; i != callArguments.size(); ++i) {
                    auto& argument = callArguments[i];
                    if (!argument) {
                        return nullptr;
                    }

                    auto reducedArgument = reduceExpression(argument.get());
                    if (!reducedArgument) {
                        return nullptr;
                    }

                    if ((reducedArgument->info->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) {
                        report->error("argument #" + std::to_string(i) + " of function call cannot be `writeonly`", reducedArgument->location);
                        return nullptr;
                    }

                    reducedArguments.push_back(std::move(reducedArgument));
                }

                if (const auto resolvedIdentifier = function->tryGet<Expression::ResolvedIdentifier>()) {
                    const auto definition = resolvedIdentifier->definition;
                    if (const auto letDefinition = definition->variant.tryGet<Definition::Let>()) {
                        if (call.inlined) {
                            report->error("`inline` keyword cannot be applied to a `let` function call.", expression->location);
                            return nullptr;
                        }

                        const auto& parameters = letDefinition->parameters;
                        if (reducedArguments.size() != parameters.size()) {
                            auto expected = letDefinition->parameters.size();
                            auto got = reducedArguments.size();

                            report->error("`let` function `" + definition->name.toString()
                                + "` expects exactly " + std::to_string(expected)
                                + " argument" + (expected != 1 ? "s" : "")
                                + ", but got " + std::to_string(got)
                                + " argument" + (got != 1 ? "s" : "")
                                + " instead", expression->location);

                            return nullptr;
                        }

                        FwdUniquePtr<const Expression> result;

                        if (definition == builtins.getDefinition(Builtins::DefinitionType::HasDef)) {
                            if (const auto key = reducedArguments[0]->tryGet<Expression::StringLiteral>()) {
                                return makeFwdUnique<const Expression>(
                                    Expression::BooleanLiteral(builtins.getDefineExpression(key->value) != nullptr),
                                    expression->location,
                                    ExpressionInfo(EvaluationContext::CompileTime,
                                        makeFwdUnique<TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                                        Qualifiers::None));
                            } else {
                                report->error("`" + definition->name.toString() + "` argument #1 must be a compile-time string literal", expression->location);
                                return nullptr;
                            }
                        } else if (definition == builtins.getDefinition(Builtins::DefinitionType::GetDef)) {
                            if (const auto key = reducedArguments[0]->tryGet<Expression::StringLiteral>()) {
                                if (const auto define = builtins.getDefineExpression(key->value)) {
                                    if (enterLetExpression(definition->name, expression->location)) {
                                        result = reduceExpression(define);
                                        exitLetExpression();
                                    }
                                } else {
                                    return std::move(reducedArguments[1]);
                                }
                            } else {
                                report->error("`" + definition->name.toString() + "` argument #1 must be a compile-time string literal", expression->location);
                                return nullptr;
                            }
                        } else {
                            // Create a temporary scope with a bunch of temporary let declarations representing the arguments.
                            // This scope will be cleaned up at the end of this function.
                            auto scope = std::make_unique<SymbolTable>(definition->parentScope, StringView());

                            std::vector<FwdUniquePtr<const Statement>> argumentBindings;
                            argumentBindings.reserve(parameters.size());

                            for (std::size_t i = 0; i != parameters.size(); ++i) {
                                if (scope->createDefinition(report, Definition::Let({}, reducedArguments[i].get()), parameters[i], definition->declaration) == nullptr) {
                                    return nullptr;
                                }
                            }

                            // Use temporary scope to evaluate let function, and return the result.
                            enterScope(scope.get());
                            if (enterLetExpression(definition->name, expression->location)) {                            
                                result = reduceExpression(letDefinition->expression);
                                exitLetExpression();
                            }
                            exitScope();
                        }

                        return result;
                    } else if (const auto funcDefinition = definition->variant.tryGet<Definition::Func>()) {
                        auto& functionType = funcDefinition->resolvedSignatureType->function;
                        auto resultType = functionType.returnType->clone();

                        if (functionType.parameterTypes.size() != reducedArguments.size()) {
                            auto expected = functionType.parameterTypes.size();
                            auto got = reducedArguments.size();

                            report->error("`func " + definition->name.toString()
                                + "` expects exactly " + std::to_string(expected)
                                + " argument" + (expected != 1 ? "s" : "")
                                + ", but got " + std::to_string(got)
                                + " argument" + (got != 1 ? "s" : "")
                                + " instead", expression->location);
                        }

                        for (std::size_t i = 0; i != reducedArguments.size(); ++i) {
                            const auto& reducedArgument = reducedArguments[i];
                            const auto& parameterType = functionType.parameterTypes[i];

                            if (const auto resultType = findCompatibleAssignmentType(reducedArgument.get(), parameterType.get())) {
                                reducedArguments[i] = createConvertedExpression(reducedArgument.get(), resultType);
                            } else {
                                report->error("argument of type `" + getTypeName(parameterType.get()) + "` cannot be assigned `" + getTypeName(reducedArgument->info->type.get()) + "` expression", expression->location);
                                return nullptr;
                            }
                        }

                        return makeFwdUnique<const Expression>(
                            Expression::Call(call.inlined, std::move(function), std::move(reducedArguments)),
                            expression->location,
                            ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), Qualifiers::None));
                    } else if (const auto loadIntrinsic = definition->variant.tryGet<Definition::BuiltinLoadIntrinsic>()) {
                        if (call.inlined) {
                            report->error("`inline` keyword is not valid for instrinsics.", expression->location);
                            return nullptr;
                        }

                        return makeFwdUnique<const Expression>(
                            Expression::Call(call.inlined, std::move(function), std::move(reducedArguments)),
                            expression->location,
                            ExpressionInfo(EvaluationContext::RunTime,
                                makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(loadIntrinsic->type), expression->location),
                                Qualifiers::None));
                    } else if (definition->variant.is<Definition::BuiltinVoidIntrinsic>()) {
                        if (call.inlined) {
                            report->error("`inline` keyword is not valid for instrinsics.", expression->location);
                            return nullptr;
                        }

                        return makeFwdUnique<const Expression>(
                            Expression::Call(call.inlined, std::move(function), std::move(reducedArguments)),
                            expression->location,
                            ExpressionInfo(EvaluationContext::RunTime,
                                makeFwdUnique<const TypeExpression>(TypeExpression::Tuple({}), expression->location),
                                Qualifiers::None));
                    } else {
                        report->error("expression is not callable", expression->location);
                        return nullptr;
                    }
                } else {
                    if (const auto functionType = function->info->type->tryGet<TypeExpression::Function>()) {
                        auto resultType = functionType->returnType->clone();

                        if (call.inlined) {
                            report->error("`inline` keyword cannot be used on function expressions, only functions themselves.", expression->location);
                            return nullptr;
                        }

                        if (functionType->parameterTypes.size() != reducedArguments.size()) {
                            const auto expected = functionType->parameterTypes.size();
                            const auto got = reducedArguments.size();

                            report->error("`func` expects exactly "
                                + std::to_string(expected)
                                + " argument" + (expected != 1 ? "s" : "")
                                + ", but got " + std::to_string(got)
                                + " argument" + (got != 1 ? "s" : "")
                                + " instead", expression->location);
                        }

                        for (std::size_t i = 0; i != reducedArguments.size(); ++i) {
                            const auto& reducedArgument = reducedArguments[i];
                            const auto& parameterType = functionType->parameterTypes[i];

                            if (const auto resultType = findCompatibleAssignmentType(reducedArgument.get(), parameterType.get())) {
                                reducedArguments[i] = createConvertedExpression(reducedArgument.get(), resultType);
                            } else {
                                report->error("argument of type `" + getTypeName(parameterType.get()) + "` cannot be assigned `" + getTypeName(reducedArgument->info->type.get()) + "` expression", expression->location);
                                return nullptr;
                            }
                        }

                        return makeFwdUnique<const Expression>(
                            Expression::Call(call.inlined, std::move(function), std::move(reducedArguments)),
                            expression->location,
                            ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), Qualifiers::None));
                    }

                    report->error("expression is not callable", expression->location);
                    return nullptr;
                }

            }
            case ExpressionKind::Cast: {
                const auto& cast = expression->cast;
                auto operand = reduceExpression(cast.operand.get());
                auto destType = reduceTypeExpression(cast.type.get());

                if (operand == nullptr || destType == nullptr) {
                    return nullptr;
                }

                // TODO: bool -> integer type cast.
                const auto& sourceType = operand->info->type;

                Optional<Int128> integerValue;

                if (const auto integerLiteral = operand->tryGet<Expression::IntegerLiteral>()) {
                    integerValue = integerLiteral->value;
                }

                if (const auto resolvedIdentifier = operand->tryGet<Expression::ResolvedIdentifier>()) {
                    if (const auto funcDefinition = resolvedIdentifier->definition->variant.tryGet<Definition::Func>()) {
                        if (funcDefinition->inlined) {
                            report->error("`" + resolvedIdentifier->definition->name.toString() + "` is an `inline func` so it cannot be casted", expression->location);
                            return nullptr;
                        }

                        if (funcDefinition->address.hasValue() && funcDefinition->address.get().absolutePosition.hasValue()) {
                            integerValue = Int128(funcDefinition->address.get().absolutePosition.get()); 
                        }
                    }
                }

                bool validCast = false;

                if (isIntegerType(sourceType.get()) || isEnumType(sourceType.get()) || isPointerLikeType(sourceType.get())) {
                    if (const auto destTypeDefinition = tryGetResolvedIdentifierTypeDefinition(destType.get())) {
                        if (const auto destBuiltinIntegerType = destTypeDefinition->variant.tryGet<Definition::BuiltinIntegerType>()) {
                            validCast = true;

                            if (validCast && integerValue.hasValue()) {
                                const auto mask = Int128((1U << (8U * destBuiltinIntegerType->size)) - 1);
                                const auto result = *integerValue & mask;
                                return makeFwdUnique<const Expression>(
                                    Expression::IntegerLiteral(result),
                                    expression->location,
                                    ExpressionInfo(EvaluationContext::CompileTime, std::move(destType), Qualifiers::None));
                            }
                        } else if (destTypeDefinition->variant.is<Definition::BuiltinIntegerExpressionType>() || destTypeDefinition->variant.is<Definition::Enum>()) {
                            validCast = true;

                            if (validCast && integerValue.hasValue()) {
                                return makeFwdUnique<const Expression>(Expression::IntegerLiteral(*integerValue),
                                    expression->location,
                                    ExpressionInfo(EvaluationContext::CompileTime, std::move(destType), Qualifiers::None));
                            }
                        } else {
                            report->error("TODO: integer literal cast from `" + getTypeName(sourceType.get()) + "` to `" + getTypeName(destType.get()) + "`", expression->location);
                            return nullptr;
                        }
                    } else if (isPointerLikeType(destType.get())) {
                        const auto destFar = isFarType(destType.get());
                        const auto sourceFar = isFarType(sourceType.get());

                        validCast = !destFar || !isPointerLikeType(sourceType.get()) || sourceFar == destFar;

                        if (validCast && integerValue.hasValue()) {
                            const auto pointerSizedType = destFar ? platform->getFarPointerSizedType() : platform->getPointerSizedType();

                            return makeFwdUnique<const Expression>(
                                Expression::IntegerLiteral(*integerValue & Int128((1U << (8U * pointerSizedType->variant.get<Definition::BuiltinIntegerType>().size)) - 1)),
                                expression->location,
                                ExpressionInfo(EvaluationContext::CompileTime, std::move(destType), destFar ? Qualifiers::Far : Qualifiers::None));
                        }
                    }
                }

                if (validCast) {
                    const auto context = operand->info->context;
                    const auto destFar = isFarType(destType.get());
                    auto destTypeClone = destType->clone();
                    return makeFwdUnique<const Expression>(
                        Expression::Cast(std::move(operand), std::move(destType)),
                        expression->location,
                        ExpressionInfo(context, std::move(destTypeClone), destFar ? Qualifiers::Far : Qualifiers::None));
                }

                report->error("cannot cast expression from `" + getTypeName(sourceType.get()) + "` to `" + getTypeName(destType.get()) + "`", expression->location);
                return nullptr;
            }
            case ExpressionKind::Embed: {
                const auto& embed = expression->embed;
                Optional<StringView> data;
                StringView displayPath;
                StringView canonicalPath;
                std::unique_ptr<Reader> reader;

                importManager->setCurrentPath(expression->location.canonicalPath);
                const auto result = importManager->importModule(embed.originalPath, ImportOptions {}, displayPath, canonicalPath, reader);

                switch (result) {
                    case ImportResult::JustImported: {
                        if (reader && reader->isOpen()) {
                            data = stringPool->intern(reader->readFully());
                            embedCache[canonicalPath] = *data;
                        }
                        break;
                    }
                    case ImportResult::AlreadyImported: {
                        const auto match = embedCache.find(canonicalPath);
                        if (match != embedCache.end()) {
                            data = match->second;
                        }
                        break;
                    }
                    default:
                    case ImportResult::Failed: {
                        break;
                    }
                }

                if (data.hasValue()) {
                    return createStringLiteralExpression(*data, expression->location);
                } else {
                    report->error("could not open file \"" + text::escape(embed.originalPath, '\"') + "\" referenced by `embed` expression", expression->location);
                    return nullptr;
                }
            }
            case ExpressionKind::FieldAccess: {
                const auto& fieldAccess = expression->fieldAccess;
                auto operand = reduceExpression(fieldAccess.operand.get());
                if (operand == nullptr) {
                    return nullptr;
                }
                if (const auto typeOf = operand->tryGet<Expression::TypeOf>()) {
                    const auto& typeExpression = typeOf->expression->info->type;
                    if (auto member = resolveTypeMemberExpression(typeExpression.get(), fieldAccess.field)) {
                        return member;
                    } else {
                        return nullptr;
                    }
                } else {
                    if (auto member = resolveValueMemberExpression(operand.get(), fieldAccess.field)) {
                        return member;
                    } else {
                        return nullptr;
                    }
                }
            }
            case ExpressionKind::Identifier: {
                const auto& identifier = expression->identifier;
                const auto& pieces = identifier.pieces;
                const auto resolveResult = resolveIdentifier(pieces, expression->location);
                const auto definition = resolveResult.first;
                auto pieceIndex = resolveResult.second;

                if (definition == nullptr) {
                    return nullptr;
                }

                FwdUniquePtr<const Expression> currentExpression;

                if (isTypeDefinition(definition) && pieceIndex < pieces.size() - 1) {
                    ++pieceIndex;

                    auto resolvedType = makeFwdUnique<TypeExpression>(TypeExpression::ResolvedIdentifier(definition), expression->location);

                    const auto field = pieces[pieceIndex];
                    if (auto member = resolveTypeMemberExpression(resolvedType.get(), field)) {
                        currentExpression = std::move(member);
                    } else {
                        return nullptr;
                    }

                    if (pieceIndex == pieces.size() - 1) {
                        return currentExpression;
                    }
                } else {
                    currentExpression = resolveDefinitionExpression(definition, pieces, expression->location);
                }

                if (currentExpression == nullptr) {
                    return nullptr;
                } else if (pieceIndex < pieces.size() - 1) {
                    ++pieceIndex;
                    for (; pieceIndex < pieces.size(); ++pieceIndex) {
                        if (auto member = resolveValueMemberExpression(currentExpression.get(), pieces[pieceIndex])) {
                            currentExpression = std::move(member);
                        } else {
                            return nullptr;
                        }
                    }
                }

                return currentExpression;
            }
            case ExpressionKind::IntegerLiteral: {
                const auto& integerLiteral = expression->integerLiteral;
                if (expression->info.hasValue()) {
                    return expression->clone();
                }

                Definition* typeDefinition = nullptr;
                if (integerLiteral.suffix.getLength() > 0) {
                    typeDefinition = builtins.getBuiltinScope()->findLocalMemberDefinition(integerLiteral.suffix);
                    if (typeDefinition == nullptr || !typeDefinition->variant.is<Definition::BuiltinIntegerType>()) {
                        report->error("unrecognized integer literal suffix `" + integerLiteral.suffix.toString() + "`", expression->location);
                        return nullptr;
                    }

                    const auto& builtinIntegerType = typeDefinition->variant.get<Definition::BuiltinIntegerType>();
                    if (integerLiteral.value < builtinIntegerType.min || integerLiteral.value > builtinIntegerType.max) {
                        report->error("integer literal `" + integerLiteral.value.toString() + "` with `" + integerLiteral.suffix.toString() + "` suffix is outside valid range `" + builtinIntegerType.min.toString() + "` .. `" + builtinIntegerType.max.toString() + "`", expression->location);
                        return nullptr;
                    }
                } else {
                    typeDefinition = builtins.getDefinition(Builtins::DefinitionType::IExpr);               
                }
                
                return makeFwdUnique<const Expression>(Expression::IntegerLiteral(integerLiteral.value), expression->location,
                    ExpressionInfo(EvaluationContext::CompileTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(typeDefinition), expression->location),
                        Qualifiers::None));
            }
            case ExpressionKind::OffsetOf: {
                const auto& offsetOf = expression->offsetOf;
                const auto reducedTypeExpression = reduceTypeExpression(offsetOf.type.get());
                if (!reducedTypeExpression) {
                    return nullptr;
                }
                if (const auto resolvedIdentifierType = reducedTypeExpression->tryGet<TypeExpression::ResolvedIdentifier>()) {
                    if (const auto structDefinition = resolvedIdentifierType->definition->variant.tryGet<Definition::Struct>()) {
                        if (const auto memberDefinition = structDefinition->environment->findLocalMemberDefinition(offsetOf.field)) {
                            const auto& structMemberDefinition = memberDefinition->variant.get<Definition::StructMember>();

                            if (structMemberDefinition.offset.hasValue()) {
                                return makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(*structMemberDefinition.offset)), expression->location,
                                    ExpressionInfo(EvaluationContext::CompileTime,
                                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), expression->location),
                                        Qualifiers::None));
                            } else {
                                report->error("offset of field `" + offsetOf.field.toString() + "` in type `" + getTypeName(reducedTypeExpression.get()) + "` could not be resolved yet", expression->location);
                                return nullptr;                
                            }
                        } else {
                            report->error("`" + getTypeName(reducedTypeExpression.get()) + "` has no field named `" + offsetOf.field.toString() + "`", expression->location);
                            return nullptr;                
                        }
                    }
                }

                report->error("type `" + getTypeName(reducedTypeExpression.get()) + "` passed to `offsetof` is not a `struct` or `union` type", expression->location);

                return nullptr;                
            }
            case ExpressionKind::RangeLiteral: {
                const auto& rangeLiteral = expression->rangeLiteral;
                auto reducedStart = reduceExpression(rangeLiteral.start.get());
                auto reducedEnd = reduceExpression(rangeLiteral.end.get());
                auto reducedStep = rangeLiteral.step != nullptr
                    ? reduceExpression(rangeLiteral.step.get())
                    : makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(1)), expression->location,
                        ExpressionInfo(EvaluationContext::CompileTime,
                            makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), expression->location),
                            Qualifiers::None));
                if (!reducedStart || !reducedEnd || !reducedStep) {
                    return nullptr;
                }
                if (reducedStart->kind != ExpressionKind::IntegerLiteral) {
                    report->error("range start must be a compile-time integer literal", reducedStart->location);
                    return nullptr;
                }
                if (reducedEnd->kind != ExpressionKind::IntegerLiteral) {
                    report->error("range end must be a compile-time integer literal", reducedEnd->location);
                    return nullptr;
                }
                if (reducedStep->kind != ExpressionKind::IntegerLiteral) {
                    report->error("range step must be a compile-time integer literal", reducedStep->location);
                    return nullptr;
                }
                return makeFwdUnique<const Expression>(Expression::RangeLiteral(std::move(reducedStart), std::move(reducedEnd), std::move(reducedStep)), expression->location,
                    ExpressionInfo(EvaluationContext::CompileTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Range)), expression->location),
                        Qualifiers::None));
            }
            case ExpressionKind::ResolvedIdentifier: return expression->clone();
            case ExpressionKind::SideEffect: {
                const auto& sideEffect = expression->sideEffect;
                auto reducedResult = reduceExpression(sideEffect.result.get());
                if (reducedResult == nullptr) {
                    return nullptr;
                }
                auto resultType = reducedResult->info->type->clone();

                return makeFwdUnique<const Expression>(
                    Expression::SideEffect(sideEffect.statement->clone(), std::move(reducedResult)), expression->location,
                    ExpressionInfo(EvaluationContext::RunTime, std::move(resultType),
                    Qualifiers::None));
            }
            case ExpressionKind::StringLiteral: {
                const auto& stringLiteral = expression->stringLiteral;
                return createStringLiteralExpression(stringLiteral.value, expression->location);
            }
            case ExpressionKind::StructLiteral: {
                const auto& structLiteral = expression->structLiteral;
                auto reducedTypeExpression = reduceTypeExpression(structLiteral.type.get());
                if (reducedTypeExpression == nullptr) {
                    return nullptr;
                }

                Definition* definition = nullptr;
                if (const auto resolvedTypeIdentifier = reducedTypeExpression->tryGet<TypeExpression::ResolvedIdentifier>()) {
                    if (resolvedTypeIdentifier->definition->variant.is<Definition::Struct>()) {
                        definition = resolvedTypeIdentifier->definition;
                    }
                }
                if (definition == nullptr) {
                    report->error("struct literal is invalid for non-struct type `" + getTypeName(reducedTypeExpression.get()) + "`", expression->location);
                    return nullptr;                    
                }
                
                const auto& structDefinition = definition->variant.get<Definition::Struct>();
                std::unordered_map<StringView, std::unique_ptr<const Expression::StructLiteral::Item>> reducedItems;

                bool invalidLiteral = false;
                auto context = EvaluationContext::CompileTime;

                for (const auto& it : structLiteral.items) {
                    const auto& name = it.first;
                    const auto& item = it.second;

                    if (const auto memberDefinition = structDefinition.environment->findLocalMemberDefinition(name)) {
                        const auto& structMemberDefinition = memberDefinition->variant.get<Definition::StructMember>();

                        auto reducedValue = reduceExpression(item->value.get());
                        if (reducedValue != nullptr) {
                            if (const auto compatibleInitializerType = findCompatibleAssignmentType(reducedValue.get(), structMemberDefinition.resolvedType.get())) {
                                reducedValue = createConvertedExpression(reducedValue.get(), compatibleInitializerType);
                                switch (reducedValue->info->context) {
                                    case EvaluationContext::CompileTime: break;
                                    case EvaluationContext::LinkTime: {
                                        if (context == EvaluationContext::CompileTime) {
                                            context = reducedValue->info->context;
                                        }
                                        break;
                                    }
                                    case EvaluationContext::RunTime: {
                                        if (context == EvaluationContext::CompileTime
                                        || context == EvaluationContext::LinkTime) {
                                            context = reducedValue->info->context;
                                        }
                                        break;
                                    }
                                    default: std::abort(); break;
                                }

                                reducedItems[name] = std::make_unique<const Expression::StructLiteral::Item>(std::move(reducedValue), item->location);
                            } else {
                                report->error("field `" + name.toString() + "` of type `" + getTypeName(structMemberDefinition.resolvedType.get()) + "` cannot be initialized with `" + getTypeName(reducedValue->info->type.get()) + "` expression", reducedValue->location);
                                invalidLiteral = true;
                            }
                        } else {
                            invalidLiteral = true;
                        }
                    } else {
                        report->error("`" + getTypeName(reducedTypeExpression.get()) + "` has no field named `" + name.toString() + "`", item->location);
                        invalidLiteral = true;
                    }
                }

                if (invalidLiteral) {
                    return nullptr;
                }

                if (structDefinition.kind == StructKind::Struct) {
                    for (const auto& member : structDefinition.members) {
                        if (reducedItems.find(member->name) == reducedItems.end()) {
                            report->error("missing value for `" + member->name.toString() + "` in `" + getTypeName(reducedTypeExpression.get()) + "` literal", expression->location);
                        }
                    }
                } else if (structDefinition.kind == StructKind::Union) {
                    if (structDefinition.members.size() != 1) {
                        report->error("`" + getTypeName(reducedTypeExpression.get()) + "` literal must use exactly one field because it is a `union`, but " + std::to_string(structDefinition.members.size()) + " fields were given", expression->location);
                    }
                }

                auto reducedTypeExpressionClone = reducedTypeExpression->clone();

                return makeFwdUnique<const Expression>(
                    Expression::StructLiteral(std::move(reducedTypeExpressionClone), std::move(reducedItems)),
                    expression->location,
                    ExpressionInfo(context,
                        std::move(reducedTypeExpression),
                        Qualifiers::None));                
            }
            case ExpressionKind::TupleLiteral: {
                const auto& tupleLiteral = expression->tupleLiteral;
                std::vector<FwdUniquePtr<const Expression>> reducedItems;
                std::vector<FwdUniquePtr<const TypeExpression>> reducedItemTypes;

                reducedItems.reserve(tupleLiteral.items.size());
                reducedItemTypes.reserve(tupleLiteral.items.size());

                for (const auto& item : tupleLiteral.items) {
                    auto reducedItem = reduceExpression(item.get());
                    reducedItemTypes.push_back(reducedItem->info->type->clone());

                    if (reducedItem != nullptr) {
                        reducedItems.push_back(std::move(reducedItem));
                    } else {
                        return nullptr;
                    }
                }

                return makeFwdUnique<const Expression>(
                    Expression::TupleLiteral(std::move(reducedItems)),
                    expression->location,
                    ExpressionInfo(EvaluationContext::CompileTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::Tuple(std::move(reducedItemTypes)), expression->location),
                        Qualifiers::None));
            }
            case ExpressionKind::TypeOf: {
                const auto& typeOf = expression->typeOf;
                auto reducedExpression = reduceExpression(typeOf.expression.get());
                if (!reducedExpression) {
                    return nullptr;
                }
                return makeFwdUnique<const Expression>(
                    Expression::TypeOf(std::move(reducedExpression)),
                    expression->location,
                    ExpressionInfo(EvaluationContext::CompileTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::TypeOf)), expression->location),
                        Qualifiers::None));
            }
            case ExpressionKind::TypeQuery: {
                const auto& typeQuery = expression->typeQuery;
                auto reducedType = reduceTypeExpression(typeQuery.type.get());
                if (!reducedType) {
                    return nullptr;
                }    
                switch (typeQuery.kind) {
                    case TypeQueryKind::None: default: std::abort(); return nullptr;
                    case TypeQueryKind::SizeOf: {
                        const auto storageSize = calculateStorageSize(reducedType.get(), "`sizeof`"_sv);
                        if (storageSize.hasValue()) {
                            return makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(*storageSize)), expression->location,
                                ExpressionInfo(EvaluationContext::CompileTime,
                                    makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), expression->location),
                                    Qualifiers::None));
                        }
                        return nullptr;
                    }
                    case TypeQueryKind::AlignOf: {
                        report->error("TODO: alignof support.", expression->location);
                        return nullptr;
                    }
                }
            }
            case ExpressionKind::UnaryOperator: {
                const auto& unaryOperator = expression->unaryOperator;
                const auto op = unaryOperator.op;
                auto operand = reduceExpression(unaryOperator.operand.get());
                if (!operand) {
                    return nullptr;
                }

                switch (op) {
                    case UnaryOperatorKind::Indirection: break;
                    case UnaryOperatorKind::Grouping: break;
                    case UnaryOperatorKind::AddressOf: break;
                    case UnaryOperatorKind::FarAddressOf: break;
                    case UnaryOperatorKind::LowByte: break;
                    case UnaryOperatorKind::HighByte: break;
                    default: {
                        if ((operand->info->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) {
                            report->error("operand to " + getUnaryOperatorName(op).toString() + " cannot be `writeonly`", operand->location);
                            return nullptr;
                        }
                        break;
                    }
                }

                switch (op) {
                    default: case UnaryOperatorKind::None: std::abort(); return nullptr;
                    
                    // Increment operations.
                    // T -> T
                    // T is either an integer or pointer type.
                    case UnaryOperatorKind::PostDecrement: 
                    case UnaryOperatorKind::PostIncrement:
                    case UnaryOperatorKind::PreDecrement:
                    case UnaryOperatorKind::PreIncrement: {
                        auto resultType = operand->info->type->clone();
                        return makeFwdUnique<const Expression>(
                            Expression::UnaryOperator(unaryOperator.op, std::move(operand)), expression->location,
                            ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), Qualifiers::None));
                    }
                    
                    // Indirection. Run-time.
                    // *T -> T
                    case UnaryOperatorKind::Indirection: {
                        if (const auto& pointerType = operand->info->type->tryGet<TypeExpression::Pointer>()) {
                            const auto qualifiers = Qualifiers::LValue | (pointerType->qualifiers & (Qualifiers::Const | Qualifiers::WriteOnly | Qualifiers::Far));
                            auto resultType = pointerType->elementType->clone();

                            return makeFwdUnique<const Expression>(
                                Expression::UnaryOperator(unaryOperator.op, std::move(operand)), expression->location,
                                ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), qualifiers));
                        }

                        report->error(getUnaryOperatorName(unaryOperator.op).toString() + " is not defined for provided operand type `" + getTypeName(operand->info->type.get()) + "`", expression->location);
                        return nullptr;
                    }

                    // Address-of operator. Compile-time or link-time.
                    // T -> *T
                    // If the operand is an indirection, cancels it out.
                    // If the operand has a known address at the time of evaluation, it is compile-time. Otherwise, it is link-time.
                    case UnaryOperatorKind::AddressOf:
                    case UnaryOperatorKind::FarAddressOf: {
                        if (const auto nestedUnaryOperator = operand->tryGet<Expression::UnaryOperator>()) {
                            if (nestedUnaryOperator->op == UnaryOperatorKind::Indirection) {
                                if (const auto pointerType = nestedUnaryOperator->operand->info->type->tryGet<TypeExpression::Pointer>()) {
                                    if (((pointerType->qualifiers & Qualifiers::Far) != Qualifiers::None) != (op == UnaryOperatorKind::FarAddressOf)) {
                                        report->error(getUnaryOperatorName(unaryOperator.op).toString() + " is not defined for provided operand type `" + getTypeName(operand->info->type.get()) + "`", expression->location);
                                    }

                                    return nestedUnaryOperator->operand->clone();
                                }
                            }
                        } else if (const auto binaryOperator = operand->tryGet<Expression::BinaryOperator>()) {
                            if (binaryOperator->op == BinaryOperatorKind::Indexing) {
                                const auto left = binaryOperator->left.get();
                                const auto right = binaryOperator->right.get();
                                auto context = left->info->context;
                                auto qualifiers = Qualifiers::None;

                                if (right->info->context == EvaluationContext::RunTime) {
                                    context = right->info->context;
                                }

                                if (const auto pointerType = binaryOperator->left->info->type->tryGet<TypeExpression::Pointer>()) {
                                    if (((pointerType->qualifiers & Qualifiers::Far) != Qualifiers::None) != (op == UnaryOperatorKind::FarAddressOf)) {
                                        if (op == UnaryOperatorKind::AddressOf) {
                                            report->error(getUnaryOperatorName(unaryOperator.op).toString() + " is not defined for provided operand type `" + getTypeName(operand->info->type.get()) + "`. use `far &` instead", expression->location);
                                        } else {
                                            report->error(getUnaryOperatorName(unaryOperator.op).toString() + " is not defined for provided far operand type `" + getTypeName(operand->info->type.get()) + "`. use `&` (without `far`) instead", expression->location);
                                        }
                                    }
                                }

                                auto resultType = makeFwdUnique<const TypeExpression>(
                                    TypeExpression::Pointer(operand->info->type->clone(), qualifiers),
                                    operand->info->type->location);
                                return makeFwdUnique<const Expression>(
                                    Expression::UnaryOperator(unaryOperator.op, std::move(operand)), expression->location,
                                    ExpressionInfo(context, std::move(resultType), Qualifiers::None));
                            }
                        } else if (const auto resolvedIdentifier = operand->tryGet<Expression::ResolvedIdentifier>()) {
                            const auto pointerSizedType = op == UnaryOperatorKind::FarAddressOf ? platform->getFarPointerSizedType() : platform->getPointerSizedType();
                            const auto mask = Int128((1U << (8U * pointerSizedType->variant.get<Definition::BuiltinIntegerType>().size)) - 1);
                            const auto farQualifier = op == UnaryOperatorKind::FarAddressOf ? Qualifiers::Far : Qualifiers::None;

                            if (const auto varDefinition = resolvedIdentifier->definition->variant.tryGet<Definition::Var>()) {
                                auto resultType = makeFwdUnique<const TypeExpression>(
                                    TypeExpression::Pointer(
                                        operand->info->type->clone(),
                                        ((operand->info->qualifiers & (Qualifiers::Const | Qualifiers::WriteOnly)) | farQualifier)),
                                    operand->info->type->location);

                                if (varDefinition->address.hasValue() && varDefinition->address->absolutePosition.hasValue()) {
                                    return makeFwdUnique<const Expression>(
                                        Expression::IntegerLiteral(Int128(varDefinition->address->absolutePosition.get()) & mask), expression->location,
                                        ExpressionInfo(EvaluationContext::CompileTime, std::move(resultType), farQualifier));
                                } else {
                                    return makeFwdUnique<const Expression>(
                                        Expression::UnaryOperator(unaryOperator.op, std::move(operand)), expression->location,
                                        ExpressionInfo(EvaluationContext::LinkTime, std::move(resultType), farQualifier));
                                }
                            } else if (const auto funcDefinition = resolvedIdentifier->definition->variant.tryGet<Definition::Func>()) {
                                if (funcDefinition->inlined) {
                                    report->error("`" + resolvedIdentifier->definition->name.toString() + "` is an `inline func` so it has no address that can be taken with " + getUnaryOperatorName(unaryOperator.op).toString(), expression->location);
                                    return nullptr;
                                }

                                auto resultType = makeFwdUnique<const TypeExpression>(
                                    TypeExpression::Pointer(
                                        operand->info->type->clone(), 
                                        Qualifiers::Const | farQualifier),
                                    operand->info->type->location);

                                if (funcDefinition->address.hasValue() && funcDefinition->address.get().absolutePosition.hasValue()) {
                                    return makeFwdUnique<const Expression>(
                                        Expression::IntegerLiteral(Int128(funcDefinition->address->absolutePosition.get()) & mask), expression->location,
                                        ExpressionInfo(EvaluationContext::CompileTime, std::move(resultType), farQualifier));
                                } else {
                                    return makeFwdUnique<const Expression>(
                                        Expression::UnaryOperator(unaryOperator.op, std::move(operand)), expression->location,
                                        ExpressionInfo(EvaluationContext::LinkTime, std::move(resultType), farQualifier));
                                }
                            }
                        }
                        report->error(getUnaryOperatorName(unaryOperator.op).toString() + " cannot be used on provided expression", expression->location);
                        return nullptr;
                    }

                    // Bitwise negation:
                    // integer -> integer
                    case UnaryOperatorKind::BitwiseNegation: {
                        const auto resultType = operand->info->type.get();

                        if (isBooleanType(resultType)) {
                            return simplifyLogicalNotExpression(expression, std::move(operand));
                        } else if (isIntegerType(resultType)) {
                            if (operand->info->context == EvaluationContext::RunTime) {
                                return makeFwdUnique<const Expression>(
                                    Expression::UnaryOperator(unaryOperator.op, std::move(operand)), expression->location,
                                    ExpressionInfo(EvaluationContext::RunTime, resultType->clone(), Qualifiers::None));
                            } else if (operand->info->context == EvaluationContext::LinkTime) {
                                return makeFwdUnique<const Expression>(
                                    Expression::UnaryOperator(unaryOperator.op, std::move(operand)), expression->location,
                                    ExpressionInfo(EvaluationContext::LinkTime, resultType->clone(), Qualifiers::None));
                            } else {
                                if (const auto integerLiteral = operand->tryGet<Expression::IntegerLiteral>()) {
                                    if (const auto typeDefinition = tryGetResolvedIdentifierTypeDefinition(resultType)) {
                                        if (const auto builtinIntegerType = typeDefinition->variant.tryGet<Definition::BuiltinIntegerType>()) {
                                            const auto mask = Int128((1U << (8U * builtinIntegerType->size)) - 1);
                                            const auto result = ~integerLiteral->value & mask;
                                            return makeFwdUnique<const Expression>(
                                                Expression::IntegerLiteral(result),
                                                expression->location,
                                                ExpressionInfo(EvaluationContext::CompileTime, resultType->clone(), Qualifiers::None));
                                        } else if (typeDefinition->variant.is<Definition::BuiltinIntegerExpressionType>()) {
                                            return makeFwdUnique<const Expression>(
                                                Expression::IntegerLiteral(~integerLiteral->value), expression->location,
                                                ExpressionInfo(EvaluationContext::CompileTime, resultType->clone(), Qualifiers::None));
                                        }
                                    }
                                }
                            }
                        }
                           
                        report->error(getUnaryOperatorName(unaryOperator.op).toString() + " is not defined for provided operand type `" + getTypeName(resultType) + "`", expression->location);
                        return nullptr;
                    }

                    // Expression grouping. Returns the operand directly and removes itself from the tree.
                    case UnaryOperatorKind::Grouping: return operand;

                    // Logical negation.
                    // bool -> bool
                    case UnaryOperatorKind::LogicalNegation: {
                        return simplifyLogicalNotExpression(expression, std::move(operand));
                    }

                    // Signed negation.
                    // integer -> integer
                    case UnaryOperatorKind::SignedNegation: {
                        const auto resultType = operand->info->type.get();

                        if (isIntegerType(resultType)) {
                            if (operand->info->context == EvaluationContext::RunTime) {
                                return makeFwdUnique<const Expression>(
                                    Expression::UnaryOperator(unaryOperator.op, std::move(operand)), expression->location,
                                    ExpressionInfo(EvaluationContext::RunTime, resultType->clone(), Qualifiers::None));
                            } else if (operand->info->context == EvaluationContext::LinkTime) {
                                return makeFwdUnique<const Expression>(
                                    Expression::UnaryOperator(unaryOperator.op, std::move(operand)), expression->location,
                                    ExpressionInfo(EvaluationContext::LinkTime, resultType->clone(), Qualifiers::None));
                            } else {
                                const auto operandValue = operand->integerLiteral.value;
                                const auto result = Int128().checkedSubtract(operandValue);
                                if (result.first == Int128::CheckedArithmeticResult::Success) {
                                    const auto value = result.second;

                                    if (const auto typeDefinition = tryGetResolvedIdentifierTypeDefinition(resultType)) {
                                        if (const auto builtinIntegerType = typeDefinition->variant.tryGet<Definition::BuiltinIntegerType>()) {
                                            if (value < builtinIntegerType->min || value > builtinIntegerType->max) {
                                                report->error(getUnaryOperatorName(unaryOperator.op).toString() + " resulted in `" + getTypeName(resultType) + "` value of `" + value.toString() + "` outside valid range `" + builtinIntegerType->min.toString() + "` .. `" + builtinIntegerType->max.toString() + "`", expression->location);
                                                return nullptr;
                                            }
                                        }
                                    }

                                    return makeFwdUnique<const Expression>(
                                        Expression::IntegerLiteral(value), expression->location,
                                        ExpressionInfo(EvaluationContext::CompileTime, resultType->clone(), Qualifiers::None));
                                } else {
                                    report->error(getUnaryOperatorName(unaryOperator.op).toString() + " resulted in overflow", expression->location);
                                    return nullptr;
                                }
                            }
                        }
                           
                        report->error(getUnaryOperatorName(unaryOperator.op).toString() + " is not defined for provided operand type `" + getTypeName(operand->info->type.get()) + "`", expression->location);
                        return nullptr;
                    }

                    // Byte access operators.
                    // T -> u8, where offset < sizeof(T) or T is iexpr.
                    case UnaryOperatorKind::LowByte:
                    case UnaryOperatorKind::HighByte:
                    case UnaryOperatorKind::BankByte: {
                        const auto sourceType = operand->info->type.get();

                        std::size_t offset;
                        switch (unaryOperator.op) {
                            case UnaryOperatorKind::LowByte: offset = 0; break;
                            case UnaryOperatorKind::HighByte: offset = 1; break;
                            case UnaryOperatorKind::BankByte: offset = 2; break;
                            default: std::abort(); return nullptr;
                        }

                        Optional<std::size_t> storageSize;
                        bool needsStorageSizeCheck = true;

                        if (const auto typeDefinition = tryGetResolvedIdentifierTypeDefinition(sourceType)) {
                            needsStorageSizeCheck = typeDefinition != builtins.getDefinition(Builtins::DefinitionType::IExpr);
                        }

                        if (needsStorageSizeCheck) {
                            storageSize = calculateStorageSize(sourceType, "operand"_sv);
                            if (!storageSize.hasValue()) {
                                return nullptr;
                            }
                        }

                        if ((!isIntegerType(sourceType) && !isEnumType(sourceType) && !isPointerLikeType(sourceType))
                        || (storageSize.hasValue() && offset >= *storageSize)) {
                            report->error(getUnaryOperatorName(unaryOperator.op).toString() + " is not defined for provided operand type `" + getTypeName(operand->info->type.get()) + "`", expression->location);
                            return nullptr;
                        }

                        auto resultType = makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::U8)), expression->location);


                        bool simplify = false;
                        auto context = operand->info->context;
                        Optional<Int128> absolutePosition;
                    
                        if (const auto integerLiteral = operand->tryGet<Expression::IntegerLiteral>()) {
                            return makeFwdUnique<const Expression>(
                                Expression::IntegerLiteral(integerLiteral->value.logicalRightShift(8 * offset) & Int128(0xFF)), expression->location,
                                ExpressionInfo(EvaluationContext::CompileTime, std::move(resultType), Qualifiers::None));                                
                        } else if (const auto resolvedIdentifier = operand->tryGet<Expression::ResolvedIdentifier>()) {
                            const auto definition = resolvedIdentifier->definition;
                            if (const auto registerDefinition = definition->variant.tryGet<Definition::BuiltinRegister>()) {
                                const auto subRegisters = builtins.findRegisterDecomposition(definition);
                                if (subRegisters.getLength() == 0) {
                                    report->error("`" + definition->name.toString() + "` cannot be split into smaller registers, so it cannot be used with " + getUnaryOperatorName(unaryOperator.op).toString(), expression->location);
                                    return nullptr;
                                } else if (offset >= subRegisters.getLength()) {
                                    report->error("`" + definition->name.toString() + "` is only made of " + std::to_string(subRegisters.getLength()) + " registers, so it cannot be used with " + getUnaryOperatorName(unaryOperator.op).toString(), expression->location);
                                    return nullptr;
                                } else {
                                    const auto subRegister = subRegisters[offset];
                                    const auto& subRegisterDefinition = subRegisters[offset]->variant.get<Definition::BuiltinRegister>();

                                    return makeFwdUnique<const Expression>(Expression::ResolvedIdentifier(subRegister, {}), expression->location,
                                        ExpressionInfo(EvaluationContext::RunTime,
                                            makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(subRegisterDefinition.type), expression->location),
                                            Qualifiers::LValue));
                                }
                            } else if (const auto varDefinition = definition->variant.tryGet<Definition::Var>()) {
                                simplify = true;

                                if (varDefinition->address.hasValue() && varDefinition->address->absolutePosition.hasValue()) {
                                    absolutePosition = varDefinition->address->absolutePosition.get();
                                }
                            } else if (const auto funcDefinition = definition->variant.tryGet<Definition::Func>()) {
                                if (funcDefinition->inlined) {
                                    report->error("`" + definition->name.toString() + "` is an `inline func` so it cannot be used with " + getUnaryOperatorName(unaryOperator.op).toString(), expression->location);
                                    return nullptr;
                                }

                                if (funcDefinition->address.hasValue() && funcDefinition->address.get().absolutePosition.hasValue()) {
                                    return makeFwdUnique<const Expression>(
                                        Expression::IntegerLiteral(Int128(funcDefinition->address->absolutePosition.get()).logicalRightShift(8 * offset) & Int128(0xFF)), expression->location,
                                        ExpressionInfo(EvaluationContext::CompileTime, std::move(resultType), Qualifiers::None));
                                }
                            }
                        } else if (const auto nestedUnaryOperator = operand->tryGet<Expression::UnaryOperator>()) {
                            const auto& nestedOperand = nestedUnaryOperator->operand;
                            if (nestedUnaryOperator->op == UnaryOperatorKind::Indirection) {
                                simplify = true;
                                context = nestedUnaryOperator->operand->info->context;

                                if (const auto integerLiteral = nestedOperand->tryGet<Expression::IntegerLiteral>()) {
                                    absolutePosition = integerLiteral->value;
                                }
                            }
                        } else if (const auto binaryOperator = operand->tryGet<Expression::BinaryOperator>()) {
                            if (binaryOperator->op == BinaryOperatorKind::Indexing) {
                                simplify = true;
                            }
                        }

                        if (simplify) {
                            return simplifyIndirectionOffsetExpression(std::move(resultType), operand.get(), context, absolutePosition, Int128(offset));
                        }

                        return makeFwdUnique<const Expression>(
                            Expression::UnaryOperator(unaryOperator.op, std::move(operand)), expression->location,
                            ExpressionInfo(context, std::move(resultType), operand->info->qualifiers));
                    }

                    // Address reserve operator `@`
                    // Reserves an address for some data, and returns the pointer to that data.
                    // The data will be located immediately after the expression that references it.
                    // T -> *const T
                    case UnaryOperatorKind::AddressReserve: {
                        if (!allowReservedConstants) {
                            report->error("cannot use " + getUnaryOperatorName(unaryOperator.op).toString() + " here", expression->location);
                            return nullptr;
                        }

                        if (operand->info->context == EvaluationContext::CompileTime
                        || operand->info->context == EvaluationContext::LinkTime) {
                            const auto constType = operand->info->type.get();
                            const auto constName = stringPool->intern("$data" + std::to_string(definitionPool.size()));
                            auto constDeclaration = statementPool.addNew(Statement::InternalDeclaration(), expression->location);
                            auto definition = definitionPool.addNew(Definition::Var(Qualifiers::Const, currentFunction, nullptr, nullptr, 0), constName, constDeclaration);
                            auto& constDefinition = definition->variant.get<Definition::Var>();

                            constDefinition.resolvedType = constType;
                            constDefinition.initializerExpression = std::move(operand);

                            const auto elementTypePtr =
                                constType->kind == TypeExpressionKind::Array
                                ? constType->array.elementType.get()
                                : constType;

                            const auto elementStorageSize = calculateStorageSize(elementTypePtr, ""_sv);
                            if (!elementStorageSize.hasValue()) {
                                report->error("operand of " + getUnaryOperatorName(unaryOperator.op).toString() + " cannot be of type " + getTypeName(constType) + " because it has unknown size", expression->location);
                                return nullptr;
                            }

                            reservedConstants.push_back(definition);

                            auto pointerToElementType = makeFwdUnique<const TypeExpression>(
                                TypeExpression::Pointer(elementTypePtr->clone(), Qualifiers::Const),
                                expression->location);
                            const auto pointerToElementTypePtr = pointerToElementType.get();

                            // &data as *U
                            return makeFwdUnique<const Expression>(
                                Expression::Cast(
                                    makeFwdUnique<const Expression>(
                                        Expression::UnaryOperator(
                                            UnaryOperatorKind::AddressOf,
                                            makeFwdUnique<const Expression>(
                                                Expression::ResolvedIdentifier(definition, {constName}),
                                                expression->location,
                                                ExpressionInfo(EvaluationContext::LinkTime,
                                                    constType->clone(),
                                                    Qualifiers::LValue | Qualifiers::Const))),
                                        expression->location,
                                        ExpressionInfo(
                                            EvaluationContext::LinkTime,
                                            makeFwdUnique<const TypeExpression>(
                                                TypeExpression::Pointer(constType->clone(), Qualifiers::Const),
                                                expression->location),
                                            Qualifiers::None)),
                                std::move(pointerToElementType)),
                                expression->location,
                                ExpressionInfo(
                                    EvaluationContext::LinkTime,
                                    pointerToElementTypePtr->clone(),
                                    Qualifiers::None));
                        } else {
                            report->error("operand of " + getUnaryOperatorName(unaryOperator.op).toString() + " must be a link-time expression.", expression->location);
                            return nullptr;
                        }
                    }
                }
            }
            default: std::abort(); return nullptr;
        }
    }

    Optional<std::size_t> Compiler::tryGetSequenceLiteralLength(const Expression* expression) const {
        if (const auto arrayLiteral = expression->tryGet<Expression::ArrayLiteral>()) {
            return arrayLiteral->items.size();
        } else if (const auto stringLiteral = expression->tryGet<Expression::StringLiteral>()) {
            return stringLiteral->value.getLength();
        } else if (const auto rangeLiteral = expression->tryGet<Expression::RangeLiteral>()) {
            const auto rangeStartLiteral = rangeLiteral->start->tryGet<Expression::IntegerLiteral>();
            const auto rangeEndLiteral = rangeLiteral->end->tryGet<Expression::IntegerLiteral>();
            const auto rangeStepLiteral = rangeLiteral->step->tryGet<Expression::IntegerLiteral>();

            if (rangeStartLiteral == nullptr || rangeEndLiteral == nullptr || rangeStepLiteral == nullptr) {
                return Optional<std::size_t>();
            }

            if (rangeStepLiteral->value.isZero()) {
                return Optional<std::size_t>();
            } else {
                const auto low = rangeStepLiteral->value.isNegative() ? rangeEndLiteral->value : rangeStartLiteral->value;
                const auto high = rangeStepLiteral->value.isNegative() ? rangeStartLiteral->value : rangeEndLiteral->value;
                if (low > high) {
                    return 0;
                }

                const auto step = rangeStepLiteral->value;

                return (high - low) / (step.isNegative() ? -step : step) + Int128(1);
            }
        }

        return Optional<std::size_t>();
    }

    FwdUniquePtr<const Expression> Compiler::getSequenceLiteralItem(const Expression* expression, std::size_t index) const {
        if (const auto arrayLiteral = expression->tryGet<Expression::ArrayLiteral>()) {
            return arrayLiteral->items[index]->clone();
        } else if (const auto stringLiteral = expression->tryGet<Expression::StringLiteral>()) {
            return makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(static_cast<std::uint8_t>(stringLiteral->value[index]))), expression->location,
                ExpressionInfo(EvaluationContext::CompileTime,
                    makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), expression->location),
                    Qualifiers::None));
        } else if (const auto rangeLiteral = expression->tryGet<Expression::RangeLiteral>()) {
            const auto rangeStartLiteral = rangeLiteral->start->tryGet<Expression::IntegerLiteral>();
            const auto rangeEndLiteral = rangeLiteral->end->tryGet<Expression::IntegerLiteral>();
            const auto rangeStepLiteral = rangeLiteral->step->tryGet<Expression::IntegerLiteral>();

            if (rangeStartLiteral == nullptr || rangeEndLiteral == nullptr || rangeStepLiteral == nullptr) {
                return nullptr;
            }

            if (rangeStepLiteral->value.isZero()) {
                std::abort();
                return nullptr;
            } else {
                return makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(rangeStartLiteral->value + rangeStepLiteral->value * Int128(index))), expression->location,
                    ExpressionInfo(EvaluationContext::CompileTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), expression->location),
                        Qualifiers::None));
            }  
        }

        std::abort();
        return nullptr;
    }

    FwdUniquePtr<const Expression> Compiler::createStringLiteralExpression(StringView data, SourceLocation location) const {
        return makeFwdUnique<const Expression>(Expression::StringLiteral(data), location,
            ExpressionInfo(EvaluationContext::CompileTime,
                makeFwdUnique<const TypeExpression>(TypeExpression::Array(
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::U8)), location), 
                        makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(data.getLength())), location, 
                            ExpressionInfo(EvaluationContext::CompileTime,
                                makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), location),
                                Qualifiers::None))),
                    location),
                Qualifiers::None));
    }

    FwdUniquePtr<const Expression> Compiler::createArrayLiteralExpression(std::vector<FwdUniquePtr<const Expression>> items, const TypeExpression* elementType, SourceLocation location) const {
        const auto size = items.size();

        auto context = EvaluationContext::CompileTime;
        for (const auto& item : items) {
            if (item->info->context == EvaluationContext::LinkTime) {
                context = item->info->context;
            }
        }

        return makeFwdUnique<const Expression>(Expression::ArrayLiteral(std::move(items)), location,
            ExpressionInfo(context,
                makeFwdUnique<const TypeExpression>(TypeExpression::Array(
                        elementType ? elementType->clone() : nullptr, 
                        makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(size)), location,
                            ExpressionInfo(EvaluationContext::CompileTime,
                                makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), location),
                                Qualifiers::None))),
                    location),
                Qualifiers::None));
    }

    std::string Compiler::getResolvedIdentifierName(Definition* definition, const std::vector<StringView>& pieces) const {
        if (pieces.size() > 0) {
            return text::join(pieces.begin(), pieces.end(), ".");
        } else {
            return definition->name.toString();
        }
    }

    FwdUniquePtr<const Expression> Compiler::resolveDefinitionExpression(Definition* definition, const std::vector<StringView>& pieces, SourceLocation location) {
        if (const auto letDefinition = definition->variant.tryGet<Definition::Let>()) {
            if (letDefinition->parameters.size() == 0) {
                FwdUniquePtr<const Expression> result;

                enterScope(definition->parentScope);
                if (enterLetExpression(definition->name, location)) {
                    result = reduceExpression(letDefinition->expression);
                    exitLetExpression();
                }
                exitScope();

                return result != nullptr
                    ? result->clone(location, ExpressionInfo(result->info->context, result->info->type->clone(), result->info->qualifiers))
                    : nullptr;
            } else {
                return makeFwdUnique<const Expression>(Expression::ResolvedIdentifier(definition, pieces), location,
                    ExpressionInfo(EvaluationContext::CompileTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Let)), location),
                        Qualifiers::None));
            }
        } else if (const auto varDefinition = definition->variant.tryGet<Definition::Var>()) {
            if (varDefinition->resolvedType == nullptr) {
                report->error("encountered a reference to `" + std::string(
                    ((varDefinition->qualifiers & Qualifiers::Const) != Qualifiers::None) ? "const"
                    : ((varDefinition->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) ? "writeonly"
                    : "var") + " " + getResolvedIdentifierName(definition, pieces) + "` before its type was known", location);
                return nullptr;
            }
            if (const auto designatedStorageType = varDefinition->resolvedType->tryGet<TypeExpression::DesignatedStorage>()) {
                return designatedStorageType->holder->clone(location, ExpressionInfo(
                    EvaluationContext::RunTime,
                    designatedStorageType->elementType->clone(),
                    designatedStorageType->holder->info->qualifiers
                ));
            }
            return makeFwdUnique<const Expression>(Expression::ResolvedIdentifier(definition, pieces), location,
                ExpressionInfo(
                    varDefinition->resolvedType->kind == TypeExpressionKind::Array ? EvaluationContext::LinkTime : EvaluationContext::RunTime,
                    varDefinition->resolvedType->clone(),
                    Qualifiers::LValue | (varDefinition->qualifiers & (Qualifiers::Const | Qualifiers::WriteOnly))));
        } else if (const auto registerDefinition = definition->variant.tryGet<Definition::BuiltinRegister>()) {
            return makeFwdUnique<const Expression>(Expression::ResolvedIdentifier(definition, pieces), location,
                ExpressionInfo(EvaluationContext::RunTime,
                    makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(registerDefinition->type), location),
                    Qualifiers::LValue));
        } else if (const auto funcDefinition = definition->variant.tryGet<Definition::Func>()) {
            if (funcDefinition->resolvedSignatureType == nullptr) {
                report->error("encountered a reference to func `" + getResolvedIdentifierName(definition, pieces) + "` before its type was known", location);
                return nullptr;
            }

            auto context = EvaluationContext::LinkTime;
            if (funcDefinition->address.hasValue() && funcDefinition->address->absolutePosition.hasValue()) {
                context = EvaluationContext::CompileTime;
            }

            return makeFwdUnique<const Expression>(Expression::ResolvedIdentifier(definition, pieces),
                location,
                ExpressionInfo(
                    context,
                    funcDefinition->resolvedSignatureType->clone(),
                    (funcDefinition->far ? Qualifiers::Far : Qualifiers::None)));
        } else if (definition->variant.is<Definition::BuiltinVoidIntrinsic>()) {
            return makeFwdUnique<const Expression>(Expression::ResolvedIdentifier(definition, pieces), location,
                ExpressionInfo(EvaluationContext::RunTime,
                    makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Intrinsic)), location),
                Qualifiers::None));
        } else if (definition->variant.is<Definition::BuiltinLoadIntrinsic>()) {
            return makeFwdUnique<const Expression>(Expression::ResolvedIdentifier(definition, pieces), location,
                ExpressionInfo(EvaluationContext::RunTime,
                    makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Intrinsic)), location),
                Qualifiers::None));
        } else if (const auto enumMemberDefinition = definition->variant.tryGet<Definition::EnumMember>()) {
            if (enumMemberDefinition->reducedExpression == nullptr) {
                report->error("encountered a reference to enum value `" + getResolvedIdentifierName(definition, pieces) + "` before its value was known", location);
                return nullptr;
            }
            return enumMemberDefinition->reducedExpression->clone();
        }

        report->error("`" + getResolvedIdentifierName(definition, pieces) + "` cannot be used as an expression", location);
        return nullptr;
    }

    FwdUniquePtr<const Expression> Compiler::resolveTypeMemberExpression(const TypeExpression* typeExpression, StringView name) {
        if (const auto resolvedTypeIdentifier = tryGetResolvedIdentifierTypeDefinition(typeExpression)) {
            if (const auto enumDefinition = resolvedTypeIdentifier->variant.tryGet<Definition::Enum>()) {
                if (const auto enumMemberDefinition = enumDefinition->environment->findLocalMemberDefinition(name)) {
                    return resolveDefinitionExpression(enumMemberDefinition, {}, typeExpression->location);
                }
            } else {
                const auto prop = builtins.findPropertyByName(name);

                if (const auto builtinIntegerType = resolvedTypeIdentifier->variant.tryGet<Definition::BuiltinIntegerType>()) {
                    switch (prop) {
                        case Builtins::Property::MinValue: {
                            return makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(builtinIntegerType->min)), typeExpression->location,
                                ExpressionInfo(EvaluationContext::CompileTime,
                                    makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(resolvedTypeIdentifier), typeExpression->location),
                                Qualifiers::None));
                        }
                        case Builtins::Property::MaxValue: {
                            return makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(builtinIntegerType->max)), typeExpression->location,
                                ExpressionInfo(EvaluationContext::CompileTime,
                                    makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(resolvedTypeIdentifier), typeExpression->location),
                                Qualifiers::None));
                        }
                        default: break;
                    }
                }
            }
        }

        report->error("`" + getTypeName(typeExpression) + "` has no member named `" + name.toString() + "`", typeExpression->location);
        return nullptr;
    }

    FwdUniquePtr<const Expression> Compiler::resolveValueMemberExpression(const Expression* expression, StringView name) {
        const auto typeExpression = expression->info->type.get();

        if (const auto resolvedTypeIdentifier = tryGetResolvedIdentifierTypeDefinition(typeExpression)) {
            if (const auto structDefinition = resolvedTypeIdentifier->variant.tryGet<Definition::Struct>()) {
                if (const auto memberDefinition = structDefinition->environment->findLocalMemberDefinition(name)) {
                    const auto& structMemberDefinition = memberDefinition->variant.get<Definition::StructMember>();
                    auto resultType = structMemberDefinition.resolvedType->clone();

                    bool simplify = false;
                    auto context = expression->info->context;
                    Optional<Int128> absolutePosition;
                    
                    if (const auto structLiteral = expression->tryGet<Expression::StructLiteral>()) {
                        const auto& match = structLiteral->items.find(name);
                        return match->second->value->clone();
                    } else if (const auto resolvedExpressionIdentifier = expression->tryGet<Expression::ResolvedIdentifier>()) {
                        if (const auto varDefinition = resolvedExpressionIdentifier->definition->variant.tryGet<Definition::Var>()) {
                            simplify = true;

                            if (varDefinition->address.hasValue() && varDefinition->address->absolutePosition.hasValue()) {
                                absolutePosition = varDefinition->address->absolutePosition.get();
                            }
                        }
                    } else if (const auto unaryOperator = expression->tryGet<Expression::UnaryOperator>()) {
                        const auto& operand = unaryOperator->operand;
                        if (unaryOperator->op == UnaryOperatorKind::Indirection) {
                            simplify = true;
                            context = unaryOperator->operand->info->context;

                            if (const auto integerLiteral = operand->tryGet<Expression::IntegerLiteral>()) {
                                absolutePosition = integerLiteral->value;
                            }
                        }
                    } else if (const auto binaryOperator = expression->tryGet<Expression::BinaryOperator>()) {
                        if (binaryOperator->op == BinaryOperatorKind::Indexing) {
                            simplify = true;
                        }
                    }

                    if (simplify) {
                        return simplifyIndirectionOffsetExpression(std::move(resultType), expression, context, absolutePosition, Int128(*structMemberDefinition.offset));
                    }

                    return makeFwdUnique<const Expression>(
                        Expression::FieldAccess(expression->clone(), memberDefinition->name),
                        expression->location,
                        ExpressionInfo(
                            EvaluationContext::RunTime,
                            std::move(resultType),
                            expression->info->qualifiers & (Qualifiers::LValue | Qualifiers::Const | Qualifiers::WriteOnly | Qualifiers::Far)));
                }
            }
        } else if (const auto& pointerType = typeExpression->tryGet<TypeExpression::Pointer>()) {
            const auto qualifiers = Qualifiers::LValue | (pointerType->qualifiers & (Qualifiers::Const | Qualifiers::WriteOnly | Qualifiers::Far));
            auto resultType = pointerType->elementType->clone();
            auto indirection = makeFwdUnique<const Expression>(
                Expression::UnaryOperator(UnaryOperatorKind::Indirection, expression->clone()), expression->location,
                ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), qualifiers));
            auto member = resolveValueMemberExpression(indirection.get(), name);
            return member;
        } else {
            const auto prop = builtins.findPropertyByName(name);
            switch (prop) {
                case Builtins::Property::Len: {
                     if (const auto& arrayType = typeExpression->tryGet<TypeExpression::Array>()) {
                         if (const auto& sizeLiteral = arrayType->size->tryGet<Expression::IntegerLiteral>()) {
                             return makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(sizeLiteral->value)), expression->location,
                                 ExpressionInfo(EvaluationContext::CompileTime,
                                     makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), expression->location),
                                 Qualifiers::None));
                         } else {
                             report->error("`" + getTypeName(typeExpression) + "` expression has unknown length", expression->location);
                         }
                     } else if (const auto len = tryGetSequenceLiteralLength(expression)) {
                        return makeFwdUnique<const Expression>(Expression::IntegerLiteral(Int128(*len)), expression->location,
                            ExpressionInfo(EvaluationContext::CompileTime,
                                makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), expression->location),
                            Qualifiers::None));
                    } else {
                        report->error("`" + getTypeName(typeExpression) + "` expression has unknown length", expression->location);
                    }
                }
                default: break;
            }
        }

        report->error("`" + getTypeName(typeExpression) + "` has no field named `" + name.toString() + "`", expression->location);
        return nullptr;
    }

    FwdUniquePtr<const Expression> Compiler::simplifyIndirectionOffsetExpression(FwdUniquePtr<const TypeExpression> resultType, const Expression* expression, EvaluationContext context, Optional<Int128> absolutePosition, Int128 offset) {
        const auto qualifiers = expression->info->qualifiers & (Qualifiers::LValue | Qualifiers::Const | Qualifiers::WriteOnly | Qualifiers::Far);
        auto addressType = makeFwdUnique<const TypeExpression>(
            TypeExpression::Pointer(
                resultType->clone(),
                qualifiers & (Qualifiers::Const | Qualifiers::WriteOnly | Qualifiers::Far)),
            expression->info->type->location);

        if (absolutePosition.hasValue()) {
            if (resultType->kind == TypeExpressionKind::Array) {
                return makeFwdUnique<const Expression>(
                    Expression::IntegerLiteral(Int128(absolutePosition.get()) + offset),
                    expression->location,
                    ExpressionInfo(EvaluationContext::CompileTime, std::move(resultType), qualifiers));
            } else {
                return makeFwdUnique<const Expression>(
                    Expression::UnaryOperator(
                        UnaryOperatorKind::Indirection,
                        makeFwdUnique<const Expression>(
                            Expression::IntegerLiteral(Int128(absolutePosition.get()) + offset),
                            expression->location,
                            ExpressionInfo(EvaluationContext::CompileTime, std::move(addressType), Qualifiers::None))),
                    expression->location,
                    ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), qualifiers));
            }
        } else {
            if (resultType->kind == TypeExpressionKind::Array) {
                return makeFwdUnique<const Expression>(
                    Expression::BinaryOperator(
                        BinaryOperatorKind::Addition,
                        expression->clone(),
                        makeFwdUnique<const Expression>(
                            Expression::IntegerLiteral(offset),
                            expression->location,
                            ExpressionInfo(
                                EvaluationContext::CompileTime,
                                makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), expression->location),
                                Qualifiers::None))),
                    expression->location,
                    ExpressionInfo(context, std::move(resultType), qualifiers));
            } else {
                const auto pointerSizedType = (qualifiers & Qualifiers::Far) != Qualifiers::None ? platform->getFarPointerSizedType() : platform->getPointerSizedType();
                const auto addressOfOp = (qualifiers & Qualifiers::Far) != Qualifiers::None ? UnaryOperatorKind::FarAddressOf : UnaryOperatorKind::AddressOf;
                auto addressOf = makeFwdUnique<const Expression>(Expression::UnaryOperator(addressOfOp, expression->clone()), expression->location, Optional<ExpressionInfo>());
                auto reducedAddressOf = reduceExpression(addressOf.get());

                if (!reducedAddressOf) {
                    return nullptr;
                }

                return makeFwdUnique<const Expression>(
                    Expression::UnaryOperator(
                        UnaryOperatorKind::Indirection,
                        makeFwdUnique<const Expression>(
                            Expression::Cast(
                                makeFwdUnique<const Expression>(
                                    Expression::BinaryOperator(
                                        BinaryOperatorKind::Addition,
                                        makeFwdUnique<const Expression>(
                                            Expression::Cast(
                                                std::move(reducedAddressOf),
                                                makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(pointerSizedType), expression->location)),
                                            expression->location,
                                            ExpressionInfo(
                                                context, 
                                                makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(pointerSizedType), expression->location),
                                                Qualifiers::None)),
                                        makeFwdUnique<const Expression>(
                                            Expression::IntegerLiteral(offset),
                                            expression->location,
                                            ExpressionInfo(
                                                EvaluationContext::CompileTime,
                                                makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), expression->location),
                                                Qualifiers::None))),
                                    expression->location,
                                    ExpressionInfo(
                                        context,
                                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(pointerSizedType), expression->location),
                                        Qualifiers::None)),
                            addressType->clone()),
                            expression->location,
                            ExpressionInfo(context, addressType->clone(), Qualifiers::None))),
                        expression->location,
                        ExpressionInfo(EvaluationContext::RunTime, std::move(resultType), qualifiers));
            }
        }
    }

    FwdUniquePtr<const Expression> Compiler::simplifyLogicalNotExpression(const Expression* expression, FwdUniquePtr<const Expression> operand) {
        const auto resultType = operand->info->type.get();

        if (isBooleanType(resultType)) {
            if (operand->info->context == EvaluationContext::RunTime) {
                return makeFwdUnique<const Expression>(
                    Expression::UnaryOperator(UnaryOperatorKind::LogicalNegation, std::move(operand)), expression->location,
                    ExpressionInfo(EvaluationContext::RunTime, resultType->clone(), Qualifiers::None));
            } else if (operand->info->context == EvaluationContext::LinkTime) {
                return makeFwdUnique<const Expression>(
                    Expression::UnaryOperator(UnaryOperatorKind::LogicalNegation, std::move(operand)), expression->location,
                    ExpressionInfo(EvaluationContext::LinkTime, resultType->clone(), Qualifiers::None));
            } else {
                const auto operandValue = operand->booleanLiteral.value;
                return makeFwdUnique<const Expression>(
                    Expression::BooleanLiteral(!operandValue), expression->location,
                    ExpressionInfo(EvaluationContext::CompileTime, resultType->clone(), Qualifiers::None));
            }
        }
                           
        report->error(getUnaryOperatorName(UnaryOperatorKind::LogicalNegation).toString() + " is not defined for provided operand type `" + getTypeName(resultType) + "`", expression->location);
        return nullptr;
    }

    FwdUniquePtr<const Expression> Compiler::simplifyBinaryArithmeticExpression(const Expression* expression, BinaryOperatorKind op, FwdUniquePtr<const Expression> left, FwdUniquePtr<const Expression> right) {
        if (const auto resultType = findCompatibleBinaryArithmeticExpressionType(left.get(), right.get())) {
            const auto leftContext = left->info->context;
            const auto rightContext = right->info->context;
            if (leftContext == EvaluationContext::RunTime || rightContext == EvaluationContext::RunTime) {
                return makeFwdUnique<const Expression>(Expression::BinaryOperator(op, std::move(left), std::move(right)), expression->location,
                    ExpressionInfo(EvaluationContext::RunTime, resultType->clone(), Qualifiers::None));
            } else if (leftContext == EvaluationContext::LinkTime || rightContext == EvaluationContext::LinkTime) {
                return makeFwdUnique<const Expression>(Expression::BinaryOperator(op, std::move(left), std::move(right)), expression->location,
                    ExpressionInfo(EvaluationContext::LinkTime, resultType->clone(), Qualifiers::None));
            } else {
                const auto leftValue = left->integerLiteral.value;
                const auto rightValue = right->integerLiteral.value;
                const auto result = applyIntegerArithmeticOp(op, leftValue, rightValue);

                switch (result.first) {
                    case Int128::CheckedArithmeticResult::Success: {
                        const auto& value = result.second;
                        if (const auto typeDefinition = tryGetResolvedIdentifierTypeDefinition(resultType)) {
                            if (const auto builtinIntegerType = typeDefinition->variant.tryGet<Definition::BuiltinIntegerType>()) {
                                if (value < builtinIntegerType->min || value > builtinIntegerType->max) {
                                    report->error(getBinaryOperatorName(op).toString() + " resulted in `" + getTypeName(resultType) + "` value of `" + value.toString() + "` outside valid range `" + builtinIntegerType->min.toString() + "` .. `" + builtinIntegerType->max.toString() + "`", expression->location);
                                    return nullptr;
                                }
                            }
                        }
                        return makeFwdUnique<const Expression>(Expression::IntegerLiteral(result.second), expression->location, 
                            ExpressionInfo(EvaluationContext::CompileTime, resultType->clone(), Qualifiers::None));
                    }
                    case Int128::CheckedArithmeticResult::OverflowError: {
                        report->error(getBinaryOperatorName(op).toString() + " resulted in overflow", right->location);
                        return nullptr;
                    }
                    case Int128::CheckedArithmeticResult::DivideByZeroError: {
                        report->error(getBinaryOperatorName(op).toString() + " by zero", right->location);
                        return nullptr;
                    }
                    default: std::abort();
                }
            }
        }
        report->error(getBinaryOperatorName(op).toString() + " is not defined between provided operand types `" + getTypeName(left->info->type.get()) + "` and `" + getTypeName(right->info->type.get()) + "`", expression->location);
        return nullptr;
    }

    FwdUniquePtr<const Expression> Compiler::simplifyBinaryLogicalExpression(const Expression* expression, BinaryOperatorKind op, FwdUniquePtr<const Expression> left, FwdUniquePtr<const Expression> right) {
        if (isBooleanType(left->info->type.get()) && isBooleanType(right->info->type.get())) {
            bool isValidOperator = false;
            switch (op) {
                case BinaryOperatorKind::LogicalAnd:
                case BinaryOperatorKind::LogicalOr:
                case BinaryOperatorKind::BitwiseAnd:
                case BinaryOperatorKind::BitwiseOr:
                case BinaryOperatorKind::BitwiseXor:
                    isValidOperator = true;
                default: break;
            }

            if (isValidOperator) {
                const auto leftLiteral = left->tryGet<Expression::BooleanLiteral>();
                const auto rightLiteral = right->tryGet<Expression::BooleanLiteral>();

                if (leftLiteral != nullptr && rightLiteral != nullptr) {
                    bool result = false;
                    switch (op) {
                        case BinaryOperatorKind::LogicalAnd: result = leftLiteral->value && rightLiteral->value;  break;
                        case BinaryOperatorKind::LogicalOr: result = leftLiteral->value || rightLiteral->value; break;
                        case BinaryOperatorKind::BitwiseAnd: result = leftLiteral->value && rightLiteral->value; break;
                        case BinaryOperatorKind::BitwiseOr: result = leftLiteral->value || rightLiteral->value; break;
                        case BinaryOperatorKind::BitwiseXor: result = leftLiteral->value != rightLiteral->value; break;
                        default: std::abort(); break;
                    }

                    return makeFwdUnique<const Expression>(
                        Expression::BooleanLiteral(result), expression->location,
                        ExpressionInfo(EvaluationContext::CompileTime,
                            makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                            Qualifiers::None));
                } else if (op == BinaryOperatorKind::LogicalAnd)  {
                    if ((leftLiteral != nullptr && !leftLiteral->value) || (rightLiteral != nullptr && !rightLiteral->value)) {
                        return makeFwdUnique<const Expression>(
                            Expression::BooleanLiteral(false), expression->location,
                            ExpressionInfo(EvaluationContext::CompileTime,
                                makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                                Qualifiers::None));
                    } else if (leftLiteral != nullptr && leftLiteral->value) {
                        return right;
                    } else if (rightLiteral != nullptr && rightLiteral->value) {
                        return left;
                    }
                } else if (op == BinaryOperatorKind::LogicalOr)  {
                    if ((leftLiteral != nullptr && leftLiteral->value) || (rightLiteral != nullptr && rightLiteral->value)) {
                        return makeFwdUnique<const Expression>(
                            Expression::BooleanLiteral(true), expression->location,
                            ExpressionInfo(EvaluationContext::CompileTime,
                                makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                                Qualifiers::None));
                    } else if (leftLiteral != nullptr && !leftLiteral->value) {
                        return right;
                    } else if (rightLiteral != nullptr && !rightLiteral->value) {
                        return left;
                    }
                }

                const auto isRuntime = left->info->context == EvaluationContext::RunTime || right->info->context == EvaluationContext::RunTime;
                return makeFwdUnique<const Expression>(
                    Expression::BinaryOperator(op, std::move(left), std::move(right)),
                    expression->location,
                    ExpressionInfo(isRuntime ? EvaluationContext::RunTime : EvaluationContext::LinkTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                        Qualifiers::None));
            }
        }

        report->error(getBinaryOperatorName(op).toString() + " is not defined between provided operand types `" + getTypeName(left->info->type.get()) + "` and `" + getTypeName(right->info->type.get()) + "`", expression->location);
        return nullptr;
    }

    FwdUniquePtr<const Expression> Compiler::simplifyBinaryRotateExpression(const Expression* expression, BinaryOperatorKind op, FwdUniquePtr<const Expression> left, FwdUniquePtr<const Expression> right) {
        if (const auto resultType = findCompatibleBinaryArithmeticExpressionType(left.get(), right.get())) {
            if (const auto resultTypeDefinition = tryGetResolvedIdentifierTypeDefinition(resultType)) {
                if (const auto resultBuiltinIntegerType = resultTypeDefinition->variant.tryGet<Definition::BuiltinIntegerType>()) {
                    const auto leftContext = left->info->context;
                    const auto rightContext = right->info->context;
                    if (leftContext == EvaluationContext::RunTime || rightContext == EvaluationContext::RunTime) {
                        return makeFwdUnique<const Expression>(Expression::BinaryOperator(op, std::move(left), std::move(right)), expression->location,
                            ExpressionInfo(EvaluationContext::RunTime, resultType->clone(), Qualifiers::None));            
                    } else if (leftContext == EvaluationContext::LinkTime || rightContext == EvaluationContext::LinkTime) {
                        return makeFwdUnique<const Expression>(Expression::BinaryOperator(op, std::move(left), std::move(right)), expression->location,
                            ExpressionInfo(EvaluationContext::LinkTime, resultType->clone(), Qualifiers::None));
                    } else {
                        const auto value = left->integerLiteral.value;
                        std::size_t bits = right->integerLiteral.value >= Int128(SIZE_MAX)
                            ? SIZE_MAX
                            : static_cast<size_t>(right->integerLiteral.value);

                        bits %= 8 * resultBuiltinIntegerType->size;

                        switch (op) {
                            case BinaryOperatorKind::LeftRotate: {
                                const auto result = value.logicalLeftShift(bits) | value.logicalRightShift(8 * resultBuiltinIntegerType->size - bits);
                                return makeFwdUnique<const Expression>(Expression::IntegerLiteral(result), expression->location, 
                                    ExpressionInfo(EvaluationContext::CompileTime, resultType->clone(), Qualifiers::None));
                            }
                            case BinaryOperatorKind::RightRotate: {
                                const auto result = value.logicalRightShift(bits) | value.logicalLeftShift(8 * resultBuiltinIntegerType->size - bits);
                                return makeFwdUnique<const Expression>(Expression::IntegerLiteral(result), expression->location, 
                                    ExpressionInfo(EvaluationContext::CompileTime, resultType->clone(), Qualifiers::None));
                            }
                            default: std::abort();
                        }
                    }
                }
            }            
        }
        report->error(getBinaryOperatorName(op).toString() + " is not defined between provided operand types `" + getTypeName(left->info->type.get()) + "` and `" + getTypeName(right->info->type.get()) + "`", expression->location);
        return nullptr;
    }

    FwdUniquePtr<const Expression> Compiler::simplifyBinaryComparisonExpression(const Expression* expression, BinaryOperatorKind op, FwdUniquePtr<const Expression> left, FwdUniquePtr<const Expression> right) {
        const auto leftContext = left->info->context;
        const auto rightContext = right->info->context;

        if (const auto commonType = findCompatibleBinaryArithmeticExpressionType(left.get(), right.get())) {
            static_cast<void>(commonType);

            if (leftContext == EvaluationContext::RunTime || rightContext == EvaluationContext::RunTime) {
                return makeFwdUnique<const Expression>(
                    Expression::BinaryOperator(op, std::move(left), std::move(right)), expression->location,
                    ExpressionInfo(EvaluationContext::RunTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                        Qualifiers::None));
            } else if (leftContext == EvaluationContext::LinkTime || rightContext == EvaluationContext::LinkTime) {
                return makeFwdUnique<const Expression>(
                    Expression::BinaryOperator(op, std::move(left), std::move(right)), expression->location,
                    ExpressionInfo(EvaluationContext::LinkTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                        Qualifiers::None));
            } else {
                const auto leftValue = left->integerLiteral.value;
                const auto rightValue = right->integerLiteral.value;
                const auto result = applyIntegerComparisonOp(op, leftValue, rightValue);
                return makeFwdUnique<const Expression>(
                    Expression::BooleanLiteral(result), expression->location,
                    ExpressionInfo(EvaluationContext::CompileTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                        Qualifiers::None));
            }
        } else if (isBooleanType(left->info->type.get()) && isBooleanType(right->info->type.get())) {
            if (leftContext == EvaluationContext::RunTime || rightContext == EvaluationContext::RunTime) {
                return makeFwdUnique<const Expression>(
                    Expression::BinaryOperator(op, std::move(left), std::move(right)), expression->location,
                    ExpressionInfo(EvaluationContext::RunTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                        Qualifiers::None));
            } else if (leftContext == EvaluationContext::LinkTime || rightContext == EvaluationContext::LinkTime) {
                return makeFwdUnique<const Expression>(
                    Expression::BinaryOperator(op, std::move(left), std::move(right)), expression->location,
                    ExpressionInfo(EvaluationContext::LinkTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                        Qualifiers::None));
            } else {
                const auto leftValue = left->booleanLiteral.value;
                const auto rightValue = right->booleanLiteral.value;                   
                const auto result = applyBooleanComparisonOp(op, leftValue, rightValue);
                return makeFwdUnique<const Expression>(
                    Expression::BooleanLiteral(result), expression->location,
                    ExpressionInfo(EvaluationContext::CompileTime,
                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::Bool)), expression->location),
                        Qualifiers::None));
            }
        }

        report->error(getBinaryOperatorName(op).toString() + " is not defined between provided operand types `" + getTypeName(left->info->type.get()) + "` and `" + getTypeName(right->info->type.get()) + "`", expression->location);
        return nullptr;
    }

    bool Compiler::isSimpleCast(const Expression* expression) const {
        if (const auto cast = expression->tryGet<Expression::Cast>()) {
            const auto originalSize = calculateStorageSize(cast->operand->info->type.get(), ""_sv);
            const auto castedSize = calculateStorageSize(expression->info->type.get(), ""_sv);
            if (originalSize.hasValue() && castedSize.hasValue() && *originalSize == *castedSize) {
                return true;
            }
        }

        return false;
    }

    bool Compiler::isTypeDefinition(const Definition* definition) const {
        if (definition->variant.is<Definition::BuiltinBankType>()
        || definition->variant.is<Definition::BuiltinBoolType>()
        || definition->variant.is<Definition::BuiltinIntegerType>()
        || definition->variant.is<Definition::BuiltinIntegerExpressionType>()
        || definition->variant.is<Definition::BuiltinRangeType>()
        || definition->variant.is<Definition::Enum>()
        || definition->variant.is<Definition::Struct>()) {
            return true;
        }
        return false;
    }

    Definition* Compiler::tryGetResolvedIdentifierTypeDefinition(const TypeExpression* typeExpression) const {
        if (typeExpression != nullptr) {
            if (const auto resolvedDefinitionType = typeExpression->tryGet<TypeExpression::ResolvedIdentifier>()) {
                const auto definition = resolvedDefinitionType->definition;
                if (isTypeDefinition(definition)) {
                    return definition;
                }
            }
        }

        return nullptr;
    }

    bool Compiler::isIntegerType(const TypeExpression* typeExpression) const {
        const auto definition = tryGetResolvedIdentifierTypeDefinition(typeExpression);
        if (definition != nullptr) {
            return definition->variant.is<Definition::BuiltinIntegerExpressionType>()
            || definition->variant.is<Definition::BuiltinIntegerType>();
        } else {
            return false;
        }
    }

    bool Compiler::isBooleanType(const TypeExpression* typeExpression) const {
        const auto definition = tryGetResolvedIdentifierTypeDefinition(typeExpression);
        return definition != nullptr && definition->variant.is<Definition::BuiltinBoolType>();
    }

    bool Compiler::isEmptyTupleType(const TypeExpression* typeExpression) const {
        if (const auto tupleReturnType = typeExpression->tryGet<TypeExpression::Tuple>()) {
            if (tupleReturnType->elementTypes.size() == 0) {
                return true;
            }
        }
        return false;
    }

    bool Compiler::isEnumType(const TypeExpression* typeExpression) const {
        const auto definition = tryGetResolvedIdentifierTypeDefinition(typeExpression);
        if (definition != nullptr) {
            return definition->variant.is<Definition::Enum>();
        } else {
            return false;
        }
    }

    bool Compiler::isPointerLikeType(const TypeExpression* typeExpression) const {
        return typeExpression->kind == TypeExpressionKind::Pointer || typeExpression->kind == TypeExpressionKind::Function;
    }

    bool Compiler::isFarType(const TypeExpression* typeExpression) const {
        if (const auto pointerType = typeExpression->tryGet<TypeExpression::Pointer>()) {
            return (pointerType->qualifiers & Qualifiers::Far) == Qualifiers::Far;
        } else if (const auto functionType = typeExpression->tryGet<TypeExpression::Function>()) {
            return functionType->far;
        } 
        return false;
    }

    const TypeExpression* Compiler::findCompatibleBinaryArithmeticExpressionType(const Expression* left, const Expression* right) const {
        if (left == nullptr || right == nullptr || left->info->type == nullptr || right->info->type == nullptr) {
            return nullptr;
        }

        // Check both left and right have integral types.
        const auto leftDefinition = tryGetResolvedIdentifierTypeDefinition(left->info->type.get());
        const auto rightDefinition = tryGetResolvedIdentifierTypeDefinition(right->info->type.get());
        if (leftDefinition != nullptr && rightDefinition != nullptr) {
            if (isIntegerType(left->info->type.get()) && isIntegerType(right->info->type.get())) {
                // If left type and right type are same type, return that type.
                if (leftDefinition == rightDefinition) {
                    return left->info->type.get();
                }
                // If left type is iexpr and right side isn't, attempt to narrow to right side type.
                if (leftDefinition->variant.is<Definition::BuiltinIntegerExpressionType>()
                && rightDefinition->variant.is<Definition::BuiltinIntegerType>()
                && canNarrowIntegerExpression(left, rightDefinition)) {
                    return right->info->type.get();
                }
                // If right type is iexpr and left side isn't, attempt to narrow to right side type.
                if (rightDefinition->variant.is<Definition::BuiltinIntegerExpressionType>()
                && leftDefinition->variant.is<Definition::BuiltinIntegerType>()
                && canNarrowIntegerExpression(right, leftDefinition)) {
                    return left->info->type.get();
                }
            }
        }

        // Otherwise, failure to find a common type.
        return nullptr;
    }

    const TypeExpression* Compiler::findCompatibleConcatenationExpressionType(const Expression* left, const Expression* right) const {
        if (left == nullptr || right == nullptr || left->info->type == nullptr || right->info->type == nullptr) {
            return nullptr;
        }

        if (const auto leftArrayType = left->info->type->tryGet<TypeExpression::Array>()) {
            if (const auto rightArrayType = right->info->type->tryGet<TypeExpression::Array>()) {
                const auto leftElementType = leftArrayType->elementType.get();
                const auto rightElementType = rightArrayType->elementType.get();

                if (isTypeEquivalent(leftElementType, rightElementType)) {
                    return left->info->type.get();
                }

                if (const auto leftArray = left->tryGet<Expression::ArrayLiteral>()) {
                    bool success = true;
                    for (const auto& item : leftArray->items) {
                        if (!canNarrowExpression(item.get(), rightElementType)) {
                            success = false;
                            break;
                        }
                    }
                    if (success) {
                        return right->info->type.get();
                    }
                }

                if (const auto rightArray = left->tryGet<Expression::ArrayLiteral>()) {
                    bool success = true;
                    for (const auto& item : rightArray->items) {
                        if (!canNarrowExpression(item.get(), leftElementType)) {
                            success = false;
                            break;
                        }
                    }
                    if (success) {
                        return left->info->type.get();
                    }
                }
            }
        }

        return nullptr;
    }


    const TypeExpression* Compiler::findCompatibleAssignmentType(const Expression* initializer, const TypeExpression* declarationType) const {
        // TODO: prevent declaring anything as iexpr, [iexpr], etc or any other type without a guaranteed size after compilation.
        // TODO: probably need to recurse array types to handle complex cases like, [[[iexpr]]] -> [[[integer type]]]
        // TODO: if declaration type is an array with no explicit size, use initializer's known size.

        if (initializer == nullptr || declarationType == nullptr) {
            return nullptr;
        }

        if (canNarrowExpression(initializer, declarationType)) {
            const auto& initializerType = initializer->info->type;

            if (const auto sourceArrayType = initializerType->tryGet<TypeExpression::Array>()) {
                if (const auto destinationArrayType = declarationType->tryGet<TypeExpression::Array>()) {
                    if (sourceArrayType->size && destinationArrayType->size) {
                        const auto sourceSize = sourceArrayType->size->integerLiteral.value;
                        const auto destinationSize = destinationArrayType->size->integerLiteral.value;

                        if (sourceSize != destinationSize) {
                            return nullptr;
                        }
                    }
                }
            }

            return declarationType;
        }

        return nullptr;
    }

    bool Compiler::canNarrowExpression(const Expression* sourceExpression, const TypeExpression* destinationType) const {
        if (sourceExpression == nullptr || destinationType == nullptr) {
            return false;
        }

        const auto sourceExpressionType = sourceExpression->info->type.get();

        if (const auto destinationArrayType = destinationType->tryGet<TypeExpression::Array>()) {
            if (const auto sourceArrayType = sourceExpressionType->tryGet<TypeExpression::Array>()) {
                if (destinationArrayType->size != nullptr && sourceArrayType->size->integerLiteral.value != destinationArrayType->size->integerLiteral.value) {
                    return false;
                }

                const auto sourceElementType = sourceArrayType->elementType.get();
                const auto destinationElementType = destinationArrayType->elementType.get();

                if (isTypeEquivalent(destinationElementType, sourceElementType)) {
                    return true;
                }

                if (const auto sourceArray = sourceExpression->tryGet<Expression::ArrayLiteral>()) {
                    for (const auto& item : sourceArray->items) {
                        if (!canNarrowExpression(item.get(), destinationElementType)) {
                            return false;
                        }
                    }

                    return true;
                }
            }
        }

        if (const auto destinationDesignatedStorageType = destinationType->tryGet<TypeExpression::DesignatedStorage>()) {
            return canNarrowExpression(sourceExpression, destinationDesignatedStorageType->elementType.get());
        }

        if (const auto destinationPointerType = destinationType->tryGet<TypeExpression::Pointer>()) {
            if (const auto sourcePointerType = sourceExpressionType->tryGet<TypeExpression::Pointer>()) {
                const auto destinationElementType = destinationPointerType->elementType.get();
                const auto sourceElementType = sourcePointerType->elementType.get();

                if (isTypeEquivalent(destinationElementType, sourceElementType)
                && (((sourcePointerType->qualifiers & Qualifiers::WriteOnly) == Qualifiers::None
                        && (destinationPointerType->qualifiers & Qualifiers::Const) != Qualifiers::None)
                    || ((sourcePointerType->qualifiers & Qualifiers::Const) == Qualifiers::None
                        && (destinationPointerType->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None))
                && ((sourcePointerType->qualifiers & Qualifiers::Far) == (destinationPointerType->qualifiers & Qualifiers::Far)
                    || (destinationPointerType->qualifiers & Qualifiers::Far) == Qualifiers::None)) {
                    return true;
                }
            }
        }

        if (const auto destinationTypeDefinition = tryGetResolvedIdentifierTypeDefinition(destinationType)) {
            if (const auto sourceExpressionTypeDefinition = tryGetResolvedIdentifierTypeDefinition(sourceExpressionType)) {
                // If source expression type and definition type are same type, return that type.
                if (sourceExpressionTypeDefinition == destinationTypeDefinition) {
                    return true;
                }

                // Are they different types, but both integers?
                if (isIntegerType(sourceExpressionType) && isIntegerType(destinationType)) {
                    // If source expression type is iexpr and destination type is some bounded integer type, attempt to narrow to destination type.
                    if (sourceExpressionTypeDefinition->variant.is<Definition::BuiltinIntegerExpressionType>()) {
                        return canNarrowIntegerExpression(sourceExpression, destinationTypeDefinition);
                    }
                }
            }
        }

        return isTypeEquivalent(sourceExpressionType, destinationType);
    }

    bool Compiler::canNarrowIntegerExpression(const Expression* expression, const Definition* integerTypeDefinition) const {
        if (const auto integerLiteral = expression->tryGet<Expression::IntegerLiteral>()) {
            if (const auto builtinIntegerType = integerTypeDefinition->variant.tryGet<Definition::BuiltinIntegerType>()) {            
                if (integerLiteral->value >= builtinIntegerType->min
                && integerLiteral->value <= builtinIntegerType->max) {
                    return true;
                }
            }
        }

        return false;
    }

    FwdUniquePtr<const Expression> Compiler::createConvertedExpression(const Expression* sourceExpression, const TypeExpression* destinationType) const {
        if (sourceExpression == nullptr || destinationType == nullptr) {
            return nullptr;
        }

        const auto sourceExpressionType = sourceExpression->info->type.get();

        if (const auto destinationArrayType = destinationType->tryGet<TypeExpression::Array>()) {
            if (const auto sourceArrayType = sourceExpressionType->tryGet<TypeExpression::Array>()) {
                const auto destinationElementType = destinationArrayType->elementType.get();

                if (isTypeEquivalent(destinationElementType, sourceArrayType->elementType.get())) {
                    return sourceExpression->clone();
                }

                if (const auto sourceArray = sourceExpression->tryGet<Expression::ArrayLiteral>()) {
                    std::vector<FwdUniquePtr<const Expression>> convertedItems;
                    convertedItems.reserve(sourceArray->items.size());

                    for (const auto& item : sourceArray->items) {
                        convertedItems.push_back(createConvertedExpression(item.get(), destinationElementType));
                    }

                    return createArrayLiteralExpression(std::move(convertedItems), destinationElementType, sourceExpression->location);
                }
            }
        }

        if (isTypeEquivalent(sourceExpressionType, destinationType)) {
            return sourceExpression->clone();
        }

        if (const auto destinationDesignatedStorageType = destinationType->tryGet<TypeExpression::DesignatedStorage>()) {
            return createConvertedExpression(sourceExpression, destinationDesignatedStorageType->elementType.get());
        }

        // Adding const or writeonly to expression that didn't have it.
        if (const auto destinationPointerType = destinationType->tryGet<TypeExpression::Pointer>()) {
            if (const auto sourcePointerType = sourceExpressionType->tryGet<TypeExpression::Pointer>()) {
                const auto destinationElementType = destinationPointerType->elementType.get();
                const auto sourceElementType = sourcePointerType->elementType.get();

                if (isTypeEquivalent(destinationElementType, sourceElementType)
                && (((sourcePointerType->qualifiers & Qualifiers::WriteOnly) == Qualifiers::None
                        && (destinationPointerType->qualifiers & Qualifiers::Const) != Qualifiers::None)
                    || ((sourcePointerType->qualifiers & Qualifiers::Const) == Qualifiers::None
                        && (destinationPointerType->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None))
                && ((sourcePointerType->qualifiers & Qualifiers::Far) == (destinationPointerType->qualifiers & Qualifiers::Far)
                    || (destinationPointerType->qualifiers & Qualifiers::Far) == Qualifiers::None)) {
                    return sourceExpression->clone(
                        sourceExpression->location,
                        ExpressionInfo(
                            sourceExpression->info->context,
                            destinationType->clone(),
                            sourceExpression->info->qualifiers));
                }
            }
        }

        if (const auto destinationTypeDefinition = tryGetResolvedIdentifierTypeDefinition(destinationType)) {
            if (const auto sourceExpressionTypeDefinition = tryGetResolvedIdentifierTypeDefinition(sourceExpressionType)) {
                // If source expression type and definition type are same type, return that type.
                if (sourceExpressionTypeDefinition == destinationTypeDefinition) {
                    return sourceExpression->clone();
                }

                // Are they different types, but both integers?
                if (isIntegerType(sourceExpressionType) && isIntegerType(destinationType)) {
                    // If source expression type is iexpr and destination type is some bounded integer type, attempt to narrow to destination type.
                    if (sourceExpressionTypeDefinition->variant.is<Definition::BuiltinIntegerExpressionType>()) {
                        if (const auto integerLiteral = sourceExpression->tryGet<Expression::IntegerLiteral>()) {
                            if (const auto builtinIntegerType = destinationTypeDefinition->variant.tryGet<Definition::BuiltinIntegerType>()) {            
                                if (integerLiteral->value >= builtinIntegerType->min
                                && integerLiteral->value <= builtinIntegerType->max) {
                                    return makeFwdUnique<const Expression>(Expression::IntegerLiteral(integerLiteral->value), sourceExpression->location,
                                        ExpressionInfo(EvaluationContext::CompileTime, destinationType->clone(), Qualifiers::None));
                                }
                            }
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    bool Compiler::isTypeEquivalent(const TypeExpression* leftTypeExpression, const TypeExpression* rightTypeExpression) const {
        if (leftTypeExpression == nullptr || rightTypeExpression == nullptr) {
            return false;
        }

        if (const auto rightDesignatedStorageType = rightTypeExpression->tryGet<TypeExpression::DesignatedStorage>()) {
            const auto leftSize = calculateStorageSize(leftTypeExpression, ""_sv);
            const auto rightSize = calculateStorageSize(rightDesignatedStorageType->elementType.get(), ""_sv);

            return leftSize.hasValue() && rightSize.hasValue() && *leftSize == *rightSize;
        }

        switch (leftTypeExpression->kind) {
            case TypeExpressionKind::Array: {
                const auto& leftArrayType = leftTypeExpression->array;
                if (const auto rightArrayType = rightTypeExpression->tryGet<TypeExpression::Array>()) {

                    if (leftArrayType.size && rightArrayType->size) {
                        const auto leftSize = leftArrayType.size->integerLiteral.value;
                        const auto rightSize = rightArrayType->size->integerLiteral.value;

                        if (leftSize != rightSize) {
                            return false;
                        }
                    }
                    return isTypeEquivalent(leftArrayType.elementType.get(), rightArrayType->elementType.get());
                }
                return false;
            }
            case TypeExpressionKind::DesignatedStorage: {
                const auto& leftDesignatedStorageType = leftTypeExpression->designatedStorage;
                const auto leftSize = calculateStorageSize(leftDesignatedStorageType.elementType.get(), ""_sv);
                const auto rightSize = calculateStorageSize(rightTypeExpression, ""_sv);

                return leftSize.hasValue() && rightSize.hasValue() && *leftSize == *rightSize;
            }
            case TypeExpressionKind::Function: {
                const auto& leftFunctionType = leftTypeExpression->function;
                if (const auto rightFunctionType = rightTypeExpression->tryGet<TypeExpression::Function>()) {
                    const auto& leftParameterTypes = leftFunctionType.parameterTypes;
                    const auto& rightParameterTypes = rightFunctionType->parameterTypes;

                    if (!isTypeEquivalent(leftFunctionType.returnType.get(), rightFunctionType->returnType.get())) {
                        return false;
                    }
                    if (leftParameterTypes.size() != rightParameterTypes.size()) {
                        return false;
                    }
                    for (std::size_t i = 0; i != leftParameterTypes.size(); ++i) {
                        if (!isTypeEquivalent(leftParameterTypes[i].get(), rightParameterTypes[i].get())) {
                            return false;
                        }
                    }
                    return true;
                }
                return false;
            }
            case TypeExpressionKind::Identifier: std::abort(); return false;
            case TypeExpressionKind::Pointer: {
                const auto& leftPointerType = leftTypeExpression->pointer;
                if (const auto rightPointerType = rightTypeExpression->tryGet<TypeExpression::Pointer>()) {
                    return isTypeEquivalent(leftPointerType.elementType.get(), rightPointerType->elementType.get())
                        && leftPointerType.qualifiers == rightPointerType->qualifiers;
                }
                return false;
            }
            case TypeExpressionKind::ResolvedIdentifier: {
                const auto& leftResolvedIdentifierType = leftTypeExpression->resolvedIdentifier;
                if (const auto rightResolvedIdentifierType = rightTypeExpression->tryGet<TypeExpression::ResolvedIdentifier>()) {
                    return leftResolvedIdentifierType.definition == rightResolvedIdentifierType->definition;
                }
                return false;
            }
            case TypeExpressionKind::Tuple: {
                const auto& leftTuple = leftTypeExpression->tuple;
                if (const auto rightTuple = rightTypeExpression->tryGet<TypeExpression::Tuple>()) {
                    const auto& leftElementTypes = leftTuple.elementTypes;
                    const auto& rightElementTypes = rightTuple->elementTypes;

                    if (leftElementTypes.size() != rightElementTypes.size()) {
                        return false;
                    }
                    for (std::size_t i = 0; i != leftElementTypes.size(); ++i) {
                        if (!isTypeEquivalent(leftElementTypes[i].get(), rightElementTypes[i].get())) {
                            return false;
                        }
                    }
                    return true;
                }
                return false;
            }
            case TypeExpressionKind::TypeOf:  return false;
            default: std::abort(); return false;
        }
    }

    std::string Compiler::getTypeName(const TypeExpression* typeExpression) const {
        if (typeExpression == nullptr) {
            return "<unknown type>";
        }

        switch (typeExpression->kind) {
            case TypeExpressionKind::Array: {
                const auto& arrayType = typeExpression->array;
                std::string result = "[" + getTypeName(arrayType.elementType.get());
                if (arrayType.size != nullptr) {
                    result += "; ";
                    if (arrayType.size->kind == ExpressionKind::IntegerLiteral) {
                        result += arrayType.size->integerLiteral.value.toString();
                    } else {
                        result += "...";
                    }
                }
                return result + "]";
            }
            case TypeExpressionKind::DesignatedStorage: {
                const auto& designatedStorageType = typeExpression->designatedStorage;

				std::string result = getTypeName(designatedStorageType.elementType.get()) + " in ";
				if (const auto& holder = designatedStorageType.holder) {
					if (const auto& resolvedIdentifier = holder->tryGet<Expression::ResolvedIdentifier>()) {
						result += getResolvedIdentifierName(resolvedIdentifier->definition, resolvedIdentifier->pieces);
						return result;
					} else if (const auto& identifier = holder->tryGet<Expression::Identifier>()) {
						const auto& pieces = identifier->pieces;
						result += text::join(pieces.begin(), pieces.end(), ".");
						return result;
					}
				}

				result += "<designated storage>"; 
				return result;
            }
            case TypeExpressionKind::Function: {
                const auto& functionType = typeExpression->function;
                auto result = std::string(functionType.far ? "far " : "") + "func";
                const auto& parameterTypes = functionType.parameterTypes;
                if (parameterTypes.size() > 0) {
                    result += "(";
                    for (std::size_t i = 0; i != parameterTypes.size(); ++i) {
                        result += (i != 0 ? ", " : "") + getTypeName(parameterTypes[i].get());
                    }
                    result += ")";
                }

                const auto& returnType = functionType.returnType;
                if (returnType->kind != TypeExpressionKind::Tuple
                || returnType->tuple.elementTypes.size() != 0) {
                    result += " : " + getTypeName(functionType.returnType.get());
                }
                return result;
            }
            case TypeExpressionKind::Identifier: {
                const auto& identifierType = typeExpression->identifier;
                const auto& pieces = identifierType.pieces;
                return text::join(pieces.begin(), pieces.end(), ".");
            }
            case TypeExpressionKind::Pointer: {
                const auto& pointerType = typeExpression->pointer;
                return 
                    std::string(((pointerType.qualifiers & Qualifiers::Far) != Qualifiers::None) ? "far " : "")
                    + "*"
                    + std::string(
                        ((pointerType.qualifiers & Qualifiers::Const) != Qualifiers::None) ? "const "
                        : ((pointerType.qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) ? "writeonly "
                        : "")
                    + getTypeName(pointerType.elementType.get());
            }
            case TypeExpressionKind::ResolvedIdentifier: {
                const auto& resolvedIdentifierType = typeExpression->resolvedIdentifier;
                const auto& pieces = resolvedIdentifierType.pieces;
                if (pieces.size() > 0) {
                    return text::join(pieces.begin(), pieces.end(), ".");
                }
                return resolvedIdentifierType.definition->name.toString();
            }
            case TypeExpressionKind::Tuple: {
                const auto& tupleType = typeExpression->tuple;
                std::string result = "(";
                const auto& elementTypes = tupleType.elementTypes;
                for (std::size_t i = 0; i != elementTypes.size(); ++i) {
                    result += (i != 0 ? ", " : "") + getTypeName(elementTypes[i].get());
                }
                result += ")";
                return result;
            }
            case TypeExpressionKind::TypeOf: return "`typeof`";
            default: std::abort(); return "";
        }
    }

    Optional<std::size_t> Compiler::calculateStorageSize(const TypeExpression* typeExpression, StringView description) const {
        if (typeExpression == nullptr) {
            return Optional<std::size_t>();
        }

        switch (typeExpression->kind) {
            case TypeExpressionKind::Array: {
                const auto& arrayType = typeExpression->array;
                if (arrayType.size != nullptr) {
                    const auto elementSize = calculateStorageSize(arrayType.elementType.get(), description);
                    if (elementSize.hasValue()) {
                        const auto arraySize = arrayType.size->integerLiteral.value;

                        if (arraySize >= Int128(0) && arraySize <= Int128(SIZE_MAX)) {
                            const auto checkedArraySize = static_cast<std::size_t>(arraySize);

                            if (checkedArraySize == 0 || elementSize.get() * SIZE_MAX / checkedArraySize) {
                                return Optional<std::size_t>(elementSize.get() * checkedArraySize);
                            }
                        }

                        if (description.getLength() > 0) {
                            report->error("array length of `" + arraySize.toString() + "` is too large to be used for " + description.toString(), typeExpression->location);
                        }
                    }
                } else {
                    if (description.getLength() > 0) {
                        report->error("could not resolve length for implicitly-sized array used for " + description.toString(), typeExpression->location);
                    }
                }
                return Optional<std::size_t>();
            }
            case TypeExpressionKind::DesignatedStorage: return Optional<std::size_t>();
            case TypeExpressionKind::Function: {
                const auto& functionType = typeExpression->function;
                const auto pointerSizedType = functionType.far ? platform->getFarPointerSizedType() : platform->getPointerSizedType();
                return Optional<std::size_t>(pointerSizedType->variant.get<Definition::BuiltinIntegerType>().size);
            }
            case TypeExpressionKind::Identifier: std::abort(); return Optional<std::size_t>();
            case TypeExpressionKind::Pointer: {
                const auto& pointerType = typeExpression->pointer;
                const auto pointerSizedType = (pointerType.qualifiers & Qualifiers::Far) != Qualifiers::None ? platform->getFarPointerSizedType() : platform->getPointerSizedType();
                return Optional<std::size_t>(pointerSizedType->variant.get<Definition::BuiltinIntegerType>().size);
            }
            case TypeExpressionKind::ResolvedIdentifier: {
                const auto& resolvedIdentifierType = typeExpression->resolvedIdentifier;
                const auto definition = resolvedIdentifierType.definition;
                if (resolvedIdentifierType.definition->variant.is<Definition::BuiltinBoolType>()) {
                    return Optional<std::size_t>(1);
                } else if (const auto builtinIntegerType = resolvedIdentifierType.definition->variant.tryGet<Definition::BuiltinIntegerType>()) {
                    return Optional<std::size_t>(builtinIntegerType->size);
                } else if (const auto enumType = resolvedIdentifierType.definition->variant.tryGet<Definition::Enum>()) {
                    if (enumType->resolvedUnderlyingType != nullptr) {
                        return calculateStorageSize(enumType->resolvedUnderlyingType.get(), description);
                    }
                } else if (const auto structType = resolvedIdentifierType.definition->variant.tryGet<Definition::Struct>()) {
                    if (structType->size) {
                        return Optional<std::size_t>(structType->size);
                    }
                }

                if (description.getLength() > 0) {
                    report->error("type `" + definition->name.toString() + "` has unknown storage size, so it cannot be used for " + description.toString(), typeExpression->location);
                }
                return Optional<std::size_t>();
            }
            case TypeExpressionKind::Tuple: {
                const auto& tupleType = typeExpression->tuple;
                std::size_t result = 0;
                const auto& elementTypes = tupleType.elementTypes;

                for (std::size_t i = 0; i != elementTypes.size(); ++i) {
                    auto elementSize = calculateStorageSize(elementTypes[i].get(), description);
                    if (elementSize.hasValue()) {
                        if (SIZE_MAX - result < elementSize.get()) {
                            if (description.getLength() > 0) {
                                report->error("tuple size is too large to be calculated for " + description.toString(), typeExpression->location);
                            }
                            return Optional<std::size_t>();
                        } else {
                            result += elementSize.get();
                        }
                    } else {
                        return Optional<std::size_t>();
                    }
                }

                return Optional<std::size_t>(result);
            }
            case TypeExpressionKind::TypeOf: return Optional<std::size_t>();
            default: std::abort(); return Optional<std::size_t>();
        }
    }

    Optional<std::size_t> Compiler::resolveExplicitAddressExpression(const Expression* expression) {
        if (expression != nullptr) {
            if (const auto reducedAddressExpression = reduceExpression(expression)) {
                if (const auto addressLiteral = reducedAddressExpression->tryGet<Expression::IntegerLiteral>()) {
                    if (addressLiteral->value.isNegative()) {
                        report->error("address must be a non-negative integer, but got `" + addressLiteral->value.toString() + "` instead", reducedAddressExpression->location);
                    } else {
                        const auto maxPointerSizedType = platform->getFarPointerSizedType();
                        const auto addressMax = Int128((1U << (8U * maxPointerSizedType->variant.get<Definition::BuiltinIntegerType>().size)) - 1);
                        if (addressLiteral->value > addressMax) {
                            report->error("address of `0x" + addressLiteral->value.toString(16) + "` is outside the valid address range `0` .. `0x" + addressMax.toString(16) + "` supported by this platform.", reducedAddressExpression->location);
                        } else {
                            return static_cast<std::size_t>(addressLiteral->value);
                        }
                    }
                } else {
                    report->error("address must be a compile-time integer literal", reducedAddressExpression->location);
                }
            }
        }

        return Optional<std::size_t>();
    }

    bool Compiler::serializeInteger(Int128 value, std::size_t size, std::vector<std::uint8_t>& result) const {
        // TODO: handle big-endian
        switch (size) {
            case 1: {
                result.push_back(static_cast<std::uint8_t>(value));
                return true;                            
            }
            case 2: {
                auto x = static_cast<std::uint16_t>(value);
                result.push_back(x & 0xFF);
                x >>= 8; result.push_back(x & 0xFF);
                return true;                            
            }
            case 4: {
                auto x = static_cast<std::uint32_t>(value);
                result.push_back(x & 0xFF);
                x >>= 8; result.push_back(x & 0xFF);
                x >>= 8; result.push_back(x & 0xFF);
                x >>= 8; result.push_back(x & 0xFF);
                return true;
            }
            case 8: {
                auto x = static_cast<std::uint64_t>(value);
                result.push_back(x & 0xFF);
                x >>= 8; result.push_back(x & 0xFF);
                x >>= 8; result.push_back(x & 0xFF);
                x >>= 8; result.push_back(x & 0xFF);
                x >>= 8; result.push_back(x & 0xFF);
                x >>= 8; result.push_back(x & 0xFF);
                x >>= 8; result.push_back(x & 0xFF);
                x >>= 8; result.push_back(x & 0xFF);
                return true;                            
            }
            default: return false;
        }
    }


    bool Compiler::serializeConstantInitializer(const Expression* expression, std::vector<std::uint8_t>& result) const {
        // NOTE: this requires a fully-reduced literal value expression.
        // All identifiers, operators, embeds, etc. must be substituted with a reduced literal values.
        // Otherwise, it cannot be serialized.
        switch (expression->kind) {
            case ExpressionKind::ArrayComprehension: return false;
            case ExpressionKind::ArrayPadLiteral: return false;
            case ExpressionKind::ArrayLiteral: {
                const auto& arrayLiteral = expression->arrayLiteral;
                for (const auto& item : arrayLiteral.items) {
                    if (!serializeConstantInitializer(item.get(), result)) {
                        return false;
                    }
                }
                return true;
            }
            case ExpressionKind::BinaryOperator: return false;
            case ExpressionKind::BooleanLiteral: {
                const auto& booleanLiteral = expression->booleanLiteral;
                return serializeInteger(Int128(booleanLiteral.value ? 1 : 0), 1, result);
            }
            case ExpressionKind::Call: return false;
            case ExpressionKind::Cast: return false;
            case ExpressionKind::Embed: return false;
            case ExpressionKind::FieldAccess: return false;
            case ExpressionKind::Identifier: std::abort(); return false;
            case ExpressionKind::IntegerLiteral: {
                const auto& integerLiteral = expression->integerLiteral;
                if (const auto storageSize = calculateStorageSize(expression->info->type.get(), "integer literal"_sv)) {
                    return serializeInteger(integerLiteral.value, *storageSize, result);
                }
                return false;
            }
            case ExpressionKind::OffsetOf: std::abort(); return false;
            case ExpressionKind::RangeLiteral: return false;
            case ExpressionKind::ResolvedIdentifier: {
                const auto& resolvedIdentifier = expression->resolvedIdentifier;
                const auto definition = resolvedIdentifier.definition;

                Optional<std::size_t> absolutePosition;
                if (const auto funcDefinition = definition->variant.tryGet<Definition::Func>()) {
                    if (funcDefinition->inlined) {
                        report->error("`inline func` has no address so it cannot be used as a constant initializer", expression->location);
                    } else {
                        if (const auto address = funcDefinition->address.tryGet()) {
                            absolutePosition = address->absolutePosition;
                        }
                    }
                }

                if (absolutePosition.hasValue()) {
                    return serializeInteger(Int128(*absolutePosition), platform->getPointerSizedType()->variant.get<Definition::BuiltinIntegerType>().size, result);
                }
                return false;
            }
            case ExpressionKind::SideEffect: return false;
            case ExpressionKind::StringLiteral: {
                const auto& stringLiteral = expression->stringLiteral;
                const auto data = stringLiteral.value.getData();
                const auto len = stringLiteral.value.getLength();
                for (std::size_t i = 0; i != len; ++i) {
                    result.push_back(data[i]);
                }
                return true;
            }
            case ExpressionKind::StructLiteral: {
                const auto& structLiteral = expression->structLiteral;
                const auto definition = structLiteral.type->resolvedIdentifier.definition;
                const auto& structDefinition = definition->variant.get<Definition::Struct>();                
                const auto sizeBefore = result.size();


                if (structDefinition.kind == StructKind::Union) {
                    const auto& item = structLiteral.items.begin();
                    if (!serializeConstantInitializer(item->second->value.get(), result)) {
                        return false;
                    }

                    const auto sizeAfter = result.size();

                    // Pad unions to the size of their largest element.
                    if (structLiteral.items.size() > sizeAfter - sizeBefore) {
                        const auto trailingPadding = structLiteral.items.size() - (sizeAfter - sizeBefore);

                        for (std::size_t i = 0; i != trailingPadding; ++i) {
                            result.push_back(0);
                        }
                    }
                } else {
                    for (const auto& member : structDefinition.members) {
                        const auto& item = structLiteral.items.find(member->name);
                        if (!serializeConstantInitializer(item->second->value.get(), result)) {
                            return false;
                        }
                    }
                    // TODO: alignment/padding as required for structs.                    
                }

                return true;
            }
            case ExpressionKind::TupleLiteral: {
                const auto& tupleLiteral = expression->tupleLiteral;
                // TODO: alignment/padding as required for tuples.
                for (const auto& item : tupleLiteral.items) {
                    if (!serializeConstantInitializer(item.get(), result)) {
                        return false;
                    }
                }
                return true;
            }
            case ExpressionKind::TypeOf: return false;
            case ExpressionKind::TypeQuery: std::abort(); return false;
            case ExpressionKind::UnaryOperator: return false;
            default: std::abort(); return false;
        }
    }


    std::pair<bool, Optional<std::size_t>> Compiler::handleInStatement(const std::vector<StringView>& bankIdentifierPieces, const Expression* dest, SourceLocation location) {
        const auto resolveResult = resolveIdentifier(bankIdentifierPieces, location);
        if (resolveResult.first == nullptr) {
            return {false, Optional<std::size_t>()};
        }
        if (resolveResult.second < bankIdentifierPieces.size() - 1) {
            raiseUnresolvedIdentifierError(bankIdentifierPieces, resolveResult.second, location);
            return {false, Optional<std::size_t>()};
        }

        if (const auto definition = resolveResult.first) {
            if (auto bankDefinition = definition->variant.tryGet<Definition::Bank>()) {
                currentBank = bankDefinition->bank;

                if (dest != nullptr) {
                    if (const auto reducedAddressExpression = reduceExpression(dest)) {
                        if (const auto addressLiteral = reducedAddressExpression->tryGet<Expression::IntegerLiteral>()) {
                            if (addressLiteral->value.isNegative()) {
                                report->error("address must be a non-negative integer, but got `" + addressLiteral->value.toString() + "` instead", reducedAddressExpression->location);
                            } else {
                                const auto oldPosition = currentBank->getAddress().absolutePosition;
                                const auto maxPointerSizedType = platform->getFarPointerSizedType();
                                const auto addressMax = Int128((1U << (8U * maxPointerSizedType->variant.get<Definition::BuiltinIntegerType>().size)) - 1);

                                if (addressLiteral->value > addressMax) {
                                    report->error("address of `0x" + addressLiteral->value.toString(16) + "` is outside the address range `0` .. `0x" + addressMax.toString(16) + "` supported by this platform.", reducedAddressExpression->location);
                                } else if (!oldPosition.hasValue() && addressLiteral->value + Int128(currentBank->getCapacity() - 1) > addressMax) {
                                    report->error("bank start address of `0x" + addressLiteral->value.toString(16) + "` with size `" + Int128(currentBank->getCapacity()).toString() + "` will cause upper address `0x" + (addressLiteral->value + Int128(currentBank->getCapacity() - 1)).toString(16) + "` to be outside the valid address range `0` .. `0x" + addressMax.toString(16) + "` supported by this platform.", reducedAddressExpression->location);
                                } else {
                                    currentBank->absoluteSeek(report, static_cast<std::size_t>(addressLiteral->value), reducedAddressExpression->location);
                                    return {true, static_cast<std::size_t>(addressLiteral->value)};
                                }
                            }
                        } else {
                            report->error("address must be a compile-time integer literal", reducedAddressExpression->location);
                            return {false, Optional<std::size_t>()};
                        }
                    }
                } else {
                    return {true, Optional<std::size_t>()};
                }
            } else {
                report->error(getResolvedIdentifierName(definition, bankIdentifierPieces) + " is not a valid bank", definition->name);
                return {false, Optional<std::size_t>()};
            }
        }
        return {false, Optional<std::size_t>()};
    }

    void Compiler::pushAttributeList(CompiledAttributeList* attributeList) {
        modeFlagsStack.push_back(modeFlags);
        attributeListStack.push_back(attributeList);

        for (const auto& attribute : attributeList->attributes) {
            attributeStack.push_back(attribute.get());

            const auto attributeName = attribute->name;
            const auto modeIndex = builtins.findModeAttributeByName(attributeName);
            if (modeIndex != SIZE_MAX) {
                const auto modeAttribute = builtins.getModeAttribute(modeIndex);

                for (std::size_t otherModeIndex = 0, modeCount = builtins.getModeAttributeCount(); otherModeIndex != modeCount; ++otherModeIndex) {
                    if ((modeFlags & (1U << otherModeIndex)) != 0) {
                        const auto otherModeAttribute = builtins.getModeAttribute(otherModeIndex);
                        if (otherModeAttribute->groupIndex == modeAttribute->groupIndex) {
                            modeFlags &= ~(1U << otherModeIndex);
                        }
                    }
                }

                modeFlags |= (1U << modeIndex);
            }
        }
    }

    void Compiler::popAttributeList() {
        modeFlags = modeFlagsStack.back();
        modeFlagsStack.pop_back();

        const auto attributeList = attributeListStack.back();
        attributeListStack.pop_back();

        for (std::size_t i = 0, size = attributeList->attributes.size(); i != size; ++i) {
            attributeStack.pop_back();
        }
    }

    bool Compiler::checkConditionalCompilationAttributes() {
        for (const auto& attribute : attributeStack) {
            if (attribute->name == "compile_if"_sv) {
                if (attribute->arguments.size() == 1) {
                    if (const auto booleanLiteral = attribute->arguments[0]->tryGet<Expression::BooleanLiteral>()) {
                        if (booleanLiteral->value == false) {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool Compiler::reserveDefinitions(const Statement* statement) {
        switch (statement->kind) {
            case StatementKind::Attribution: {
                const auto& attributedStatement = statement->attribution;
                const auto body = attributedStatement.body.get();

                auto attributeList = attributeLists.addNew();
                statementAttributeLists[statement] = attributeList;

                for (const auto& attribute : attributedStatement.attributes) {
                    std::vector<FwdUniquePtr<const Expression>> reducedArguments;

                    bool foundAttribute = false;
                    bool validAttributeName = false;
                    std::size_t attributeRequiredArgumentCount = 0;

                    const auto modeAttribute = builtins.findModeAttributeByName(attribute->name);
                    const auto declarationAttribute = builtins.findDeclarationAttributeByName(attribute->name);

                    if (modeAttribute != SIZE_MAX) {
                        foundAttribute = true;
                        validAttributeName = true;
                        attributeRequiredArgumentCount = 0;
                    } else if (declarationAttribute != Builtins::DeclarationAttribute::None) {
                        foundAttribute = true;
                        validAttributeName = builtins.isDeclarationAttributeValid(declarationAttribute, body);
                        attributeRequiredArgumentCount = builtins.getDeclarationAttributeArgumentCount(declarationAttribute);
                    } else {
                        if (attribute->name == "compile_if"_sv) {
                            foundAttribute = true;
                            validAttributeName = true;
                            attributeRequiredArgumentCount = 1;
                        }
                    }

                    bool validAttributeArguments = true;

                    if (foundAttribute && attribute->arguments.size() != attributeRequiredArgumentCount) {
                        report->error("attribute `" + attribute->name.toString()
                            + "` expects exactly " + std::to_string(attributeRequiredArgumentCount)
                            + " argument" + (attributeRequiredArgumentCount != 1 ? "s" : "")
                            + ", but got " + std::to_string(attribute->arguments.size())
                            + " argument" + (attribute->arguments.size() != 1 ? "s" : "")
                            + " instead", attribute->location);

                        validAttributeArguments = false;
                    }

                    if (validAttributeArguments && attribute->arguments.size() > 0) {
                        for (const auto& argument : attribute->arguments) {
                            if (auto reducedArgument = reduceExpression(argument.get())) {
                                reducedArguments.push_back(std::move(reducedArgument));
                            } else {
                                validAttributeArguments = false;
                            }
                        }
                    }

                    if (validAttributeName && validAttributeArguments) {
                        if (attribute->name == "compile_if"_sv) {
                            if (attribute->arguments.size() == 1) {
                                if (reducedArguments[0]->kind != ExpressionKind::BooleanLiteral) {
                                    report->error("attribute `" + attribute->name.toString() + "` requires a compile-time boolean conditional.", attribute->location);
                                }
                            }
                        }

                        attributeList->attributes.addNew(body, attribute->name, std::move(reducedArguments), attribute->location);
                    } else {
                        if (foundAttribute) {
                            if (!validAttributeName) {
                                report->error("attribute `" + attribute->name.toString() + "` is not valid here.", attribute->location);
                            }
                        } else {
                            report->error("could not resolve attribute `" + attribute->name.toString() + "`", attribute->location);
                        }
                    }
                }

                pushAttributeList(attributeList);
                if (checkConditionalCompilationAttributes()) {
                    reserveDefinitions(attributedStatement.body.get());
                }
                popAttributeList();
                break;
            }
            case StatementKind::Bank: {
                const auto& bankDeclaration = statement->bank;
                const auto& names = bankDeclaration.names;
                const auto& addresses = bankDeclaration.addresses;
                const auto typeExpression = bankDeclaration.typeExpression.get();
                for (std::size_t i = 0, size = names.size(); i != size; ++i) {
                    definitionsToResolve.push_back(currentScope->createDefinition(report, Definition::Bank(addresses[i].get(), typeExpression), names[i], statement));
                }
                break;
            }
            case StatementKind::Block: {
                const auto& blockStatement = statement->block;
                enterScope(getOrCreateStatementScope(StringView(), statement, currentScope));
                for (const auto& item : blockStatement.items) {
                    reserveDefinitions(item.get());
                }
                exitScope();
                break;
            }
            case StatementKind::Config: break;
            case StatementKind::DoWhile: {
                const auto& doWhileStatement = statement->doWhile;
                reserveDefinitions(doWhileStatement.body.get());
                break;
            }
            case StatementKind::Enum: {
                const auto& enumDeclaration = statement->enum_;
                const auto scope = getOrCreateStatementScope(StringView(), statement, currentScope);

                auto definition = currentScope->createDefinition(report, Definition::Enum(enumDeclaration.underlyingTypeExpression.get(), scope), enumDeclaration.name, statement);

                if (definition == nullptr) {
                    break;
                }

                definitionsToResolve.push_back(definition);

                auto& enumDefinition = definition->variant.get<Definition::Enum>();

                enterScope(scope);

                const Expression* previousExpression = nullptr;
                std::size_t offset = 0;

                for (const auto& item : enumDeclaration.items) {
                    const Expression* expression = nullptr;

                    if (const auto enumExpression = item->value.get()) {
                        expression = enumExpression;
                        previousExpression = enumExpression;
                        offset = 0;
                    } else {
                        expression = previousExpression;
                    }

                    auto enumMemberDefinition = currentScope->createDefinition(report, Definition::EnumMember(expression, offset), item->name, statement);
                    enumDefinition.members.push_back(enumMemberDefinition);
                    ++offset;
                }

                exitScope();
                break;
            }
            case StatementKind::ExpressionStatement: break;
            case StatementKind::File: {
                const auto& file = statement->file;
                const auto outerScope = currentScope;
                enterScope(getOrCreateStatementScope(StringView(), statement, builtins.getBuiltinScope()));
                bindModuleScope(file.expandedPath, currentScope);
                for (const auto& item : file.items) {
                    reserveDefinitions(item.get());
                }
                if (outerScope != nullptr) {
                    outerScope->addRecursiveImport(currentScope);
                }
                exitScope();
                break;
            }
            case StatementKind::For: {
                const auto& forStatement = statement->for_;
                reserveDefinitions(forStatement.body.get());
                break;
            }
            case StatementKind::Func: {
                const auto& funcDeclaration = statement->func;
                const auto oldFunction = currentFunction;
                const auto onExit = makeScopeGuard([&]() {
                    currentFunction = oldFunction;
                });

                bool fallthrough = false;
                BranchKind returnKind = funcDeclaration.far ? BranchKind::FarReturn : BranchKind::Return;
                for (const auto& attribute : attributeStack) {
                    if (attribute->statement == statement) {
                        const auto functionAttribute = builtins.findDeclarationAttributeByName(attribute->name);
                        switch (functionAttribute) {
                            case Builtins::DeclarationAttribute::Irq: returnKind = BranchKind::IrqReturn; break;
                            case Builtins::DeclarationAttribute::Nmi: returnKind = BranchKind::NmiReturn; break;
                            case Builtins::DeclarationAttribute::Fallthrough: fallthrough = true; break;
                            case Builtins::DeclarationAttribute::None: break;
                            default: std::abort(); break;
                        }
                    }
                }

                if (funcDeclaration.inlined) {
                    if (returnKind != BranchKind::Return) {
                        report->error("`inline func` cannot have an attribute that changes its return convention", statement->location);    
                    }

                    returnKind = BranchKind::None;
                }

                auto body = funcDeclaration.body.get();
                auto definition = currentScope->createDefinition(report, Definition::Func(fallthrough, funcDeclaration.inlined, funcDeclaration.far, returnKind, funcDeclaration.returnTypeExpression.get(), currentScope, body), funcDeclaration.name, statement);
                definitionsToResolve.push_back(definition);

                if (definition == nullptr) {
                    break;
                }

                auto& funcDefinition = definition->variant.get<Definition::Func>();

                enterScope(getOrCreateStatementScope(StringView(), body, currentScope));
                for (const auto& parameter : funcDeclaration.parameters) {
                    funcDefinition.parameters.push_back(currentScope->createDefinition(report, Definition::Var(Qualifiers::None, definition, nullptr, parameter->typeExpression.get(), 0), parameter->name, statement));
                }
                exitScope();

                currentFunction = definition;
                reserveDefinitions(body);
                break;
            }
            case StatementKind::If: {
                const auto& ifStatement = statement->if_;
                reserveDefinitions(ifStatement.body.get());
                if (ifStatement.alternative) {
                    reserveDefinitions(ifStatement.alternative.get());
                }
                break;
            }
            case StatementKind::In: {
                const auto& inStatement = statement->in;
                bindStatementScope(inStatement.body.get(), currentScope);
                reserveDefinitions(inStatement.body.get());
                break;
            }
            case StatementKind::InlineFor: break;
            case StatementKind::ImportReference: {
                const auto& importReference = statement->importReference;
                if (currentScope != nullptr) {                    
                    if (const auto moduleScope = findModuleScope(importReference.expandedPath)) {
                        currentScope->addRecursiveImport(moduleScope);
                    } else {
                        report->error("import reference appeared before a file node actually registered the module", statement->location, ReportErrorFlags::InternalError);
                    }
                }
                break;
            }
            case StatementKind::InternalDeclaration: break;
            case StatementKind::Branch: break;
            case StatementKind::Label: {
                const auto& labelDeclaration = statement->label;
                auto definition = currentScope->createDefinition(report, Definition::Func(true, false, labelDeclaration.far, BranchKind::None, builtins.getUnitTuple(), currentScope, nullptr), labelDeclaration.name, statement);

                if (definition == nullptr) {
                    break;
                }

                auto& func = definition->variant.get<Definition::Func>();
                func.resolvedSignatureType = makeFwdUnique<const TypeExpression>(TypeExpression::Function(labelDeclaration.far, {}, func.returnTypeExpression->clone()), func.returnTypeExpression->location);
                break;
            }
            case StatementKind::Let: {
                const auto& letDeclaration = statement->let;
                currentScope->createDefinition(report, Definition::Let(letDeclaration.parameters, letDeclaration.value.get()), letDeclaration.name, statement);
                break;
            }
            case StatementKind::Namespace: {
                const auto& namespaceDeclaration = statement->namespace_;
                SymbolTable* scope = nullptr;
                if (const auto definition = currentScope->findLocalMemberDefinition(namespaceDeclaration.name)) {
                    if (const auto ns = definition->variant.tryGet<Definition::Namespace>()) {
                        // Reuse scope if it already exists.
                        scope = ns->environment;
                    } else {
                        // If another symbol with the same name exists, but it is not a namespace, trigger a duplicate key error.
                        currentScope->createDefinition(report, Definition::Namespace(nullptr), namespaceDeclaration.name, statement);
                        break;
                    }
                } else {
                    scope = getOrCreateStatementScope(namespaceDeclaration.name, statement, currentScope);
                    currentScope->createDefinition(report, Definition::Namespace(scope), namespaceDeclaration.name, statement);

                    tempImportedDefinitions.clear();
                    currentScope->findImportedMemberDefinitions(namespaceDeclaration.name, tempImportedDefinitions);
                    for (const auto importedDefinition : tempImportedDefinitions) {
                        if (const auto ns = importedDefinition->variant.tryGet<Definition::Namespace>()) {
                            scope->addRecursiveImport(ns->environment);
                        }
                    }
                }

                enterScope(scope);
                bindStatementScope(namespaceDeclaration.body.get(), scope);
                reserveDefinitions(namespaceDeclaration.body.get());
                exitScope();
                break;
            }
            case StatementKind::Struct: {
                const auto& structDeclaration = statement->struct_;
                const auto scope = getOrCreateStatementScope(StringView(), statement, currentScope);

                auto definition = currentScope->createDefinition(report, Definition::Struct(structDeclaration.kind, scope), structDeclaration.name, statement);
                definitionsToResolve.push_back(definition);

                if (definition == nullptr) {
                    break;
                }

                auto& structDefinition = definition->variant.get<Definition::Struct>();

                enterScope(scope);

                for (const auto& item : structDeclaration.items) {
                    auto structMemberDefinition = currentScope->createDefinition(report, Definition::StructMember(item->typeExpression.get()), item->name, statement);
                    structDefinition.members.push_back(structMemberDefinition);
                }

                exitScope();
                break;
            }
            case StatementKind::TypeAlias: {
                const auto& typeAliasDeclaration = statement->typeAlias;
                definitionsToResolve.push_back(currentScope->createDefinition(report, Definition::TypeAlias(typeAliasDeclaration.typeExpression.get()), typeAliasDeclaration.name, statement));
                break;
            }
            case StatementKind::Var: {
                const auto& varDeclaration = statement->var;
                const auto& names = varDeclaration.names;
                const auto& addresses = varDeclaration.addresses;
                const auto typeExpression = varDeclaration.typeExpression.get();

                std::size_t alignment = 0;

                for (const auto& attribute : attributeStack) {
                    if (attribute->statement == statement) {
                        const auto functionAttribute = builtins.findDeclarationAttributeByName(attribute->name);
                        switch (functionAttribute) {
                            case Builtins::DeclarationAttribute::Align: {
                                if (const auto integerLiteral = attribute->arguments[0]->tryGet<Expression::IntegerLiteral>()) {
                                    const auto& value = integerLiteral->value;
                                    if (!value.isNegative() && value <= Int128(SIZE_MAX) && value.isPowerOfTwo()) {
                                        alignment = static_cast<std::size_t>(value);
                                    } else {
                                        report->error("invalid value " + value.toString() + " provided to `align` attribute. must be a positive power-of-two, or zero.", statement->location);
                                    }
                                }
                                break;
                            }
                            case Builtins::DeclarationAttribute::None: break;
                            default: std::abort(); break;
                        }
                    }
                }

                for (std::size_t i = 0, size = names.size(); i != size; ++i) {
                    definitionsToResolve.push_back(currentScope->createDefinition(report, Definition::Var(varDeclaration.qualifiers, currentFunction, addresses[i].get(), typeExpression, alignment), names[i], statement));
                }
                break;
            }
            case StatementKind::While: {
                const auto& whileStatement = statement->while_;
                reserveDefinitions(whileStatement.body.get());
                break;
            }
            default: std::abort(); return false;
        }

        return statement == program.get() ? report->validate() : report->alive();
    }

    bool Compiler::resolveDefinitionTypes() {
        for (auto& definition : definitionsToResolve) {
            if (auto enumDefinition = definition->variant.tryGet<Definition::Enum>()) {
                enterScope(definition->parentScope);

                if (const auto underlyingTypeExpression = enumDefinition->underlyingTypeExpression) {
                    auto resolvedUnderlyingTypeExpression = reduceTypeExpression(underlyingTypeExpression);

                    if (resolvedUnderlyingTypeExpression != nullptr) {
                        if (isIntegerType(resolvedUnderlyingTypeExpression.get())) {
                            enumDefinition->resolvedUnderlyingType = std::move(resolvedUnderlyingTypeExpression);
                        } else {
                            report->error("underlying type for `enum` must be an integer type, not `" + getTypeName(resolvedUnderlyingTypeExpression.get()) + "`", resolvedUnderlyingTypeExpression->location);
                        }
                    }
                }

                Int128 previousValue;
                const Expression* previousExpression = nullptr;                
                FwdUniquePtr<const TypeExpression> enumTypeExpression = makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(definition, {definition->name}), definition->declaration->location);

                enterScope(enumDefinition->environment);
                for (auto member : enumDefinition->members) {
                    auto& enumMemberDefinition = member->variant.get<Definition::EnumMember>();

                    if (enumMemberDefinition.expression == previousExpression) {
                        enumMemberDefinition.reducedExpression = makeFwdUnique<const Expression>(
                            Expression::IntegerLiteral(previousValue + Int128(enumMemberDefinition.offset)),
                            previousExpression != nullptr ? previousExpression->location : enumTypeExpression->location,
                            ExpressionInfo(EvaluationContext::CompileTime, enumTypeExpression->clone(), Qualifiers::None));
                    } else {
                        if (auto reducedExpression = reduceExpression(enumMemberDefinition.expression)) {
                            previousExpression = enumMemberDefinition.expression;

                            if (const auto lit = reducedExpression->tryGet<Expression::IntegerLiteral>()) {
                                previousValue = lit->value;

                                enumMemberDefinition.reducedExpression = makeFwdUnique<const Expression>(
                                    Expression::IntegerLiteral(previousValue + Int128(enumMemberDefinition.offset)),
                                    reducedExpression->location,
                                    ExpressionInfo(EvaluationContext::CompileTime, enumTypeExpression->clone(), Qualifiers::None));
                            } else {
                                report->error("`enum` value must be a compile-time integer literal", enumMemberDefinition.expression->location);
                            }
                        }
                    }
                }
                exitScope();
                exitScope();
            } else if (auto structDefinition = definition->variant.tryGet<Definition::Struct>()) {
                const auto description = structDefinition->kind == StructKind::Struct ? "`struct` member"_sv : "`union` member"_sv;

                enterScope(definition->parentScope);
                enterScope(structDefinition->environment);

                std::size_t offset = 0;
                std::size_t totalSize = 0;
                for (auto member : structDefinition->members) {
                    auto& structMemberDefinition = member->variant.get<Definition::StructMember>();
                    structMemberDefinition.offset = offset;

                    if (auto resolvedType = reduceTypeExpression(structMemberDefinition.typeExpression)) {

                        if (const auto resolvedSize = calculateStorageSize(resolvedType.get(), description)) {
                            if (structDefinition->kind == StructKind::Struct) {
                                offset += *resolvedSize;
                                totalSize += *resolvedSize;
                            } else {
                                totalSize = std::max(totalSize, *resolvedSize);
                            }
                        }
                        structMemberDefinition.resolvedType = std::move(resolvedType);
                    }
                }
                structDefinition->size = totalSize;

                exitScope();
                exitScope();
            } else if (auto typeAliasDefinition = definition->variant.tryGet<Definition::TypeAlias>()) {
                enterScope(definition->parentScope);
                typeAliasDefinition->resolvedType = reduceTypeExpression(typeAliasDefinition->typeExpression);
                exitScope();
            }
        }

        for (auto& definition : definitionsToResolve) {
            if (auto varDefinition = definition->variant.tryGet<Definition::Var>()) {
                enterScope(definition->parentScope);
                if (const auto typeExpression = varDefinition->typeExpression) {
                    varDefinition->reducedTypeExpression = reduceTypeExpression(typeExpression);
                    varDefinition->resolvedType = varDefinition->reducedTypeExpression.get();
                }
                exitScope();
            } else if (auto funcDefinition = definition->variant.tryGet<Definition::Func>()) {
                enterScope(definition->parentScope);

                bool valid = true;

                auto returnType = reduceTypeExpression(funcDefinition->returnTypeExpression);

                if (!returnType) {
                    valid = false;
                }

                std::vector<FwdUniquePtr<const TypeExpression>> parameterTypes;
                parameterTypes.reserve(funcDefinition->parameters.size());

                for (const auto& parameter : funcDefinition->parameters) {
                    auto& parameterDefinition = parameter->variant.get<Definition::Var>();
                    if (auto parameterType = reduceTypeExpression(parameterDefinition.typeExpression)) {
                        parameterDefinition.reducedTypeExpression = parameterType->clone();
                        parameterDefinition.resolvedType = parameterDefinition.reducedTypeExpression.get();
                        parameterTypes.push_back(std::move(parameterType));
                    } else {
                        valid = false;
                    }
                }

                if (valid) {
                    funcDefinition->resolvedSignatureType = makeFwdUnique<const TypeExpression>(TypeExpression::Function(funcDefinition->far, std::move(parameterTypes), std::move(returnType)), definition->declaration->location);
                }
                exitScope();
            } else if (auto bankDefinition = definition->variant.tryGet<Definition::Bank>()) {
                enterScope(definition->parentScope);
                bankDefinition->resolvedType = reduceTypeExpression(bankDefinition->typeExpression);

                const auto origin = resolveExplicitAddressExpression(bankDefinition->addressExpression);

                if (const auto resolvedType = bankDefinition->resolvedType.get()) {
                    bool validBankType = false;

                    if (const auto arrayType = resolvedType->tryGet<TypeExpression::Array>()) {
                        if (const auto elementType = arrayType->elementType->tryGet<TypeExpression::ResolvedIdentifier>()) {
                            if (const auto bankType = elementType->definition->variant.tryGet<Definition::BuiltinBankType>()) {
                                if (arrayType->size) {
                                    if (auto reducedSizeExpression = reduceExpression(arrayType->size.get())) {
                                        if (const auto sizeLiteral = reducedSizeExpression->tryGet<Expression::IntegerLiteral>()) {
                                            validBankType = true;
                                            if (!sizeLiteral->value.isPositive()) {
                                                report->error("bank size must be greater than zero, but got `" + sizeLiteral->value.toString() + "` instead", reducedSizeExpression->location);
                                            } else {
                                                const auto maxPointerSizedType = platform->getFarPointerSizedType();
                                                const auto addressEnd = Int128(1U << (8U * maxPointerSizedType->variant.get<Definition::BuiltinIntegerType>().size));

                                                if (sizeLiteral->value > addressEnd) {
                                                    report->error("bank size of `" + sizeLiteral->value.toString() + "` will cause an upper address outside the valid address range `0` .. `0x" + (addressEnd - Int128::one()).toString(16) + "` supported by this platform.", reducedSizeExpression->location);
                                                } else if (origin.hasValue() && Int128(origin.get()) + sizeLiteral->value > addressEnd) {
                                                    report->error("bank size of `" + sizeLiteral->value.toString() + "` will cause upper address `0x" + (Int128(origin.get() - 1) + sizeLiteral->value).toString(16) + "` to be outside the valid address range `0` .. `0x" + (addressEnd - Int128::one()).toString(16) + "` supported by this platform.", reducedSizeExpression->location);
                                                } else {
                                                    bankDefinition->bank = registeredBanks.addNew(
                                                        definition->name,
                                                        bankType->kind,
                                                        origin,
                                                        static_cast<std::size_t>(sizeLiteral->value),
                                                        Bank::DefaultPadValue);
                                                }
                                            }
                                        } else {
                                            report->error("invalid size expression in bank type", resolvedType->location);
                                        }
                                    }
                                } else {
                                    validBankType = true;
                                    report->error("bank type `" + getTypeName(resolvedType) + "` must have a known size", resolvedType->location);
                                }
                            }
                        }
                    }

                    if (!validBankType) {
                        report->error("invalid bank type `" + getTypeName(resolvedType) + "`", resolvedType->location);
                    }
                }

                exitScope();
            }
        }

        if (!report->validate()) {
            return false;
        }

        definitionsToResolve.clear();
        return true;
    }

    bool Compiler::reserveStorage(const Statement* statement) {
        switch (statement->kind) {
            case StatementKind::Attribution: {
                const auto& attributedStatement = statement->attribution;
                pushAttributeList(statementAttributeLists[statement]);
                if (checkConditionalCompilationAttributes()) {
                    reserveStorage(attributedStatement.body.get());
                }
                popAttributeList();
                break;
            }
            case StatementKind::Bank: break;
            case StatementKind::Block: {
                const auto& blockStatement = statement->block;
                enterScope(getOrCreateStatementScope(StringView(), statement, currentScope));
                for (const auto& item : blockStatement.items) {
                    reserveStorage(item.get());
                }
                exitScope();
                break;
            }
            case StatementKind::Config: break;
            case StatementKind::DoWhile: {
                const auto& doWhileStatement = statement->doWhile;
                reserveStorage(doWhileStatement.body.get());
                break;
            }
            case StatementKind::Enum: break;
            case StatementKind::ExpressionStatement: break;
            case StatementKind::File: {
                const auto& file = statement->file;
                enterScope(findStatementScope(statement));
                for (const auto& item : file.items) {
                    reserveStorage(item.get());
                }
                exitScope();
                break;
            }
            case StatementKind::For: {
                const auto& forStatement = statement->for_;
                reserveStorage(forStatement.body.get());
                break;
            }
            case StatementKind::Func: {
                const auto& funcDeclaration = statement->func;

                auto definition = currentScope->findLocalMemberDefinition(funcDeclaration.name);
                auto& funcDefinition = definition->variant.get<Definition::Func>();    
                
                const auto oldFunction = currentFunction;
                const auto onExit = makeScopeGuard([&]() {
                    currentFunction = oldFunction;
                });

                for (auto& parameter : funcDefinition.parameters) {
                    auto& parameterVarDefinition = parameter->variant.get<Definition::Var>();

                    if (parameterVarDefinition.enclosingFunction != nullptr) {
                        if (parameterVarDefinition.typeExpression->kind != TypeExpressionKind::DesignatedStorage) {
                            report->error("function parameter `" + parameter->name.toString() + "` must have a designated storage type", statement->location);
                            break;
                        }
                    }
                }

                currentFunction = definition;
                reserveStorage(funcDeclaration.body.get());
                break;
            }
            case StatementKind::If: {
                const auto& ifStatement = statement->if_;
                reserveStorage(ifStatement.body.get());
                if (ifStatement.alternative) {
                    reserveStorage(ifStatement.alternative.get());
                }
                break;
            }
            case StatementKind::In: {
                const auto& inStatement = statement->in;
                bankStack.push_back(currentBank);

                const auto result = handleInStatement(inStatement.pieces, inStatement.dest.get(), statement->location);
                if (result.first) {
                    reserveStorage(inStatement.body.get());
                }

                currentBank = bankStack.back();
                bankStack.pop_back();
                break;
            }
            case StatementKind::InlineFor: break;
            case StatementKind::ImportReference: break;
            case StatementKind::InternalDeclaration: break;
            case StatementKind::Branch: break;
            case StatementKind::Label: break;
            case StatementKind::Let: break;
            case StatementKind::Namespace: {
                const auto& namespaceDeclaration = statement->namespace_;
                enterScope(findStatementScope(namespaceDeclaration.body.get()));
                reserveStorage(namespaceDeclaration.body.get());
                exitScope();
                break;
            }
            case StatementKind::Struct: break;
            case StatementKind::TypeAlias: break;
            case StatementKind::Var: {
                const auto& varDeclaration = statement->var;
                const auto& names = varDeclaration.names;
                const auto description = statement->getDescription();
                const auto location = statement->location;

                if (varDeclaration.value != nullptr) {
                    if (names.size() != 1) {
                        report->error(description.toString() + " with initializer must contain exactly one declaration.", location);
                        break;
                    }

                    if (!resolveVariableInitializer(currentScope->findLocalMemberDefinition(varDeclaration.names[0]), varDeclaration.value.get(), description, location)) {
                        break;
                    }
                }

                for (std::size_t i = 0, size = names.size(); i != size; ++i) {
                    if (!reserveVariableStorage(currentScope->findLocalMemberDefinition(names[i]), description, location)) {
                        break;
                    }
                }

                break;
            }
            case StatementKind::While: {
                const auto& whileStatement = statement->while_;
                reserveStorage(whileStatement.body.get());
                break;
            }
            default: std::abort(); return false;
        }

        return statement == program.get() ? report->validate() : report->alive();
    }

    bool Compiler::resolveVariableInitializer(Definition* definition, const Expression* initializer, StringView description, SourceLocation location) {
        auto& varDefinition = definition->variant.get<Definition::Var>();

        if (currentBank == nullptr || !isBankKindStored(currentBank->getKind())) {
            report->error(description.toString() + " with initializer " + (currentBank == nullptr ? "must be inside an `in` statement" : "is not allowed in bank `" + currentBank->getName().toString() + "`"), location);
            return false;
        }

        if (varDefinition.enclosingFunction != nullptr) {
            report->error("local " + description.toString() + " with initializer is not currently supported", location);
            return false;
        }

        allowReservedConstants = true;

        if (auto reducedValue = reduceExpression(initializer)) {
            if (const auto declarationType = varDefinition.reducedTypeExpression.get()) {
                if (declarationType->kind == TypeExpressionKind::DesignatedStorage) {
                    report->error(description.toString() + " cannot have type `" + getTypeName(varDefinition.reducedTypeExpression.get()) + "`", location);
                    return false;
                }

                if (const auto compatibleInitializerType = findCompatibleAssignmentType(reducedValue.get(), declarationType)) {
                    varDefinition.initializerExpression = createConvertedExpression(reducedValue.get(), compatibleInitializerType);
                } else {
                    report->error(description.toString() + " of type `" + getTypeName(varDefinition.reducedTypeExpression.get()) + "` cannot be initialized with `" + getTypeName(reducedValue->info->type.get()) + "` expression", location);
                    return false;
                }
            } else {
                varDefinition.initializerExpression = std::move(reducedValue);
            }

            if (varDefinition.initializerExpression != nullptr) {
                if (const auto arrayType = varDefinition.initializerExpression->info->type->tryGet<TypeExpression::Array>()) {
                    if (arrayType->elementType == nullptr) {
                        report->error("array has unknown element type", location);
                        return false;
                    }
                }
            }

            varDefinition.resolvedType = varDefinition.initializerExpression->info->type.get();
        }

        allowReservedConstants = false;
        varDefinition.nestedConstants.insert(varDefinition.nestedConstants.end(), reservedConstants.begin(), reservedConstants.end());
        reservedConstants.clear();

        return true;
    }

    bool Compiler::reserveVariableStorage(Definition* definition, StringView description, SourceLocation location) {
        auto& varDefinition = definition->variant.get<Definition::Var>();
        const auto name = definition->name;

        if (!varDefinition.resolvedType) {
            report->error("could not resolve declaration type", location);
            return false;
        }

        if (varDefinition.resolvedType->kind == TypeExpressionKind::DesignatedStorage) {
            if ((varDefinition.qualifiers & (Qualifiers::Extern | Qualifiers::Const | Qualifiers::WriteOnly)) != Qualifiers::None) {
                report->error(((varDefinition.qualifiers & Qualifiers::Extern) != Qualifiers::None ? "extern " : "") + description.toString() + " of `" + name.toString() + "` cannot have designated storage type", location);
            }
        } else {
            const auto storageSize = calculateStorageSize(varDefinition.resolvedType, description);
            if (!storageSize.hasValue()) {
                return false;
            }

            varDefinition.storageSize = storageSize;

            if (varDefinition.addressExpression != nullptr) {
                if ((varDefinition.qualifiers & Qualifiers::Extern) != Qualifiers::None || varDefinition.enclosingFunction != nullptr || currentBank == nullptr || !isBankKindStored(currentBank->getKind())) {
                    // Variable definitions with explicit addresses can be placed at any absolute address.
                    // (They don't have relative addresses because of the explcit address can be outside of the current bank.)
                    varDefinition.address = Address(Optional<std::size_t>(), resolveExplicitAddressExpression(varDefinition.addressExpression), nullptr);
                }
            } else {
                if ((varDefinition.qualifiers & Qualifiers::Extern) != Qualifiers::None) {
                    report->error("extern " + description.toString() + " of `" + name.toString() + "` must have an explicit address", location);
                } else if (varDefinition.enclosingFunction == nullptr) {
                    if (currentBank == nullptr) {
                        report->error(description.toString() + " of `" + name.toString() + "` must be inside an `in` statement, have an explicit address `@`, or have a designated storage type", location);
                        return false;
                    }

                    if (!isBankKindStored(currentBank->getKind())) {
                        // FIXME: natural alignment requirements
                        const auto alignment = varDefinition.alignment != 0 ? varDefinition.alignment : 1;
                        if (alignment > 1) {
                            const auto unalignedAddress = currentBank->getAddress().absolutePosition.get();
                            currentBank->absoluteSeek(report, (unalignedAddress + alignment - 1) / alignment * alignment, location);
                        }

                        varDefinition.address = currentBank->getAddress();

                        if (!currentBank->reserveRam(report, description, definition->declaration, location, storageSize.get())) {
                            return false;
                        }
                    }
                } else {
                    report->error("local " + description.toString() + " of `" + name.toString() + "` must have an explicit address, or have a designated storage type", location);
                    return false;
                }
            }
        }

        for (auto& nestedConstant : varDefinition.nestedConstants) {
            reserveVariableStorage(nestedConstant, "nested constant"_sv, nestedConstant->declaration->location);
        }

        return true;
    }

    FwdUniquePtr<InstructionOperand> Compiler::createPlaceholderFromResolvedTypeDefinition(const Definition* resolvedTypeDefinition) const {
        if (const auto builtinIntegerType = resolvedTypeDefinition->variant.tryGet<Definition::BuiltinIntegerType>()) {
            const auto placeholder = platform->getPlaceholderValue();
            const auto mask = Int128((1U << (8U * builtinIntegerType->size)) - 1);
            return makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(placeholder & mask, true));
        } else if (resolvedTypeDefinition->variant.is<Definition::BuiltinBoolType>()) {
            return makeFwdUnique<InstructionOperand>(InstructionOperand::Boolean(false, true));
        } 
        return nullptr;
    }

    FwdUniquePtr<InstructionOperand> Compiler::createPlaceholderFromTypeExpression(const TypeExpression* typeExpression) const {
        if (const auto resolvedTypeDefinition = tryGetResolvedIdentifierTypeDefinition(typeExpression)) {
            return createPlaceholderFromResolvedTypeDefinition(resolvedTypeDefinition);
        } else if (isPointerLikeType(typeExpression)) {
            return createPlaceholderFromResolvedTypeDefinition(isFarType(typeExpression) ? platform->getFarPointerSizedType() : platform->getPointerSizedType());
        }
        return nullptr;
    }

    FwdUniquePtr<InstructionOperand> Compiler::createOperandFromResolvedIdentifier(const Expression* expression, const Definition* definition) const {
        const auto far = (expression->info->qualifiers & Qualifiers::Far) != Qualifiers::None;
        const auto pointerSizedType = far ? platform->getFarPointerSizedType() : platform->getPointerSizedType();
        bool isAddressableOperand = false;
        bool isFunctionLiteral = false;
        Optional<std::size_t> absolutePosition;

        if (const auto varDefinition = definition->variant.tryGet<Definition::Var>()) {
            isAddressableOperand = true;
            if (const auto address = varDefinition->address.tryGet()) {
                absolutePosition = address->absolutePosition;
            }
        } else if (const auto funcDefinition = definition->variant.tryGet<Definition::Func>()) {
            if (funcDefinition->inlined) {
                return nullptr;
            }

            isAddressableOperand = true;
            isFunctionLiteral = true;

            if (const auto address = funcDefinition->address.tryGet()) {
                absolutePosition = address->absolutePosition;
            }
        }

        if (isAddressableOperand) {
            FwdUniquePtr<InstructionOperand> operand;
            if (absolutePosition.hasValue()) {
                const auto mask = Int128((1U << (8U * pointerSizedType->variant.get<Definition::BuiltinIntegerType>().size)) - 1);
                operand = makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(Int128(*absolutePosition) & mask));
            } else {
                operand = createPlaceholderFromResolvedTypeDefinition(pointerSizedType);
            }

            const auto expressionType = expression->info->type.get();

            if (expressionType->kind != TypeExpressionKind::Array
            && (!isFunctionLiteral || expressionType->kind != TypeExpressionKind::Function)) {
                if (const auto indirectionSize = calculateStorageSize(expressionType, "operand"_sv)) {
                    return makeFwdUnique<InstructionOperand>(InstructionOperand::Dereference(far, std::move(operand), *indirectionSize));
                }
                return nullptr;
            } else {
                return operand;
            }
        }

        if (definition->variant.is<Definition::BuiltinRegister>()) {
            return makeFwdUnique<InstructionOperand>(InstructionOperand::Register(definition));
        }

        return nullptr;
    }

    FwdUniquePtr<InstructionOperand> Compiler::createOperandFromLinkTimeExpression(const Expression* expression, bool quiet) const {
        static_cast<void>(quiet);
        if (const auto integerLiteral = expression->tryGet<Expression::IntegerLiteral>()) {
            return makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(integerLiteral->value));
        } else if (const auto booleanLiteral = expression->tryGet<Expression::BooleanLiteral>()) {
            return makeFwdUnique<InstructionOperand>(InstructionOperand::Boolean(booleanLiteral->value));
        } else if (const auto resolvedIdentifier = expression->tryGet<Expression::ResolvedIdentifier>()) {
            return createOperandFromResolvedIdentifier(expression, resolvedIdentifier->definition);
        }

        return createPlaceholderFromTypeExpression(expression->info->type.get());
    }

    FwdUniquePtr<InstructionOperand> Compiler::createOperandFromRunTimeExpression(const Expression* expression, bool quiet) const {
        switch (expression->kind){
            case ExpressionKind::ArrayComprehension: return nullptr;
            case ExpressionKind::ArrayPadLiteral: return nullptr;
            case ExpressionKind::BinaryOperator: {
                const auto& binaryOperator = expression->binaryOperator;
                if (binaryOperator.op == BinaryOperatorKind::Indexing) {
                    const auto indexLiteral = binaryOperator.right->tryGet<Expression::IntegerLiteral>();

                    if (binaryOperator.left->info->context == EvaluationContext::LinkTime && indexLiteral != nullptr) {
                        if (const auto indirectionSize = calculateStorageSize(expression->info->type.get(), "operand"_sv)) {
                            const auto far = (binaryOperator.left->info->qualifiers & Qualifiers::Far) != Qualifiers::None;

                            return makeFwdUnique<InstructionOperand>(InstructionOperand::Dereference(
                                far,
                                createPlaceholderFromResolvedTypeDefinition(far ? platform->getFarPointerSizedType() : platform->getPointerSizedType()),
                                *indirectionSize));
                        }
                        return nullptr;
                    }
                    if (const auto indirectionSize = calculateStorageSize(expression->info->type.get(), "operand"_sv)) {
                        if (auto operand = createOperandFromExpression(binaryOperator.left.get(), quiet)) {
                            if (auto subscript = createOperandFromExpression(binaryOperator.right.get(), quiet)) {
                                const auto far = (binaryOperator.left->info->qualifiers & Qualifiers::Far) != Qualifiers::None;

                                return makeFwdUnique<InstructionOperand>(InstructionOperand::Index(
                                    far,
                                    std::move(operand),
                                    std::move(subscript),
                                    *indirectionSize,
                                    *indirectionSize));
                            }
                        }
                    }
                    return nullptr;
                } else if (binaryOperator.op == BinaryOperatorKind::BitIndexing) {
                    auto operand = createOperandFromExpression(binaryOperator.left.get(), quiet);
                    auto subscript = createOperandFromExpression(binaryOperator.right.get(), quiet);
                    if (operand && subscript) {
                        return makeFwdUnique<InstructionOperand>(InstructionOperand::BitIndex(std::move(operand), std::move(subscript)));
                    }
                } else if (binaryOperator.op != BinaryOperatorKind::Assignment) {
                    auto left = createOperandFromExpression(binaryOperator.left.get(), quiet);
                    auto right = createOperandFromExpression(binaryOperator.right.get(), quiet);
                    if (left && right) {
                        const auto leftIntegerOperand = left->tryGet<InstructionOperand::Integer>();
                        const auto rightIntegerOperand = right->tryGet<InstructionOperand::Integer>();

                        if (leftIntegerOperand != nullptr && rightIntegerOperand != nullptr) {
                            if (leftIntegerOperand->placeholder) {
                                return left->clone();
                            } else if (rightIntegerOperand->placeholder) {
                                return right->clone();
                            }
                        }

                        return makeFwdUnique<InstructionOperand>(InstructionOperand::Binary(binaryOperator.op, std::move(left), std::move(right)));
                    }
                }

                return nullptr;
            }
            case ExpressionKind::BooleanLiteral: {
                const auto& booleanLiteral = expression->booleanLiteral;
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Boolean(booleanLiteral.value));
            }
            case ExpressionKind::Call: return nullptr;
            case ExpressionKind::Cast: {
                const auto& cast = expression->cast;
                const auto sourceType = cast.operand->info->type.get();
                const auto destType = expression->info->type.get();
                if (const auto sourceExpressionSize = calculateStorageSize(sourceType, "left-hand side of cast expression"_sv)) {
                    if (const auto destTypeSize = calculateStorageSize(destType, "right-hand side of cast expression"_sv)) {
                        bool validRuntimeCast = false;

                        if (sourceExpressionSize.get() == destTypeSize.get()) {
                            validRuntimeCast = true;
                        } else {
                            if (const auto resolvedIdentifier = cast.operand->tryGet<Expression::ResolvedIdentifier>()) {
                                if (resolvedIdentifier->definition->variant.is<Definition::BuiltinRegister>()) {
                                    validRuntimeCast = true;
                                }
                            }
                        }

                        if (validRuntimeCast) {
                            return createOperandFromExpression(cast.operand.get(), quiet);
                        } else {
                            if (!quiet) {
                                report->error("run-time cast from `" + getTypeName(sourceType) + "` to `" + getTypeName(destType) + "` is not possible because it would require a temporary", expression->location, ReportErrorFlags::Continued);
                            }
                        }
                    }
                }
                return nullptr;
            }
            case ExpressionKind::Embed: return nullptr;
            case ExpressionKind::FieldAccess: return nullptr;
            case ExpressionKind::Identifier: return nullptr;
            case ExpressionKind::IntegerLiteral: {
                const auto& integerLiteral = expression->integerLiteral;
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(integerLiteral.value));
            }
            case ExpressionKind::OffsetOf: return nullptr;
            case ExpressionKind::RangeLiteral: return nullptr;
            case ExpressionKind::ResolvedIdentifier: {
                const auto& resolvedIdentifier = expression->resolvedIdentifier;
                return createOperandFromResolvedIdentifier(expression, resolvedIdentifier.definition);
            }
            case ExpressionKind::SideEffect: return nullptr;
            case ExpressionKind::StringLiteral: return nullptr;
            case ExpressionKind::StructLiteral: return nullptr;
            case ExpressionKind::TupleLiteral: return nullptr;
            case ExpressionKind::TypeOf: return nullptr;
            case ExpressionKind::TypeQuery: return nullptr;
            case ExpressionKind::UnaryOperator: {
                const auto& unaryOperator = expression->unaryOperator;
                if (auto operand = createOperandFromExpression(unaryOperator.operand.get(), quiet)) {
                    switch (unaryOperator.op) {
                        case UnaryOperatorKind::Indirection: {
                            const auto far = (unaryOperator.operand->info->qualifiers & Qualifiers::Far) != Qualifiers::None;

                            if (const auto indirectionSize = calculateStorageSize(expression->info->type.get(), "operand"_sv)) {
                                if (const auto bin = operand->tryGet<InstructionOperand::Binary>()) {
                                    if (bin->kind == BinaryOperatorKind::Addition) {
                                        return makeFwdUnique<InstructionOperand>(InstructionOperand::Index(
                                            far,
                                            bin->left->clone(),
                                            bin->right->clone(),
                                            1,
                                            *indirectionSize));
                                    } else if (bin->kind == BinaryOperatorKind::Subtraction) {
                                        if (const auto rightIntegerOperand = bin->right->tryGet<InstructionOperand::Integer>()) {
                                            if (rightIntegerOperand->placeholder) {
                                                return makeFwdUnique<InstructionOperand>(InstructionOperand::Index(
                                                    far,
                                                    bin->left->clone(),
                                                    bin->right->clone(),
                                                    1,
                                                    *indirectionSize));
                                            } else {
                                                return makeFwdUnique<InstructionOperand>(InstructionOperand::Index(
                                                    far,
                                                    bin->left->clone(),
                                                    makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(-rightIntegerOperand->value)),
                                                    1,
                                                    *indirectionSize));
                                            }
                                        }
                                    }
                                }
                                return makeFwdUnique<InstructionOperand>(InstructionOperand::Dereference(
                                    far,
                                    std::move(operand),
                                    *indirectionSize));
                            }
                            return nullptr;
                        }
                        default: {
                            return makeFwdUnique<InstructionOperand>(InstructionOperand::Unary(unaryOperator.op, std::move(operand)));
                        }
                    }
                }
                return nullptr;
            }
            default: std::abort(); return nullptr;
        }
    }

    FwdUniquePtr<InstructionOperand> Compiler::createOperandFromExpression(const Expression* expression, bool quiet) const {
        if (expression->info->context == EvaluationContext::RunTime) {
            return createOperandFromRunTimeExpression(expression, quiet);
        } else {
            return createOperandFromLinkTimeExpression(expression, quiet);
        }
    }

    bool Compiler::isLeafExpression(const Expression* expression) const {
        if (expression->info->context == EvaluationContext::RunTime) {
            switch (expression->kind){
                case ExpressionKind::ArrayComprehension: return true;
                case ExpressionKind::ArrayPadLiteral: return true;
                case ExpressionKind::ArrayLiteral: return true;
                case ExpressionKind::BinaryOperator: {
                    const auto& binaryOperator = expression->binaryOperator;
                    const auto op = binaryOperator.op;
                    if (op == BinaryOperatorKind::BitIndexing
                    || op == BinaryOperatorKind::Indexing) {
                        return true;
                    }
                    return false;
                }
                case ExpressionKind::BooleanLiteral: return true;
                case ExpressionKind::Call: return false;
                case ExpressionKind::Cast: return true;
                case ExpressionKind::Embed: return true;
                case ExpressionKind::FieldAccess: return true;
                case ExpressionKind::Identifier: return true;
                case ExpressionKind::IntegerLiteral: return true;
                case ExpressionKind::OffsetOf: return true;
                case ExpressionKind::RangeLiteral: return true;
                case ExpressionKind::ResolvedIdentifier: return true;
                case ExpressionKind::SideEffect: return false;
                case ExpressionKind::StringLiteral: return true;
                case ExpressionKind::StructLiteral: return true;
                case ExpressionKind::TupleLiteral: return true;
                case ExpressionKind::TypeOf: return true;
                case ExpressionKind::TypeQuery: return true;
                case ExpressionKind::UnaryOperator: {
                    const auto& unaryOperator = expression->unaryOperator;
                    if (unaryOperator.op == UnaryOperatorKind::Indirection) {
                        return true;
                    }
                    return false;
                }
                default: std::abort(); return false;
            }
        } else {
            return true;
        }
    }

    bool Compiler::hasNestedAssignment(const Expression* expression) const {
        if (expression->info->context == EvaluationContext::RunTime) {
            switch (expression->kind){
                case ExpressionKind::ArrayComprehension: return false;
                case ExpressionKind::ArrayPadLiteral: return false;
                case ExpressionKind::ArrayLiteral: return false;
                case ExpressionKind::BinaryOperator: {
                    const auto& binaryOperator = expression->binaryOperator;
                    return binaryOperator.op == BinaryOperatorKind::Assignment
                        || hasNestedAssignment(binaryOperator.left.get())
                        || hasNestedAssignment(binaryOperator.right.get());
                }
                case ExpressionKind::BooleanLiteral: return false;
                case ExpressionKind::Call: {
                    const auto& call = expression->call;
                    return !call.inlined && hasNestedAssignment(call.function.get());
                }
                case ExpressionKind::Cast: {
                    const auto& cast = expression->cast;
                    return hasNestedAssignment(cast.operand.get());
                }
                case ExpressionKind::Embed: return false;
                case ExpressionKind::FieldAccess: return false;
                case ExpressionKind::Identifier: return false;
                case ExpressionKind::IntegerLiteral: return false;
                case ExpressionKind::OffsetOf: return false;
                case ExpressionKind::RangeLiteral: return false;
                case ExpressionKind::ResolvedIdentifier: return false;
                case ExpressionKind::SideEffect: return false;
                case ExpressionKind::StringLiteral: return false;
                case ExpressionKind::StructLiteral: return false;
                case ExpressionKind::TupleLiteral: return false;
                case ExpressionKind::TypeOf: return false;
                case ExpressionKind::TypeQuery: return false;
                case ExpressionKind::UnaryOperator: {
                    const auto& unaryOperator = expression->unaryOperator;
                    return isUnaryIncrementOperator(unaryOperator.op)
                        || hasNestedAssignment(unaryOperator.operand.get());
                }
                default: std::abort(); return false;
            }
        } else {
            return false;
        }
    }

    bool Compiler::emitNestedAssignmentIr(const Expression* expression, bool pre, bool post) {
        if (expression->info->context == EvaluationContext::RunTime) {
            switch (expression->kind){
                case ExpressionKind::ArrayComprehension: return true;
                case ExpressionKind::ArrayPadLiteral: return true;
                case ExpressionKind::ArrayLiteral: return true;
                case ExpressionKind::BinaryOperator: {
                    const auto& binaryOperator = expression->binaryOperator;
                    const auto left = binaryOperator.left.get();
                    const auto right = binaryOperator.right.get();

                    if (binaryOperator.op == BinaryOperatorKind::Assignment) {
                        return pre
                            ? emitAssignmentExpressionIr(left, right, left->location)
                            : true;
                    } else {
                        return emitNestedAssignmentIr(left, pre, post)
                            && emitNestedAssignmentIr(right, pre, post);
                    }
                }
                case ExpressionKind::BooleanLiteral: return true;
                case ExpressionKind::Call: {
                    const auto& call = expression->call;
                    if (!call.inlined) {
                        return emitNestedAssignmentIr(call.function.get(), pre, post);
                    } else {
                        return true;
                    }
                }
                case ExpressionKind::Cast: {
                    const auto& cast = expression->cast;
                    return emitNestedAssignmentIr(cast.operand.get(), pre, post);
                }
                case ExpressionKind::Embed: return true;
                case ExpressionKind::FieldAccess: return true;
                case ExpressionKind::Identifier: return true;
                case ExpressionKind::IntegerLiteral: return true;
                case ExpressionKind::OffsetOf: return true;
                case ExpressionKind::RangeLiteral: return true;
                case ExpressionKind::ResolvedIdentifier: return true;
                case ExpressionKind::SideEffect: return true;
                case ExpressionKind::StringLiteral: return true;
                case ExpressionKind::StructLiteral: return true;
                case ExpressionKind::TupleLiteral: return true;
                case ExpressionKind::TypeOf: return true;
                case ExpressionKind::TypeQuery: return true;
                case ExpressionKind::UnaryOperator: {
                    const auto& unaryOperator = expression->unaryOperator;
                    const auto& op = unaryOperator.op;
                    const auto operand = unaryOperator.operand.get();

                    if (isUnaryPreIncrementOperator(op)) {
                        if (pre && !emitUnaryExpressionIr(operand, op, operand, operand->location)) {
                            raiseEmitUnaryExpressionError(operand, UnaryOperatorKind::PreIncrement, operand, operand->location);
                            return false;
                        }
                        return true;
                    } else if (isUnaryPostIncrementOperator(op)) {
                        if (post && !emitUnaryExpressionIr(operand, getUnaryPreIncrementEquivalent(op), operand, operand->location)) {
                            raiseEmitUnaryExpressionError(operand, op, operand, operand->location);
                            return false;
                        }
                        return true;
                    } else {
                        return emitNestedAssignmentIr(operand, pre, post);
                    }
                }
                default: std::abort(); return false;
            }
        } else {
            return true;
        }
    }

    FwdUniquePtr<const Expression> Compiler::stripNestedAssignment(const Expression* expression) const {
        if (expression->info->context == EvaluationContext::RunTime) {
            switch (expression->kind){
                case ExpressionKind::ArrayComprehension: return expression->clone();
                case ExpressionKind::ArrayPadLiteral: return expression->clone();
                case ExpressionKind::ArrayLiteral: return expression->clone();
                case ExpressionKind::BinaryOperator: {
                    const auto& binaryOperator = expression->binaryOperator;
                    const auto op = binaryOperator.op;
                    const auto left = binaryOperator.left.get();
                    const auto right = binaryOperator.right.get();

                    auto strippedLeft = stripNestedAssignment(left);

                    if (op == BinaryOperatorKind::Assignment) {
                        return strippedLeft;
                    } else {
                        auto strippedRight = stripNestedAssignment(right);

                        return makeFwdUnique<const Expression>(
                            Expression::BinaryOperator(op, std::move(strippedLeft), std::move(strippedRight)),
                            expression->location,
                            expression->info->clone());
                    }
                }
                case ExpressionKind::BooleanLiteral: return expression->clone();
                case ExpressionKind::Call: {
                    const auto& call = expression->call;

                    if (!call.inlined) {
                        const auto function = call.function.get();
                        const auto& arguments = call.arguments;

                        auto strippedFunction = stripNestedAssignment(function);
                        std::vector<FwdUniquePtr<const Expression>> clonedArguments;
                        for (auto& argument : arguments) {
                            clonedArguments.push_back(argument->clone());
                        }

                        return makeFwdUnique<const Expression>(
                            Expression::Call(false, std::move(strippedFunction), std::move(clonedArguments)),
                            expression->location,
                            expression->info->clone());
                    } else {
                        return expression->clone();
                    }
                }
                case ExpressionKind::Cast: {
                    const auto& cast = expression->cast;
                    const auto& operand = cast.operand.get();
                    auto strippedOperand = stripNestedAssignment(operand);

                    return makeFwdUnique<const Expression>(
                        Expression::Cast(std::move(strippedOperand), cast.type->clone()),
                        expression->location,
                        expression->info->clone());
                }
                case ExpressionKind::Embed: return expression->clone();
                case ExpressionKind::FieldAccess: return expression->clone();
                case ExpressionKind::Identifier: return expression->clone();
                case ExpressionKind::IntegerLiteral: return expression->clone();
                case ExpressionKind::OffsetOf: return expression->clone();
                case ExpressionKind::RangeLiteral: return expression->clone();
                case ExpressionKind::ResolvedIdentifier: return expression->clone();
                case ExpressionKind::SideEffect: return expression->clone();
                case ExpressionKind::StringLiteral: return expression->clone();
                case ExpressionKind::StructLiteral: return expression->clone();
                case ExpressionKind::TupleLiteral: return expression->clone();
                case ExpressionKind::TypeOf: return expression->clone();
                case ExpressionKind::TypeQuery: return expression->clone();
                case ExpressionKind::UnaryOperator: {
                    const auto& unaryOperator = expression->unaryOperator;
                    const auto op = unaryOperator.op;
                    const auto operand = unaryOperator.operand.get();

                    auto strippedOperand = stripNestedAssignment(operand);

                    if (isUnaryIncrementOperator(op)) {
                        return strippedOperand;
                    } else {
                        return makeFwdUnique<const Expression>(
                            Expression::UnaryOperator(op, std::move(strippedOperand)),
                            expression->location,
                            expression->info->clone());
                    }
                }
                default: std::abort(); return nullptr;
            }
        } else {
            return expression->clone();
        }
    }

    bool Compiler::emitLoadExpressionIr(const Expression* dest, const Expression* source, SourceLocation location) {
        auto destOperand = createOperandFromExpression(dest, true);
        auto sourceOperand = createOperandFromExpression(source, true);

        if (!destOperand || !sourceOperand) {
            return false;
        }

        if (*destOperand == *sourceOperand) {
            return true;
        }

        std::vector<InstructionOperandRoot> operandRoots;
        operandRoots.reserve(2);
        operandRoots.push_back(InstructionOperandRoot(dest, std::move(destOperand)));
        operandRoots.push_back(InstructionOperandRoot(source, std::move(sourceOperand)));

        if (const auto instruction = builtins.selectInstruction(InstructionType(BinaryOperatorKind::Assignment), modeFlags, operandRoots)) {
            irNodes.addNew(IrNode::Code(instruction, std::move(operandRoots)), location);
            return true;
        } else {
            return false;
        }
    }

    bool Compiler::emitUnaryExpressionIr(const Expression* dest, UnaryOperatorKind op, const Expression* source, SourceLocation location) {
        auto destOperand = createOperandFromExpression(dest, true);
        auto sourceOperand = createOperandFromExpression(source, true);

        if (!destOperand || !sourceOperand) {
            return false;
        }

        std::vector<InstructionOperandRoot> operandRoots;

        if (*destOperand == *sourceOperand) {
            // dest = -dest; or ++dest; instruction
            operandRoots.reserve(1);
            operandRoots.push_back(InstructionOperandRoot(dest, std::move(destOperand)));
        } else {
            // dest = -source; instruction
            operandRoots.reserve(2);
            operandRoots.push_back(InstructionOperandRoot(dest, std::move(destOperand)));
            operandRoots.push_back(InstructionOperandRoot(source, std::move(sourceOperand)));
        }            

        if (const auto instruction = builtins.selectInstruction(InstructionType(op), modeFlags, operandRoots)) {
            irNodes.addNew(IrNode::Code(instruction, std::move(operandRoots)), location);
            return true;
        } else {
            return false;
        }
    }

    bool Compiler::emitBinaryExpressionIr(const Expression* dest, BinaryOperatorKind op, const Expression* left, const Expression* right, SourceLocation location) {
        auto destOperand = createOperandFromExpression(dest, true);
        auto leftOperand = createOperandFromExpression(left, true);
        auto rightOperand = createOperandFromExpression(right, true);

        if (!destOperand || !leftOperand || !rightOperand) {
            return false;
        }

        std::vector<InstructionOperandRoot> operandRoots;

        if (*destOperand == *leftOperand) {
            // dest += right; instruction
            operandRoots.reserve(2);
            operandRoots.push_back(InstructionOperandRoot(dest, std::move(destOperand)));
            operandRoots.push_back(InstructionOperandRoot(right, std::move(rightOperand)));
        } else {
            // dest = left + right; instruction
            operandRoots.reserve(3);
            operandRoots.push_back(InstructionOperandRoot(dest, std::move(destOperand)));
            operandRoots.push_back(InstructionOperandRoot(left, std::move(leftOperand)));
            operandRoots.push_back(InstructionOperandRoot(right, std::move(rightOperand)));
        }

        if (const auto instruction = builtins.selectInstruction(InstructionType(op), modeFlags, operandRoots)) {
            irNodes.addNew(
                IrNode::Code(instruction, std::move(operandRoots)),
                location);
            return true;
        } else {
            return false;
        }
    }

    bool Compiler::emitArgumentPassIr(const TypeExpression* functionTypeExpression, const std::vector<Definition*>& parameters, const std::vector<FwdUniquePtr<const Expression>>& arguments, SourceLocation location) {
        static_cast<void>(location);

        auto& functionType = functionTypeExpression->function;

        if (functionType.parameterTypes.size() != arguments.size()) {
            return false;
        }

        for (std::size_t i = 0; i != arguments.size(); ++i) {
            const auto& parameterType = functionType.parameterTypes[i];
            const auto& argument = arguments[i];
            if (auto designatedStorageType = parameterType->tryGet<TypeExpression::DesignatedStorage>()) {
                emitAssignmentExpressionIr(designatedStorageType->holder.get(), argument.get(), argument->location);

            } else {
                report->error(
                    "could not generate initializer for argument `"
                        + (parameters.size() != 0 ? "argument `" + parameters[i]->name.toString() + "`" : "argument #" + std::to_string(i))
                        + "` of type `" + getTypeName(parameterType.get()) + "`",
                    argument->location);
                return false;
            }
        }

        return true;
    }

    bool Compiler::emitCallExpressionIr(std::size_t distanceHint, bool inlined, bool tailCall, const Expression* resultDestination, const Expression* function, const std::vector<FwdUniquePtr<const Expression>>& arguments, SourceLocation location) {
        if (const auto resolvedIdentifier = function->tryGet<Expression::ResolvedIdentifier>()) {
            const auto definition = resolvedIdentifier->definition;

            if (const auto funcDefinition = definition->variant.tryGet<Definition::Func>()) {
                auto& functionType = funcDefinition->resolvedSignatureType->function;

                if (!emitArgumentPassIr(funcDefinition->resolvedSignatureType.get(), funcDefinition->parameters, arguments, location)) {
                    return false;
                }

                if (inlined || funcDefinition->inlined) {
                    tailCall = false;

                    auto oldReturnKind = funcDefinition->returnKind;
                    auto oldInlined = funcDefinition->inlined;
                    funcDefinition->returnKind = BranchKind::None;
                    funcDefinition->inlined = true;

                    const auto oldContinueLabel = continueLabel;
                    const auto oldBreakLabel = breakLabel;
                    const auto onExit = makeScopeGuard([&]() {
                        continueLabel = oldContinueLabel;
                        breakLabel = oldBreakLabel;
                    });

                    continueLabel = nullptr;
                    breakLabel = nullptr;

                    enterInlineSite(registeredInlineSites.addNew());

                    const auto funcDeclaration = definition->declaration;
                    enterScope(getOrCreateStatementScope(StringView(), funcDeclaration, funcDefinition->enclosingScope));

                    bool valid = reserveDefinitions(funcDeclaration) && resolveDefinitionTypes() && reserveStorage(funcDeclaration);

                    if (valid) {
                        // TODO: templating on type or expression???
                        valid = emitFunctionIr(definition, function->location);
                    }

                    exitScope();
                    exitInlineSite();
                    funcDefinition->returnKind = oldReturnKind;
                    funcDefinition->inlined = oldInlined;

                } else {
                    auto destOperand = createOperandFromExpression(function, true);

                    if (!destOperand) {
                        return false;
                    }

                    std::vector<InstructionOperandRoot> operandRoots;
                    operandRoots.reserve(2);
                    operandRoots.push_back(InstructionOperandRoot(nullptr, makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(Int128(distanceHint)))));
                    operandRoots.push_back(InstructionOperandRoot(function, std::move(destOperand)));

                    bool far = functionType.far;
                    BranchKind kind = BranchKind::None;

                    if (tailCall) {
                        kind = far ? BranchKind::FarGoto : BranchKind::Goto;
                    } else {
                        kind = far ? BranchKind::FarCall : BranchKind::Call;
                    }

                    if (const auto instruction = builtins.selectInstruction(InstructionType(kind), modeFlags, operandRoots)) {
                        irNodes.addNew(IrNode::Code(instruction, std::move(operandRoots)), function->location);
                    } else {
                        return false;
                    }
                }

                if (resultDestination != nullptr) {
                    const auto returnType = functionType.returnType.get();

					if (const auto designatedStorageType = returnType->tryGet<TypeExpression::DesignatedStorage>()) {
						auto destOperand = createOperandFromExpression(resultDestination, true);
						auto sourceOperand = createOperandFromExpression(designatedStorageType->holder.get(), true);

						if (destOperand == nullptr || sourceOperand == nullptr || *destOperand != *sourceOperand) {
							report->error("could not generate assignment for `func " + definition->name.toString() + "` result of type `" + getTypeName(returnType) + "`", location);
							return false;
						}
					}
				}

                return true;
            } else if (definition->variant.is<Definition::BuiltinVoidIntrinsic>()) {
                if (resultDestination != nullptr) {
                    report->error("void intrinsic `" + definition->name.toString() + "` has no return value so its result cannot be stored.", location);
                    return false;
                }

                std::vector<InstructionOperandRoot> operandRoots;
                operandRoots.reserve(arguments.size());

                for (const auto& argument : arguments) {
                    const Expression* expression = nullptr;
                    FwdUniquePtr<InstructionOperand> operand;

                    expression = argument.get();
                    operand = createOperandFromExpression(argument.get(), true);
                    if (!operand) {
                        if (const auto binaryOperator = argument->tryGet<Expression::BinaryOperator>()) {
                            if (binaryOperator->op == BinaryOperatorKind::Assignment) {
                                if (!emitAssignmentExpressionIr(binaryOperator->left.get(), binaryOperator->right.get(), binaryOperator->left->location)) {
                                    return false;
                                }
                                expression = binaryOperator->left.get();
                                operand = createOperandFromExpression(expression, true);
                            }
                        } else if (const auto unaryOperator = argument->tryGet<Expression::UnaryOperator>()) {
                            const auto term = unaryOperator->operand.get();
                            const auto op = unaryOperator->op;
                            if (op == UnaryOperatorKind::PreIncrement || op == UnaryOperatorKind::PreDecrement) {
                                if (!emitUnaryExpressionIr(term, op, term, term->location)) {
                                    raiseEmitUnaryExpressionError(term, op, term, term->location);
                                    return false;
                                }
                                expression = term;
                                operand = createOperandFromExpression(term, true);
                            } else {
                                expression = nullptr;
                                operand = nullptr;
                            }
                        }
                    }
                        
                    operandRoots.push_back(InstructionOperandRoot(expression, std::move(operand)));
                }

                if (const auto instruction = builtins.selectInstruction(
                    InstructionType(InstructionType::VoidIntrinsic(definition)),
                    modeFlags,
                    operandRoots)
                ) {
                    irNodes.addNew(IrNode::Code(instruction, std::move(operandRoots)), function->location);
                    return true;
                } else {
                    raiseEmitIntrinsicError(InstructionType(InstructionType::VoidIntrinsic(definition)), operandRoots, location);
                    return false;
                }
            } else if (definition->variant.is<Definition::BuiltinLoadIntrinsic>()) {
                if (resultDestination == nullptr) {
                    report->error("load intrinsic `" + definition->name.toString() + "` must have its result stored somewhere.", location);
                    return false;
                }

                std::vector<InstructionOperandRoot> operandRoots;
                operandRoots.reserve(arguments.size() + 1);

                operandRoots.push_back(InstructionOperandRoot(resultDestination, createOperandFromExpression(resultDestination, true)));

                for (const auto& argument : arguments) {
                    const Expression* expression = nullptr;
                    FwdUniquePtr<InstructionOperand> operand;

                    expression = argument.get();
                    operand = createOperandFromExpression(argument.get(), true);

                    if (!operand) {
                        if (const auto binaryOperator = argument->tryGet<Expression::BinaryOperator>()) {
                            if (binaryOperator->op == BinaryOperatorKind::Assignment) {
                                if (!emitAssignmentExpressionIr(binaryOperator->left.get(), binaryOperator->right.get(), binaryOperator->left->location)) {
                                    return false;
                                }
                                expression = binaryOperator->left.get();
                                operand = createOperandFromExpression(expression, true);
                            }
                        } else if (const auto unaryOperator = argument->tryGet<Expression::UnaryOperator>()) {
                            const auto term = unaryOperator->operand.get();
                            const auto op = unaryOperator->op;
                            if (op == UnaryOperatorKind::PreIncrement || op == UnaryOperatorKind::PreDecrement) {
                                if (!emitUnaryExpressionIr(term, op, term, term->location)) {
                                    raiseEmitUnaryExpressionError(term, op, term, term->location);
                                    return false;
                                }
                                expression = term;
                                operand = createOperandFromExpression(term, true);
                            } else {
                                expression = nullptr;
                                operand = nullptr;
                            }
                        }
                    }
                    
                    operandRoots.push_back(InstructionOperandRoot(expression, std::move(operand)));
                }
                
                if (const auto instruction = builtins.selectInstruction(
                    InstructionType(InstructionType::LoadIntrinsic(definition)),
                    modeFlags,
                    operandRoots)
                ) {
                    irNodes.addNew(IrNode::Code(instruction, std::move(operandRoots)), location);
                    return true;
                } else {
                    raiseEmitIntrinsicError(InstructionType(InstructionType::LoadIntrinsic(definition)), operandRoots, location);
                    return false;
                }
            }
        } else if (const auto functionType = function->info->type->tryGet<TypeExpression::Function>()) {
            auto destOperand = createOperandFromExpression(function, true);

            if (!destOperand) {
                return false;
            }

            std::vector<InstructionOperandRoot> operandRoots;
            operandRoots.reserve(2);
            operandRoots.push_back(InstructionOperandRoot(nullptr, makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(Int128(0)))));
            operandRoots.push_back(InstructionOperandRoot(function, std::move(destOperand)));

            if (!emitArgumentPassIr(function->info->type.get(), {}, arguments, location)) {
                return false;
            }

            bool far = functionType->far;
            BranchKind kind = BranchKind::None;

            if (tailCall) {
                kind = far ? BranchKind::FarGoto : BranchKind::Goto;
            } else {
                kind = far ? BranchKind::FarCall : BranchKind::Call;
            }

            if (const auto instruction = builtins.selectInstruction(InstructionType(kind), modeFlags, operandRoots)) {
                irNodes.addNew(IrNode::Code(instruction, std::move(operandRoots)), function->location);

                if (resultDestination != nullptr) {
                    const auto returnType = functionType->returnType.get();

                    if (auto designatedStorageType = returnType->tryGet<TypeExpression::DesignatedStorage>()) {
                        emitAssignmentExpressionIr(resultDestination, designatedStorageType->holder.get(), location);
                    } else {
                        report->error("could not generate assignment for `func` result of type `" + getTypeName(returnType) + "`", location);
                    }
                }

                return true;
            } else {
                return false;
            }
        }

        report->error("unhandled call expression", location, ReportErrorFlags::InternalError);
        return false;
    }

    std::string Compiler::getModeFlagString(std::uint32_t modeFlags) {
        std::string result = "";
        for (std::size_t modeIndex = 0, modeCount = builtins.getModeAttributeCount(); modeIndex != modeCount; ++modeIndex) {
            if ((modeFlags & (1U << modeIndex)) != 0) {
                const auto modeAttribute = builtins.getModeAttribute(modeIndex);
                result += (result.size() > 0 ? ", " : "") + modeAttribute->name.toString();
            }
        }
        return result;
    }

    void Compiler::raiseEmitLoadError(const Expression* dest, const Expression* source, SourceLocation location) {
        const auto candidates = builtins.findAllInstructionsByType(InstructionType(BinaryOperatorKind::Assignment));
        report->error("could not generate code for " + getBinaryOperatorName(BinaryOperatorKind::Assignment).toString(), source->location, ReportErrorFlags::Continued);

        auto destOperand = createOperandFromExpression(dest, false);
        auto sourceOperand = createOperandFromExpression(source, false);

        if (destOperand == nullptr) {
            report->error("could not create an instruction operand for destination of " + getBinaryOperatorName(BinaryOperatorKind::Assignment).toString(), location);
            return;
        }
        if (sourceOperand == nullptr) {
            report->error("could not create an instruction operand for source of " + getBinaryOperatorName(BinaryOperatorKind::Assignment).toString(), location);
            return;
        }

        {
            auto message = "got: `"
                + destOperand->toString()
                + " = " + sourceOperand->toString()
                + "`";
            if (modeFlags != 0) {
                message += " (" + getModeFlagString(modeFlags) + ")";
            }

            report->error(message, source->location, ReportErrorFlags::Continued);
        }

        if (candidates.size() > 0) {
            std::size_t optionCount = 0;
            for (const auto candidate : candidates) {
                switch (candidate->signature.operandPatterns.size()) {
                    case 2: {
                        if (candidate->signature.operandPatterns[0]->matches(*destOperand)) {
                            if (optionCount == 0) {
                                report->error("possible options:", source->location, ReportErrorFlags::Continued);
                            }
                            ++optionCount;

                            auto message = "  `"
                                + candidate->signature.operandPatterns[0]->toString()
                                + " = " + candidate->signature.operandPatterns[1]->toString()
                                + "`";
                            if (candidate->signature.requiredModeFlags != 0) {
                                message += " (" + getModeFlagString(candidate->signature.requiredModeFlags) + ")";
                            }

                            report->log(message);
                        }
                        break;
                    }
                    default: {
                        std::abort();
                        break;
                    }
                }
            }
        }

        report->error("assignment must be rewritten some other way\n", source->location);
    }

    void Compiler::raiseEmitUnaryExpressionError(const Expression* dest, UnaryOperatorKind op, const Expression* source, SourceLocation location) {
        const auto candidates = builtins.findAllInstructionsByType(InstructionType(op));
        report->error("could not generate code for " + getUnaryOperatorName(op).toString(), source->location, ReportErrorFlags::Continued);

        auto destOperand = createOperandFromExpression(dest, false);
        auto sourceOperand = createOperandFromExpression(source, false);

        if (destOperand == nullptr) {
            report->error("could not create an instruction operand for destination of " + getUnaryOperatorName(op).toString(), location);
            return;
        }
        if (sourceOperand == nullptr) {
            report->error("could not create an instruction operand for source of " + getUnaryOperatorName(op).toString(), location);
            return;
        }

        bool isIncrement = isUnaryIncrementOperator(op);
        op = getUnaryPreIncrementEquivalent(op);

        if (isIncrement && *destOperand == *sourceOperand) {
            auto message = "got: `"
                + getUnaryOperatorSymbol(op).toString()
                + destOperand->toString()
                + "`";
            if (modeFlags != 0) {
                message += " (" + getModeFlagString(modeFlags) + ")";
            }

            report->error(message, source->location, ReportErrorFlags::Continued);
        } else {
            auto message = "got: `"
                + destOperand->toString()
                + " = "
                + getUnaryOperatorSymbol(op).toString()
                + sourceOperand->toString()
                + "`";
            if (modeFlags != 0) {
                message += " (" + getModeFlagString(modeFlags) + ")";
            }

            report->error(message, source->location, ReportErrorFlags::Continued);
        }

        if (candidates.size() > 0) {
            report->error("possible options:", source->location, ReportErrorFlags::Continued);
            for (const auto candidate : candidates) {
                switch (candidate->signature.operandPatterns.size()) {
                    case 1: {
                        std::string message;
                        if (isIncrement) {
                            message += "  `"
                                + getUnaryOperatorSymbol(op).toString()
                                + candidate->signature.operandPatterns[0]->toString()
                                + "`";
                        } else {
                            message += "  `"
                                + candidate->signature.operandPatterns[0]->toString()
                                + " = "
                                + getUnaryOperatorSymbol(op).toString()
                                + candidate->signature.operandPatterns[0]->toString()
                                + "`";
                        }
                        if (candidate->signature.requiredModeFlags != 0) {
                            message += " (" + getModeFlagString(candidate->signature.requiredModeFlags) + ")";
                        }

                        report->log(message);

                        break;
                    }
                    case 2: {
                        auto message = "  `"
                            + candidate->signature.operandPatterns[0]->toString()
                            + " = "
                            + getUnaryOperatorSymbol(op).toString()
                            + candidate->signature.operandPatterns[1]->toString()
                            + "`";
                        if (candidate->signature.requiredModeFlags != 0) {
                            message += " (" + getModeFlagString(candidate->signature.requiredModeFlags) + ")";
                        }

                        report->log(message);
                        break;
                    }
                    default: {
                        std::abort();
                        break;
                    }
                }
            }
        }

        report->error("expression must be rewritten some other way\n", source->location);
    }

    void Compiler::raiseEmitBinaryExpressionError(const Expression* dest, BinaryOperatorKind op, const Expression* left, const Expression* right, SourceLocation location) {
        const auto candidates = builtins.findAllInstructionsByType(InstructionType(op));
        report->error("could not generate code for " + getBinaryOperatorName(op).toString(), right->location, ReportErrorFlags::Continued);

        auto destOperand = createOperandFromExpression(dest, false);
        auto leftOperand = createOperandFromExpression(left, false);
        auto rightOperand = createOperandFromExpression(right, false);

        if (destOperand == nullptr) {
            report->error("could not create an instruction operand for assignment destination of " + getBinaryOperatorName(op).toString(), location);
            return;
        }
        if (leftOperand == nullptr) {
            report->error("could not create an instruction operand for left-hand side of " + getBinaryOperatorName(op).toString(), location);
            return;
        }
        if (rightOperand == nullptr) {
            report->error("could not create an instruction operand for right-hand side of " + getBinaryOperatorName(op).toString(), location);
            return;
        }

        if (*destOperand == *leftOperand) {
            auto message = "got: `"
                + destOperand->toString()
                + " " + getBinaryOperatorSymbol(op).toString() + "= "
                + rightOperand->toString()
                + "`";
            if (modeFlags != 0) {
                message += " (" + getModeFlagString(modeFlags) + ")";
            }

            report->error(message, right->location, ReportErrorFlags::Continued);
        } else {
            auto message = "got: `"
                + destOperand->toString()
                + " " + leftOperand->toString()
                + " " + getBinaryOperatorSymbol(op).toString()
                + " " + rightOperand->toString()
                + "`";
            if (modeFlags != 0) {
                message += " (" + getModeFlagString(modeFlags) + ")";
            }

            report->error(message, right->location, ReportErrorFlags::Continued);
        }

        if (candidates.size() > 0) {
            report->error("possible options:", right->location, ReportErrorFlags::Continued);
            for (const auto candidate : candidates) {
                switch (candidate->signature.operandPatterns.size()) {
                    case 2: {
                        auto message = "  `"
                            + candidate->signature.operandPatterns[0]->toString()
                            + " " + getBinaryOperatorSymbol(op).toString()
                            + "= " + candidate->signature.operandPatterns[1]->toString()
                            + "`";
                        if (candidate->signature.requiredModeFlags != 0) {
                            message += " (" + getModeFlagString(candidate->signature.requiredModeFlags) + ")";
                        }

                        report->log(message);
                        break;
                    }
                    case 3: {
                        auto message = "  `"
                            + candidate->signature.operandPatterns[0]->toString()
                            + " = " + candidate->signature.operandPatterns[1]->toString()
                            + " " + getBinaryOperatorSymbol(op).toString()
                            + " " + candidate->signature.operandPatterns[2]->toString()
                            + "`";
                        if (candidate->signature.requiredModeFlags != 0) {
                            message += " (" + getModeFlagString(candidate->signature.requiredModeFlags) + ")";
                        }

                        report->log(message);
                        break;
                    }
                    default: {
                        std::abort();
                        break;
                    }
                }
            }
        }

        report->error("expression must be rewritten some other way\n", right->location);
    }

    void Compiler::raiseEmitIntrinsicError(const InstructionType& instructionType, const std::vector<InstructionOperandRoot>& operandRoots, SourceLocation location) {
        std::string intrinsicName;
        bool isLoadIntrinsic = false;
        if (const auto voidIntrinsic = instructionType.tryGet<InstructionType::VoidIntrinsic>()) {
            intrinsicName = voidIntrinsic->definition->name.toString();
        } else if (const auto loadIntrinsic = instructionType.tryGet<InstructionType::LoadIntrinsic>()) {
            intrinsicName = loadIntrinsic->definition->name.toString();
            isLoadIntrinsic = true;
        } else {
            std::abort();
            return;
        }

        const auto candidates = builtins.findAllInstructionsByType(instructionType);
        report->error("could not generate code for intrinsic `" + intrinsicName + "`", location, ReportErrorFlags::Continued);

        for (std::size_t i = 0; i != operandRoots.size(); ++i) {
            const auto& operand = operandRoots[i].operand;
            if (operand == nullptr) {
                if (isLoadIntrinsic && i == 0) {
                    report->error("could not create an instruction operand for assignment destination of instrinsic `" + intrinsicName + "`", location);
                } else {
                    report->error("could not create an instruction operand for argument #" + std::to_string(i - (isLoadIntrinsic ? 1 : 0) + 1) + " to instrinsic `" + intrinsicName + "`", location);
                }
                return;
            }
        }

        {
            std::string message = "got: `"
                + (isLoadIntrinsic ? operandRoots[0].operand->toString() + " = " : "")
                + intrinsicName + "(";
            bool comma = false;
            for (const auto& argument : operandRoots) {
                if (!isLoadIntrinsic || argument.operand != operandRoots[0].operand) {
                    message += (comma ? ", " : "") + argument.operand->toString();
                    comma = true;
                }
            }
            message += ")`";
            if (modeFlags != 0) {
                message += " (" + getModeFlagString(modeFlags) + ")";
            }

            report->error(message, location, ReportErrorFlags::Continued);
        }

        if (candidates.size() > 0) {
            report->error("possible options:", location, ReportErrorFlags::Continued);
            for (const auto candidate : candidates) {
                std::string message = "  `"
                    + (isLoadIntrinsic ? candidate->signature.operandPatterns[0]->toString() + " = " : "")
                    + intrinsicName + "(";
                bool comma = false;
                for (const auto operandPattern : candidate->signature.operandPatterns) {
                    if (!isLoadIntrinsic || operandPattern != candidate->signature.operandPatterns[0]) {
                        message += (comma ? ", " : "") + operandPattern->toString();
                        comma = true;
                    }
                }
                message += ")`";
                if (candidate->signature.requiredModeFlags != 0) {
                    message += " (" + getModeFlagString(candidate->signature.requiredModeFlags) + ")";
                }

                report->log(message);
            }
        }

        report->error("expression must be rewritten some other way\n", location);
    }

    bool Compiler::emitAssignmentExpressionIr(const Expression* dest, const Expression* source, SourceLocation location) {
        // TODO: figure out a more general way to handle nested assigments and increments.

        if (isLeafExpression(source)) {
            if (isSimpleCast(source)) {
                return emitAssignmentExpressionIr(dest, source->cast.operand.get(), location);
            } 

            if (!emitLoadExpressionIr(dest, source, dest->location)) {
                if (hasNestedAssignment(source)) {
                    auto strippedSource = expressionPool.add(stripNestedAssignment(source));
                    return emitNestedAssignmentIr(source, true, false)
                        && emitAssignmentExpressionIr(dest, strippedSource, location)
                        && emitNestedAssignmentIr(source, false, true);
                } else if (hasNestedAssignment(dest)) {
                    auto strippedDest = expressionPool.add(stripNestedAssignment(dest));
                    return emitNestedAssignmentIr(dest, true, false)
                        && emitAssignmentExpressionIr(strippedDest, source, location)
                        && emitNestedAssignmentIr(dest, false, true);
                } else {
                    raiseEmitLoadError(dest, source, dest->location);
                    return false;
                }
            }
            return true;
        } else if (const auto binaryOperator = source->tryGet<Expression::BinaryOperator>()) {
            const auto left = binaryOperator->left.get();
            const auto right = binaryOperator->right.get();
            const auto op = binaryOperator->op;
            if (op == BinaryOperatorKind::Assignment) {
                if(!emitAssignmentExpressionIr(left, right, left->location)) {
                    return false;
                }
                if (!emitAssignmentExpressionIr(dest, left, dest->location)) {
                    return false;
                }
                return true;
            } else {
                if (emitBinaryExpressionIr(dest, op, left, right, dest->location)) {
                    return true;
                } else {
                    if (!emitAssignmentExpressionIr(dest, left, left->location)) {
                        return false;
                    }

                    bool tempRequired = true;

                    if (isLeafExpression(right)) {
                        if (!emitBinaryExpressionIr(dest, op, dest, right, right->location)) {
                            if (hasNestedAssignment(right)) {
                                auto strippedRight = expressionPool.add(stripNestedAssignment(right));
                                if (!emitNestedAssignmentIr(right, true, false)) {
                                    return false;
                                }
                                if (!emitBinaryExpressionIr(dest, op, dest, strippedRight, right->location)) {
                                    raiseEmitBinaryExpressionError(dest, op, dest, strippedRight, right->location);
                                    return false;
                                }
                                if (!emitNestedAssignmentIr(right, false, true)) {
                                    return false;
                                }
                            } else {
                                raiseEmitBinaryExpressionError(dest, op, dest, right, right->location);
                                return false;
                            }
                        }

                        tempRequired = false;
                    } else if (hasNestedAssignment(right)) {
                        auto strippedRight = expressionPool.add(stripNestedAssignment(right));
                        if (!emitNestedAssignmentIr(right, true, false)) {
                            return false;
                        }
                        if (!emitBinaryExpressionIr(dest, op, dest, strippedRight, right->location)) {
                            raiseEmitBinaryExpressionError(dest, op, dest, strippedRight, right->location);
                            return false;
                        }
                        if (!emitNestedAssignmentIr(right, false, true)) {
                            return false;
                        }

                        tempRequired = false;
                    }

                    if ((dest->info->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) {
                        report->error(getBinaryOperatorName(op).toString() + " expression cannot be done in-place because destination is `writeonly`, so it would require a temporary", right->location);
                        return false;
                    }

                    if (tempRequired) {
                        report->error(getBinaryOperatorName(op).toString() + " expression would require a temporary", right->location);
                        return false;
                    } else {
                        return true;
                    }
                }
            }
        } else if (const auto unaryOperator = source->tryGet<Expression::UnaryOperator>()) {
            const auto operand = unaryOperator->operand.get();
            const auto op = unaryOperator->op;
            if (emitUnaryExpressionIr(dest, op, operand, dest->location)) {
                return true;
            } else {
                // Incrementation of the source must happen on the source operand directly.
                if (isUnaryIncrementOperator(op)) {
                    if (isUnaryPostIncrementOperator(op)) {
                        if (!emitAssignmentExpressionIr(dest, operand, operand->location)) {
                            return false;
                        }
                        if (!emitUnaryExpressionIr(operand, getUnaryPreIncrementEquivalent(op), operand, operand->location)) {
                            raiseEmitUnaryExpressionError(operand, op, operand, operand->location);
                            return false;
                        }
                    } else if (isUnaryPreIncrementOperator(op)) {
                        if (!emitUnaryExpressionIr(operand, op, operand, operand->location)) {
                            raiseEmitUnaryExpressionError(operand, op, operand, operand->location);
                            return false;
                        }
                        if (!emitAssignmentExpressionIr(dest, operand, operand->location)) {
                            raiseEmitUnaryExpressionError(operand, op, operand, operand->location);
                            return false;
                        }
                    }
                // Other unary operations can have their operand loaded into the destination first and then be computed on the destination.
                } else {
                    if (!emitAssignmentExpressionIr(dest, operand, operand->location)) {
                        return false;
                    }
                    if (!emitUnaryExpressionIr(dest, op, dest, operand->location)) {
                        raiseEmitUnaryExpressionError(dest, op, dest, operand->location);
                        return false;
                    }
                }

                if ((dest->info->qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) {
                    report->error(getUnaryOperatorName(op).toString() + " expression cannot be done in-place because destination is `writeonly`, so it would require a temporary", operand->location);
                    return false;
                }

                return true;
            }
        } else if (const auto call = source->tryGet<Expression::Call>()) {
            return emitCallExpressionIr(0, call->inlined, false, dest, call->function.get(), call->arguments, location);
        }

        return false;
    }

    bool Compiler::emitExpressionStatementIr(const Expression* expression, SourceLocation location) {
        if (const auto binaryOperator = expression->tryGet<Expression::BinaryOperator>()) {
            switch (binaryOperator->op) {
                case BinaryOperatorKind::Assignment: {
                    return emitAssignmentExpressionIr(binaryOperator->left.get(), binaryOperator->right.get(), binaryOperator->left->location);
                }
                default:
                    break;
            }
        } else if (const auto unaryOperator = expression->tryGet<Expression::UnaryOperator>()) {
            const auto operand = unaryOperator->operand.get();
            const auto op = unaryOperator->op;
            switch (op) {
                case UnaryOperatorKind::PreIncrement:
                case UnaryOperatorKind::PostIncrement: {
                    if (!emitUnaryExpressionIr(operand, UnaryOperatorKind::PreIncrement, operand, operand->location)) {
                        raiseEmitUnaryExpressionError(operand, UnaryOperatorKind::PreIncrement, operand, operand->location);
                        return false;
                    }
                    return true;
                }
                case UnaryOperatorKind::PreDecrement:
                case UnaryOperatorKind::PostDecrement: {
                    if (!emitUnaryExpressionIr(operand, UnaryOperatorKind::PreDecrement, operand, operand->location)) {
                        raiseEmitUnaryExpressionError(operand, UnaryOperatorKind::PreDecrement, operand, operand->location);
                    }
                    return true;
                }
                default: break;
            }
        } else if (const auto call = expression->tryGet<Expression::Call>()) {
            return emitCallExpressionIr(0, call->inlined, false, nullptr, call->function.get(), call->arguments, location);
        }

        report->error("expression provided cannot be used as a statement", location);
        return false;
    }

    bool Compiler::emitReturnAssignmentIr(std::size_t distanceHint, const TypeExpression* returnType, const Expression* returnValue, SourceLocation location) {
        if (const auto designatedStorageType = returnType->tryGet<TypeExpression::DesignatedStorage>()) {
            const auto holderExpression = designatedStorageType->holder.get();

            if (const auto call = returnValue->tryGet<Expression::Call>()) {
                return emitCallExpressionIr(distanceHint, call->inlined, true, holderExpression, call->function.get(), call->arguments, location);
            } else {
                return emitAssignmentExpressionIr(holderExpression, returnValue, returnValue->location);
            }
        } else {
            report->error("could not generate initializer for return value of type `" + getTypeName(returnType) + "`", location);
            return false;
        }
        return false;
    }

    std::unique_ptr<PlatformTestAndBranch> Compiler::getTestAndBranch(BinaryOperatorKind op, const Expression* left, const Expression* right, std::size_t distanceHint) const {
        auto innerLeft = left;
        auto innerRight = right;
        while (isSimpleCast(innerLeft)) {
            innerLeft = left->cast.operand.get();
        }
        while (isSimpleCast(innerRight)) {
            innerRight = right->cast.operand.get();
        } 
        
        if (const auto commonType = findCompatibleBinaryArithmeticExpressionType(left, right)) {
            const auto definition = tryGetResolvedIdentifierTypeDefinition(commonType);
            return platform->getTestAndBranch(*this, definition, op, innerLeft, innerRight, distanceHint);
        } else if (isBooleanType(left->info->type.get()) && isBooleanType(right->info->type.get())) {
            const auto definition = tryGetResolvedIdentifierTypeDefinition(left->info->type.get());
            return platform->getTestAndBranch(*this, definition, op, innerLeft, innerRight, distanceHint);
        } else {
            return nullptr;
        }
    }

    bool Compiler::emitBranchIr(std::size_t distanceHint, BranchKind kind, const Expression* destination, const Expression* returnValue, bool negated, const Expression* condition, SourceLocation location) {
        switch (kind) {
            case BranchKind::Continue: {
                if (breakLabel != nullptr) {                    
                    const auto labelReferenceExpresison = expressionPool.add(resolveDefinitionExpression(continueLabel, {}, location));
                    return emitBranchIr(distanceHint, BranchKind::Goto, labelReferenceExpresison, returnValue, negated, condition, location);
                } else {
                    report->error("`continue` cannot be used outside of a loop", location);
                    return false;
                }
            }
            case BranchKind::Break: {
                if (breakLabel != nullptr) {
                    const auto labelReferenceExpresison = expressionPool.add(resolveDefinitionExpression(breakLabel, {}, location));
                    return emitBranchIr(distanceHint, BranchKind::Goto, labelReferenceExpresison, returnValue, negated, condition, location);
                } else {
                    report->error("`break` cannot be used outside of a loop", location);
                    return false;
                }
            }
            case BranchKind::Return: {
                if (currentFunction != nullptr) {
                    const auto& funcDefinition = currentFunction->variant.get<Definition::Func>();
                    const auto returnType = funcDefinition.resolvedSignatureType->function.returnType.get();
                    bool isVoid = isEmptyTupleType(returnType);
                    bool needsIntermediateBranch = false;

                    if (returnValue != nullptr) {
                        if (condition != nullptr) {
                            needsIntermediateBranch = true;

                            if (const auto designatedStorageType = returnType->tryGet<TypeExpression::DesignatedStorage>()) {
                                auto destOperand = createOperandFromExpression(designatedStorageType->holder.get(), true);
                                auto sourceOperand = createOperandFromExpression(returnValue, true);

                                if (destOperand != nullptr && sourceOperand != nullptr && *destOperand == *sourceOperand) {
                                    needsIntermediateBranch = false;
                                }
                            }
                        }

                        if (isVoid) {
                            if (const auto call = returnValue->tryGet<Expression::Call>()) {
                                return emitCallExpressionIr(distanceHint, call->inlined, true, nullptr, call->function.get(), call->arguments, location);
                            } else {
                                report->error("`return` value of `func` returning `()` can only be a function call", location);
                                return false;
                            }
                        } else if (!needsIntermediateBranch) {
                            if (!emitReturnAssignmentIr(distanceHint, returnType, returnValue, location)) {
                                return false;
                            }
                        }
                    } else if (returnValue == nullptr) {
                        if (!isVoid) {
                            report->error("expected `return` value of type `" + getTypeName(returnType) + "` but got empty `return;` nstead.", location);
                            return false;
                        }
                    }

                    // If a function has a different return convention use that instead of normal return.
                    const auto returnKind = funcDefinition.returnKind;

                    if (needsIntermediateBranch) {
                        const auto oldFunction = currentFunction;
                        currentFunction = nullptr;

                        const auto failureLabelDefinition = createAnonymousLabelDefinition("$skip"_sv);
                        const auto failureReferenceExpression = expressionPool.add(resolveDefinitionExpression(failureLabelDefinition, {}, location));

                        bool result = emitBranchIr(distanceHint, BranchKind::Goto, failureReferenceExpression, nullptr, !negated, condition, condition->location)
                            && emitReturnAssignmentIr(distanceHint, returnType, returnValue, location)
                            && emitBranchIr(distanceHint, returnKind, nullptr, nullptr, false, nullptr, location);

                        currentFunction = oldFunction;
                        irNodes.addNew(IrNode::Label(failureLabelDefinition), location);
                        return result;
                    } else if (returnKind != BranchKind::Return) {
                        if (returnLabel != nullptr) {
                            // inline functions should jump to a "return label" instead of actually returning.
                            const auto labelReferenceExpresison = expressionPool.add(resolveDefinitionExpression(returnLabel, {}, location));
                            return emitBranchIr(distanceHint, BranchKind::Goto, labelReferenceExpresison, returnValue, negated, condition, location);
                        } else {
                            return emitBranchIr(distanceHint, returnKind, nullptr, returnValue, negated, condition, location);
                        }
                    }
                }
                break;
            }
            case BranchKind::None: {
                return true;
            }
            default: break;
        }

        if (destination) {
            if (destination->info->type->kind != TypeExpressionKind::Function) {
                report->error("branch destination must be a label or function, but got expression of type `" + getTypeName(destination->info->type.get()) + "`", destination->location);
                return false;
            }
        }

        if (condition) {
            if (!isBooleanType(condition->info->type.get())) {
                report->error("branch conditional must be a boolean expression, but got expression of type `" + getTypeName(condition->info->type.get()) + "`", destination->location);
                return false;
            }

            if (const auto unaryOperator = condition->tryGet<Expression::UnaryOperator>()) {
                const auto op = unaryOperator->op;
                if (op == UnaryOperatorKind::LogicalNegation) {
                    return emitBranchIr(distanceHint, kind, destination, returnValue, !negated, unaryOperator->operand.get(), condition->location);
                } else {
                    report->error(getUnaryOperatorName(op).toString() + " operator is not allowed in conditional", destination->location);
                }
            } else if (const auto binaryOperator = condition->tryGet<Expression::BinaryOperator>()) {
                const auto preNegated = negated && getBinaryOperatorLogicalNegation(binaryOperator->op) != BinaryOperatorKind::None;
                const auto op = preNegated ? getBinaryOperatorLogicalNegation(binaryOperator->op) : binaryOperator->op;
                const auto left = binaryOperator->left.get();
                const auto right = binaryOperator->right.get();

                auto testAndBranch = getTestAndBranch(op, left, right, distanceHint);

                // If failed to find a test-and-branch, try "flipping" the comparison.
                // a == b -> b == a, a < b -> b > a, a <= b -> b >= a.
                if (testAndBranch == nullptr) {
                    switch (op) {
                        case BinaryOperatorKind::Equal:
                        case BinaryOperatorKind::NotEqual:
                        {
                            testAndBranch = getTestAndBranch(op, right, left, distanceHint);
                            break;
                        }
                        case BinaryOperatorKind::LessThan:
                        case BinaryOperatorKind::GreaterThan:
                        {
                            testAndBranch = getTestAndBranch(op == BinaryOperatorKind::LessThan ? BinaryOperatorKind::GreaterThan : BinaryOperatorKind::LessThan, right, left, distanceHint);
                            break;
                        }
                        case BinaryOperatorKind::LessThanOrEqual:
                        case BinaryOperatorKind::GreaterThanOrEqual:
                        {
                            testAndBranch = getTestAndBranch(op == BinaryOperatorKind::LessThanOrEqual ? BinaryOperatorKind::GreaterThanOrEqual : BinaryOperatorKind::LessThanOrEqual, right, left, distanceHint);
                            break;
                        }
                        default: break;
                    }
                }

                if (testAndBranch != nullptr) {
                    std::vector<InstructionOperandRoot> operandRoots;
                    operandRoots.reserve(testAndBranch->testOperands.size() + (testAndBranch->branches.size() == 0 ? 1 : 0));

                    for (const auto testOperand : testAndBranch->testOperands) {
                        auto operand = createOperandFromExpression(testOperand, true);
                        operandRoots.push_back(InstructionOperandRoot(testOperand, std::move(operand)));
                    }

                    if (testAndBranch->branches.size() == 0) {
                        if (kind == BranchKind::Goto || kind == BranchKind::FarGoto) {
                            if (destination != nullptr) {
                                auto operand = createOperandFromExpression(destination, true);
                                if (!operand) {
                                    return false;
                                }

                                operandRoots.push_back(InstructionOperandRoot(destination, std::move(operand)));
                            }

                            if (const auto instruction = builtins.selectInstruction(testAndBranch->testInstructionType, modeFlags, operandRoots)) {
                                irNodes.addNew(IrNode::Code(instruction, std::move(operandRoots)), location);
                                return true;
                            }
                        }

                        return false;
                    } else {
                        if (const auto testInstruction = builtins.selectInstruction(testAndBranch->testInstructionType, modeFlags, operandRoots)) {
                            irNodes.addNew(IrNode::Code(testInstruction, std::move(operandRoots)), location);
                        } else {
                            return false;
                        }

                        const auto failureLabelDefinition = createAnonymousLabelDefinition("$skip"_sv);
                        const auto failureReferenceExpression = expressionPool.add(resolveDefinitionExpression(failureLabelDefinition, {}, location));

                        for (const auto& branch : testAndBranch->branches) {
                            const auto flagReferenceExpression = expressionPool.add(resolveDefinitionExpression(branch.flag, {}, condition->location));

                            if (!emitBranchIr(distanceHint, kind, branch.success ? destination : failureReferenceExpression, returnValue, (preNegated ? !negated : negated) ? branch.value : !branch.value, flagReferenceExpression, condition->location)) {
                                return false;
                            }
                        }

                        irNodes.addNew(IrNode::Label(failureLabelDefinition), location);
                        return true;
                    }
                }

                switch (op) {
                    case BinaryOperatorKind::LogicalAnd: {
                        if (!negated) {
                            const auto failureLabelDefinition = createAnonymousLabelDefinition("$skip"_sv);
                            const auto failureLabelReferenceExpression = expressionPool.add(resolveDefinitionExpression(failureLabelDefinition, {}, condition->location));

                            if (!emitBranchIr(distanceHint, kind, failureLabelReferenceExpression, returnValue, true, binaryOperator->left.get(), condition->location)
                            || !emitBranchIr(distanceHint, kind, destination, returnValue, false, binaryOperator->right.get(), condition->location)) {
                                return false;
                            }

                            irNodes.addNew(IrNode::Label(failureLabelDefinition), location);
                            return true;
                        } else {
                            return emitBranchIr(distanceHint, kind, destination, returnValue, true, binaryOperator->left.get(), condition->location)
                            && emitBranchIr(distanceHint, kind, destination, returnValue, true, binaryOperator->right.get(), condition->location);
                        }
                    }
                    case BinaryOperatorKind::LogicalOr: {
                        if (!negated) {
                            return emitBranchIr(distanceHint, kind, destination, returnValue, false, binaryOperator->left.get(), condition->location)
                            && emitBranchIr(distanceHint, kind, destination, returnValue, false, binaryOperator->right.get(), condition->location);
                        } else {
                            const auto failureLabelDefinition = createAnonymousLabelDefinition("$skip"_sv);                        
                            const auto failureLabelReferenceExpression = expressionPool.add(resolveDefinitionExpression(failureLabelDefinition, {}, condition->location));

                            if (!emitBranchIr(distanceHint, kind, failureLabelReferenceExpression, returnValue, false, binaryOperator->left.get(), condition->location)
                            || !emitBranchIr(distanceHint, kind, destination, returnValue, true, binaryOperator->right.get(), condition->location)) {
                                return false;
                            }

                            irNodes.addNew(IrNode::Label(failureLabelDefinition), location);
                            return true;
                        }
                    }
                    default: {
                        const auto failingOperator = negated ? getBinaryOperatorLogicalNegation(op) : op;
                        report->error(getBinaryOperatorName(failingOperator).toString() + " operator is not allowed in conditional", condition->location);
                    }
                }
            } else if (const auto booleanLiteral = condition->tryGet<Expression::BooleanLiteral>()) {
                if (booleanLiteral->value != negated) {
                    return emitBranchIr(distanceHint, kind, destination, returnValue, false, nullptr, condition->location);
                } else {
                    // condition is known to be false, no branch generated.
                    return true;
                }
            } else if (const auto resolvedIdentifier = condition->tryGet<Expression::ResolvedIdentifier>()) {
                if (const auto builtinRegister = resolvedIdentifier->definition->variant.tryGet<Definition::BuiltinRegister>()) {
                    static_cast<void>(builtinRegister);
                    
                    std::vector<InstructionOperandRoot> operandRoots;
                    operandRoots.reserve((destination != nullptr ? 1 : 0) + 3);

                    operandRoots.push_back(InstructionOperandRoot(nullptr, makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(Int128(distanceHint)))));

                    if (destination != nullptr) {
                        auto operand = createOperandFromExpression(destination, true);
                        if (!operand) {
                            return false;
                        }

                        operandRoots.push_back(InstructionOperandRoot(destination, std::move(operand)));
                    }

                    operandRoots.push_back(InstructionOperandRoot(nullptr, makeFwdUnique<InstructionOperand>(InstructionOperand::Register(resolvedIdentifier->definition))));
                    operandRoots.push_back(InstructionOperandRoot(nullptr, makeFwdUnique<InstructionOperand>(InstructionOperand::Boolean(!negated))));                    

                    if (const auto instruction = builtins.selectInstruction(InstructionType(kind), modeFlags, operandRoots)) {
                        irNodes.addNew(IrNode::Code(instruction, std::move(operandRoots)), location);
                        return true;
                    } else {
                        // If that fails, try to branch-on-opposite around a return.
                        const auto oldFunction = currentFunction;
                        currentFunction = nullptr;

                        const auto failureLabelDefinition = createAnonymousLabelDefinition("$skip"_sv);
                        const auto failureReferenceExpression = expressionPool.add(resolveDefinitionExpression(failureLabelDefinition, {}, location));

                        bool result = emitBranchIr(distanceHint, BranchKind::Goto, failureReferenceExpression, nullptr, !negated, condition, condition->location)
                            && emitBranchIr(distanceHint, kind, nullptr, nullptr, false, nullptr, location);

                        currentFunction = oldFunction;

                        if (result) {
                            irNodes.addNew(IrNode::Label(failureLabelDefinition), location);
                            return true;
                        }

                        return false;
                    }
                } else {
                    report->error("`" + getResolvedIdentifierName(resolvedIdentifier->definition, resolvedIdentifier->pieces) + "` cannot be used as conditional term", condition->location);
                    return false;
                }
            } else if (const auto sideEffect = condition->tryGet<Expression::SideEffect>()) {
                return emitStatementIr(sideEffect->statement.get())
                    && emitBranchIr(distanceHint, kind, destination, returnValue, negated, sideEffect->result.get(), location);
            }
        } else {
            std::vector<InstructionOperandRoot> operandRoots;
            operandRoots.reserve((destination != nullptr ? 1 : 0) + 1);

            operandRoots.push_back(InstructionOperandRoot(nullptr, makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(Int128(distanceHint)))));

            if (destination != nullptr) {
                auto operand = createOperandFromExpression(destination, true);
                if (!operand) {
                    return false;
                }

                operandRoots.push_back(InstructionOperandRoot(destination, std::move(operand)));

                if ((destination->info->qualifiers & Qualifiers::Far) != Qualifiers::None) {
                    switch (kind) {
                        case BranchKind::Goto: kind = BranchKind::FarGoto; break;
                        case BranchKind::Call: kind = BranchKind::FarCall; break;
                        default: break;
                    }
                }
            }

            if (const auto instruction = builtins.selectInstruction(InstructionType(kind), modeFlags, operandRoots)) {
                irNodes.addNew(IrNode::Code(instruction, std::move(operandRoots)), location);
                return true;
            } else {
                return false;
            }
        }

        return false;
    }

    bool Compiler::hasUnconditionalReturn(const Statement* statement) const {
        switch (statement->kind) {
            case StatementKind::Block: {
                const auto& block = statement->block;
                const auto& items = block.items;
                return items.size() != 0 && hasUnconditionalReturn(items.back().get());
            }
            case StatementKind::Branch: {
                const auto& branch = statement->branch;
                switch (branch.kind) {
                    case BranchKind::Goto:
                    case BranchKind::FarGoto:
                    case BranchKind::Return:
                    case BranchKind::IrqReturn:
                    case BranchKind::NmiReturn:
                    case BranchKind::FarReturn:
                        return branch.condition == nullptr;
                    default: return false;
                }
            }
            case StatementKind::If: {
                const auto& if_ = statement->if_;
                return if_.body != nullptr
                && if_.alternative != nullptr
                && hasUnconditionalReturn(if_.body.get())
                && hasUnconditionalReturn(if_.alternative.get());
            }
            default: return false;
        }
    }

    bool Compiler::emitFunctionIr(Definition* definition, SourceLocation location) {
        auto& funcDefinition = definition->variant.get<Definition::Func>();

        const auto oldFunction = currentFunction;
        const auto oldReturnLabel = returnLabel;
        const auto onExit = makeScopeGuard([&]() {
            currentFunction = oldFunction;
            returnLabel = oldReturnLabel;
        });        

        currentFunction = definition;
        const auto returnKind = funcDefinition.returnKind;
        const auto returnType = funcDefinition.resolvedSignatureType->function.returnType.get();

        returnLabel = nullptr;
        if (returnKind == BranchKind::None) {
            returnLabel = createAnonymousLabelDefinition("$ret"_sv);
        }

        if (!funcDefinition.inlined) {
            irNodes.addNew(IrNode::Label(currentFunction), location);
        }

        funcDefinition.hasUnconditionalReturn = funcDefinition.hasUnconditionalReturn || hasUnconditionalReturn(funcDefinition.body);

        if (!emitStatementIr(funcDefinition.body)) {
            return false;
        }

        // TODO: omit trailing return if the function is determined to never reach the return statement (nothing breaks the loop)
        // TODO: error if there is no return at end of non-void func unless it is marked #[fallthrough] (and we determined an implicit return would have been needed)

        if (!funcDefinition.hasUnconditionalReturn && !isEmptyTupleType(returnType)) {
            report->error("`" + definition->name.toString() + "` is missing return value of type `" + getTypeName(returnType) + "`", location);
        }

        if (!funcDefinition.fallthrough && returnKind != BranchKind::None && isEmptyTupleType(returnType) && !funcDefinition.hasUnconditionalReturn) {
            if (!emitBranchIr(0, returnKind, nullptr, nullptr, false, nullptr, location)) {
                report->error("could not generate return instruction for " + definition->declaration->getDescription().toString(), location);
                return false;
            }
        }

        if (returnLabel != nullptr) {
            irNodes.addNew(IrNode::Label(returnLabel), location);
        }

        return true;
    }

    bool Compiler::emitStatementIr(const Statement* statement) {
        switch (statement->kind) {
            case StatementKind::Attribution: {
                const auto& attributedStatement = statement->attribution;
                pushAttributeList(statementAttributeLists[statement]);
                if (checkConditionalCompilationAttributes()) {
                    emitStatementIr(attributedStatement.body.get());
                }
                popAttributeList();
                break;
            }
            case StatementKind::Bank: break;
            case StatementKind::Block: {
                const auto& blockStatement = statement->block;
                enterScope(getOrCreateStatementScope(StringView(), statement, currentScope));
                for (const auto& item : blockStatement.items) {
                    emitStatementIr(item.get());
                }
                exitScope();
                break;
            }
            case StatementKind::Config: {
                const auto& configStatement = statement->config;
                for (const auto& item : configStatement.items) {
                    if (auto reducedValue = reduceExpression(item->value.get())) {
                        config->add(report, item->name, std::move(reducedValue));
                    }
                }
                break;
            }
            case StatementKind::DoWhile: {
                const auto& doWhileStatement = statement->doWhile;
                if (currentBank == nullptr) {
                    report->error(statement->getDescription().toString() + " must be inside an `in` statement", statement->location);
                    break;
                }

                const auto oldContinueLabel = continueLabel;
                const auto oldBreakLabel = breakLabel;
                const auto onExit = makeScopeGuard([&]() {
                    continueLabel = oldContinueLabel;
                    breakLabel = oldBreakLabel;
                });

                const auto reducedCondition = expressionPool.add(reduceExpression(doWhileStatement.condition.get()));
                if (!reducedCondition) {
                    break;
                }

                const auto beginLabelDefinition = createAnonymousLabelDefinition("$loop"_sv);                
                const auto beginLabelReferenceExpression = expressionPool.add(resolveDefinitionExpression(beginLabelDefinition, {}, statement->location));
                const auto continueLabelDefinition = createAnonymousLabelDefinition("$continue"_sv);
                const auto endLabelDefinition = createAnonymousLabelDefinition("$endloop"_sv);

                continueLabel = continueLabelDefinition;
                breakLabel = endLabelDefinition;

                irNodes.addNew(IrNode::Label(beginLabelDefinition), statement->location);
                emitStatementIr(doWhileStatement.body.get());
                irNodes.addNew(IrNode::Label(continueLabelDefinition), reducedCondition->location);
                if (!emitBranchIr(doWhileStatement.distanceHint, BranchKind::Goto, beginLabelReferenceExpression, nullptr, false, reducedCondition, reducedCondition->location)) {
                    report->error("could not generate branch instruction for " + statement->getDescription().toString(), statement->location);
                    break;
                }
                irNodes.addNew(IrNode::Label(endLabelDefinition), reducedCondition->location);
                break;
            }
            case StatementKind::Enum: break;
            case StatementKind::ExpressionStatement: {
                const auto& expressionStatement = statement->expressionStatement;
                if (currentBank == nullptr) {
                    report->error(statement->getDescription().toString() + " must be inside an `in` statement", statement->location);
                    break;
                }

                auto reducedExpression = expressionPool.add(reduceExpression(expressionStatement.expression.get()));
                if (!reducedExpression) {
                    break;
                }

                if (!emitExpressionStatementIr(reducedExpression, reducedExpression->location)) {
                    report->error("could not generate code for statement", statement->location);
                }
                break;
            }
            case StatementKind::File: {
                const auto& file = statement->file;
                enterScope(findStatementScope(statement));
                for (const auto& item : file.items) {
                    emitStatementIr(item.get());
                }
                exitScope();
                break;
            }
            case StatementKind::For: {
                const auto& forStatement = statement->for_;
                if (currentBank == nullptr) {
                    report->error(statement->getDescription().toString() + " must be inside an `in` statement", statement->location);
                    break;
                }

                const auto oldContinueLabel = continueLabel;
                const auto oldBreakLabel = breakLabel;
                const auto onExit = makeScopeGuard([&]() {
                    continueLabel = oldContinueLabel;
                    breakLabel = oldBreakLabel;
                });

                const auto reducedCounter = expressionPool.add(reduceExpression(forStatement.counter.get()));
                const auto reducedSequence = expressionPool.add(reduceExpression(forStatement.sequence.get()));
                if (!reducedCounter || !reducedSequence) {
                    break;
                }
                if (reducedSequence->kind != ExpressionKind::RangeLiteral || reducedSequence->info->context == EvaluationContext::RunTime) {
                    report->error("`for` loop range must be a compile-time range literal.", statement->location);
                    break;
                }

                const auto& rangeLiteral = reducedSequence->tryGet<Expression::RangeLiteral>();
                const auto counterResolvedIdentifierType = reducedCounter->info->type->tryGet<TypeExpression::ResolvedIdentifier>();
                const auto counterBuiltinIntegerType = counterResolvedIdentifierType ? counterResolvedIdentifierType->definition->variant.tryGet<Definition::BuiltinIntegerType>() : nullptr;
                if (!counterBuiltinIntegerType) {
                    report->error("`for` loop counter start must be a sized integer type.", statement->location);
                    break;
                }

                const auto rangeStart = rangeLiteral->start->tryGet<Expression::IntegerLiteral>();
                const auto rangeEnd = rangeLiteral->end->tryGet<Expression::IntegerLiteral>();
                const auto rangeStep = rangeLiteral->step->tryGet<Expression::IntegerLiteral>();

                if (!rangeStart) {
                    report->error("`for` loop range start must be a compile-time integer literal.", statement->location);
                    break;
                }
                if (!rangeEnd) {
                    report->error("`for` loop range end must be a compile-time integer literal.", statement->location);
                    break;
                }
                if (!rangeStep) {
                    report->error("`for` loop range step must be a compile-time integer literal.", statement->location);
                    break;
                }
                if (rangeStep->value.isZero()) {
                    report->error("`for` loop range step must be non-zero.", statement->location);
                    break;
                }
                
                const auto beginLabelDefinition = createAnonymousLabelDefinition("$loop"_sv);
                const auto beginLabelReferenceExpression = expressionPool.add(resolveDefinitionExpression(beginLabelDefinition, {}, statement->location));
                const auto continueLabelDefinition = createAnonymousLabelDefinition("$continue"_sv);
                const auto endLabelDefinition = createAnonymousLabelDefinition("$endloop"_sv);

                continueLabel = continueLabelDefinition;
                breakLabel = endLabelDefinition;

                auto initAssignment = makeFwdUnique<Expression>(
                    Expression::BinaryOperator(BinaryOperatorKind::Assignment, reducedCounter->clone(), rangeLiteral->start->clone()),
                    reducedCounter->location,
                    Optional<ExpressionInfo>());
                auto reducedInitAssignment = expressionPool.add(reduceExpression(initAssignment.get()));

                bool conditionNegated = false;
                const Expression* reducedCondition = nullptr;

                const Instruction* incrementInstruction = nullptr;               
                std::vector<InstructionOperandRoot> incrementOperandRoots;

                if (rangeStep->value == Int128(1) || rangeStep->value == Int128(-1)) {
                    if (auto destOperand = createOperandFromExpression(reducedCounter, true)) {
                        const auto op = rangeStep->value.isPositive() ? UnaryOperatorKind::PreIncrement : UnaryOperatorKind::PreDecrement; 
                        incrementOperandRoots.reserve(1);
                        incrementOperandRoots.push_back(InstructionOperandRoot(reducedCounter, std::move(destOperand)));
                        incrementInstruction = builtins.selectInstruction(InstructionType(op), modeFlags, incrementOperandRoots);
                    }

                    if (incrementInstruction) {
                        if ((rangeStep->value.isPositive() && rangeStart->value > rangeEnd->value)
                        || (rangeStep->value.isNegative() && rangeStart->value < rangeEnd->value)) {
                            break;
                        }

                        if ((rangeStep->value.isPositive() && rangeEnd->value == counterBuiltinIntegerType->max)
                        || (rangeStep->value.isNegative() && rangeEnd->value == Int128(1))) {
                            if (const auto zeroFlag = platform->getZeroFlag()) {
                                const auto& affectedFlags = incrementInstruction->options.affectedFlags;
                                if (std::find(affectedFlags.begin(), affectedFlags.end(), zeroFlag) != affectedFlags.end()) {
                                    conditionNegated = true;
                                    reducedCondition = expressionPool.add(resolveDefinitionExpression(zeroFlag, {}, statement->location));
                                }
                            }
                            if (!reducedCondition) {
                                auto comparisonValue = makeFwdUnique<Expression>(
                                    Expression::IntegerLiteral(Int128(0)),
                                    reducedSequence->location,
                                    ExpressionInfo(EvaluationContext::CompileTime,
                                        makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), reducedSequence->location),
                                        Qualifiers::None));
                                auto condition = makeFwdUnique<Expression>(Expression::BinaryOperator(BinaryOperatorKind::NotEqual, reducedCounter->clone(), std::move(comparisonValue)), reducedCounter->location, Optional<ExpressionInfo>());
                                reducedCondition = expressionPool.add(reduceExpression(condition.get()));
                            }
                        } else if (rangeEnd->value >= Int128(0) && rangeEnd->value <= counterBuiltinIntegerType->max) {
                            auto comparisonValue = makeFwdUnique<Expression>(
                                Expression::IntegerLiteral(rangeEnd->value + rangeStep->value),
                                reducedSequence->location,
                                ExpressionInfo(EvaluationContext::CompileTime,
                                    makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(builtins.getDefinition(Builtins::DefinitionType::IExpr)), reducedSequence->location),
                                    Qualifiers::None));
                            auto condition = makeFwdUnique<Expression>(Expression::BinaryOperator(BinaryOperatorKind::NotEqual, reducedCounter->clone(), std::move(comparisonValue)), reducedCounter->location, Optional<ExpressionInfo>());
                            reducedCondition = expressionPool.add(reduceExpression(condition.get()));
                        }
                    }
                }

                if (!reducedCondition) {
                    report->error("`for` loop range of `" + rangeStart->value.toString()
                        + "` ..  `" + rangeEnd->value.toString() + "`"
                        + (rangeStep->value != Int128(1)
                            ? " by `" + rangeStep->value.toString() + "`"
                            : "")
                        + " is not supported.", reducedSequence->location);
                    break;
                }

                if (!incrementInstruction) {
                    report->error("could not generate increment instruction for " + statement->getDescription().toString(), statement->location);
                    break;
                }               

                // TODO: decrement-and-branch optimization if the instruction exists.

                if (!emitExpressionStatementIr(reducedInitAssignment, reducedInitAssignment->location)) {
                    report->error("could not generate initial assignment instruction for " + statement->getDescription().toString(), statement->location);
                    break;
                }
                irNodes.addNew(IrNode::Label(beginLabelDefinition), statement->location);
                emitStatementIr(forStatement.body.get());
                irNodes.addNew(IrNode::Label(continueLabelDefinition), reducedCondition->location);
                irNodes.addNew(IrNode::Code(incrementInstruction, std::move(incrementOperandRoots)), reducedCondition->location);
                if (!emitBranchIr(forStatement.distanceHint, BranchKind::Goto, beginLabelReferenceExpression, nullptr, conditionNegated, reducedCondition, reducedCondition->location)) {
                    report->error("could not generate branch instruction for " + statement->getDescription().toString(), statement->location);
                    break;
                }
                irNodes.addNew(IrNode::Label(endLabelDefinition), reducedCondition->location);
                break;
            }
            case StatementKind::Func: {
                const auto& funcDeclaration = statement->func;
                auto definition = currentScope->findLocalMemberDefinition(funcDeclaration.name);
                auto& funcDefinition = definition->variant.get<Definition::Func>();

                if (funcDefinition.inlined) {
                    //report->error("TODO: `inline func`", statement->location);
                    break;
                } else {
                    emitFunctionIr(definition, statement->location);
                }
                break;
            }
            case StatementKind::If: {
                const auto& ifStatement = statement->if_;
                if (currentBank == nullptr) {
                    report->error(statement->getDescription().toString() + " must be inside an `in` statement", statement->location);
                    break;
                }

                const auto reducedCondition = expressionPool.add(reduceExpression(ifStatement.condition.get()));
                if (!reducedCondition) {
                    break;
                }

                if (const auto booleanLiteral = reducedCondition->tryGet<Expression::BooleanLiteral>()) {
                    if (booleanLiteral->value) {
                        emitStatementIr(ifStatement.body.get());
                    } else {
                        if (ifStatement.alternative) {
                            emitStatementIr(ifStatement.alternative.get());
                        }
                    }

                    break;
                }

                const auto endLabelDefinition = createAnonymousLabelDefinition("$endif"_sv);
                const auto endLabelReferenceExpression = expressionPool.add(resolveDefinitionExpression(endLabelDefinition, {}, statement->location));
                const auto elseLabelDefinition = createAnonymousLabelDefinition("$else"_sv);
                const auto elseLabelReferenceExpression = expressionPool.add(resolveDefinitionExpression(elseLabelDefinition, {}, statement->location));

                if (!emitBranchIr(ifStatement.distanceHint, BranchKind::Goto, elseLabelReferenceExpression, nullptr, true, reducedCondition, statement->location)) {
                    report->error("could not generate branch instruction for " + statement->getDescription().toString(), statement->location);
                    break;
                }

                emitStatementIr(ifStatement.body.get());
                if (ifStatement.alternative) {
                    if (!emitBranchIr(ifStatement.distanceHint, BranchKind::Goto, endLabelReferenceExpression, nullptr, false, nullptr, statement->location)) {
                        report->error("could not generate branch instruction for " + statement->getDescription().toString(), statement->location);
                        break;
                    }
                    irNodes.addNew(IrNode::Label(elseLabelDefinition), statement->location);
                    emitStatementIr(ifStatement.alternative.get());
                } else {
                    irNodes.addNew(IrNode::Label(elseLabelDefinition), statement->location);
                }
                irNodes.addNew(IrNode::Label(endLabelDefinition), statement->location);
                break;
            }
            case StatementKind::In: {
                const auto& inStatement = statement->in;
                bankStack.push_back(currentBank);

                const auto result = handleInStatement(inStatement.pieces, inStatement.dest.get(), statement->location);
                if (result.first) {
                    irNodes.addNew(IrNode::PushRelocation(currentBank, result.second), statement->location);
                    emitStatementIr(inStatement.body.get());
                    irNodes.addNew(IrNode::PopRelocation(), statement->location);
                }

                currentBank = bankStack.back();
                bankStack.pop_back();
                break;
            }
            case StatementKind::InlineFor: {
                const auto& inlineForStatement = statement->inlineFor;
                if (currentBank == nullptr) {
                    report->error(statement->getDescription().toString() + " must be inside an `in` statement", statement->location);
                    break;
                }

                const auto oldContinueLabel = continueLabel;
                const auto oldBreakLabel = breakLabel;
                const auto onExit = makeScopeGuard([&]() {
                    continueLabel = oldContinueLabel;
                    breakLabel = oldBreakLabel;
                });

                auto reducedSequence = reduceExpression(inlineForStatement.sequence.get());
                if (reducedSequence == nullptr) {
                    break;
                }

                const auto length = tryGetSequenceLiteralLength(reducedSequence.get());
                if (!length.hasValue()) {
                    report->error("source for array comprehension must be a valid compile-time sequence", statement->location);
                    break;
                }

                enterScope(getOrCreateStatementScope(StringView(), statement, currentScope));
                
                const auto beginLabelDefinition = createAnonymousLabelDefinition("$loop"_sv);
                const auto endLabelDefinition = createAnonymousLabelDefinition("$endloop"_sv);

                breakLabel = endLabelDefinition;

                irNodes.addNew(IrNode::Label(beginLabelDefinition), statement->location);

                for (std::size_t i = 0; i != *length; ++i) {
                    enterInlineSite(registeredInlineSites.addNew());
                    enterScope(getOrCreateStatementScope(StringView(), statement, currentScope));

                    const auto continueLabelDefinition = createAnonymousLabelDefinition("$continue"_sv);

                    continueLabel = continueLabelDefinition;
                    const auto body = inlineForStatement.body.get();

                    bool valid = reserveDefinitions(body) && resolveDefinitionTypes() && reserveStorage(body);

                    if (valid) {
                        auto tempDeclaration = statementPool.addNew(Statement::InternalDeclaration(), statement->location);
                        auto tempDefinition = currentScope->createDefinition(report, Definition::Let({}, nullptr), inlineForStatement.name, tempDeclaration);
                        auto& tempLetDefinition = tempDefinition->variant.get<Definition::Let>();

                        auto sourceItem = getSequenceLiteralItem(reducedSequence.get(), i);
                        tempLetDefinition.expression = sourceItem.get();

                        valid = emitStatementIr(body);
                    }

                    irNodes.addNew(IrNode::Label(continueLabelDefinition), statement->location);

                    exitScope();
                    exitInlineSite();

                    if (!valid) {
                        break;
                    }
                }

                irNodes.addNew(IrNode::Label(endLabelDefinition), statement->location);

                exitScope();
                break;
            }
            case StatementKind::ImportReference: break;
            case StatementKind::InternalDeclaration: break;
            case StatementKind::Branch: {
                const auto& branch = statement->branch;
                if (currentBank == nullptr) {
                    report->error(statement->getDescription().toString() + " must be inside an `in` statement", statement->location);
                    break;
                }

                const Expression* reducedDestination = nullptr;
                if (branch.destination) {
                    reducedDestination = expressionPool.add(reduceExpression(branch.destination.get()));
                    if (!reducedDestination) {
                        break;
                    }
                }

                const Expression* reducedReturnValue = nullptr;
                if (branch.returnValue) {
                    reducedReturnValue = expressionPool.add(reduceExpression(branch.returnValue.get()));
                    if (!reducedReturnValue) {
                        break;
                    }
                }

                const Expression* reducedCondition = nullptr;
                if (branch.condition) {
                    reducedCondition = expressionPool.add(reduceExpression(branch.condition.get()));
                    if (!reducedCondition) {
                        break;
                    }
                }            

                if (!emitBranchIr(branch.distanceHint, branch.kind, reducedDestination, reducedReturnValue, false, reducedCondition, statement->location)) {
                    report->error("branch instruction could not be generated", statement->location);
                }
                break;
            }
            case StatementKind::Label: {
                const auto& labelDeclaration = statement->label;
                if (currentBank == nullptr) {
                    report->error(statement->getDescription().toString() + " must be inside an `in` statement", statement->location);
                    break;
                }

                irNodes.addNew(IrNode::Label(currentScope->findLocalMemberDefinition(labelDeclaration.name)), statement->location);
                break;
            }
            case StatementKind::Let: break;
            case StatementKind::Namespace: {
                const auto& namespaceDeclaration = statement->namespace_;
                enterScope(findStatementScope(namespaceDeclaration.body.get()));
                emitStatementIr(namespaceDeclaration.body.get());
                exitScope();
                break;
            }
            case StatementKind::Struct: break;
            case StatementKind::TypeAlias: break;
            case StatementKind::Var: {
                const auto& varDeclaration = statement->var;
                for (const auto& name : varDeclaration.names) {
                    auto definition = currentScope->findLocalMemberDefinition(name);
                    auto& varDefinition = definition->variant.get<Definition::Var>();

                    if ((varDefinition.qualifiers & Qualifiers::Extern) == Qualifiers::None) {
                        if (currentBank == nullptr && varDefinition.addressExpression == nullptr) {
                            report->error(statement->getDescription().toString() + " must be inside an `in` statement", statement->location);
                            break;
                        }

                        if (varDefinition.enclosingFunction == nullptr && isBankKindStored(currentBank->getKind())) {
                            irNodes.addNew(IrNode::Var(definition), statement->location);

                            for (auto& nestedConstant : varDefinition.nestedConstants) {
                                irNodes.addNew(IrNode::Var(nestedConstant), statement->location);
                            }
                        }
                    }
                }
                break;
            }
            case StatementKind::While: {
                const auto& whileStatement = statement->while_;
                if (currentBank == nullptr) {
                    report->error(statement->getDescription().toString() + " must be inside an `in` statement", statement->location);
                    break;
                }

                const auto oldContinueLabel = continueLabel;
                const auto oldBreakLabel = breakLabel;
                const auto onExit = makeScopeGuard([&]() {
                    continueLabel = oldContinueLabel;
                    breakLabel = oldBreakLabel;
                });

                const auto reducedCondition = expressionPool.add(reduceExpression(whileStatement.condition.get()));
                if (!reducedCondition) {
                    report->error("could not generate branch instruction for " + statement->getDescription().toString(), statement->location);
                    break;
                }

                const auto beginLabelDefinition = createAnonymousLabelDefinition("$loop"_sv);
                const auto beginLabelReferenceExpression = expressionPool.add(resolveDefinitionExpression(beginLabelDefinition, {}, statement->location));
                const auto endLabelDefinition = createAnonymousLabelDefinition("$endloop"_sv);
                const auto endLabelReferenceExpression = expressionPool.add(resolveDefinitionExpression(endLabelDefinition, {}, statement->location));

                continueLabel = beginLabelDefinition;
                breakLabel = endLabelDefinition;

                irNodes.addNew(IrNode::Label(beginLabelDefinition), statement->location);
                if (!emitBranchIr(whileStatement.distanceHint, BranchKind::Goto, endLabelReferenceExpression, nullptr, true, reducedCondition, statement->location)) {
                    break;
                }
                emitStatementIr(whileStatement.body.get());
                if (!emitBranchIr(whileStatement.distanceHint, BranchKind::Goto, beginLabelReferenceExpression, nullptr, false, nullptr, statement->location)) {
                    break;
                }
                irNodes.addNew(IrNode::Label(endLabelDefinition), statement->location);
                break;
            }
            default: std::abort(); return false;
        }

        return statement == program.get() ? report->validate() : report->alive();
    }

    bool Compiler::generateCode() {
        for (auto& bank : registeredBanks) {
            bank->rewind();
        }
        
        std::vector<std::vector<const InstructionOperand*>> captureLists;
        std::set<std::size_t> irNodeIndexesToRemove;

        // First pass: calculate data/instruction sizes, assign labels.
        for (std::size_t i = 0; i != irNodes.size(); ++i) {
            const auto& irNode = irNodes[i];
            switch (irNode->kind) {
                case IrNodeKind::PushRelocation: {
                    const auto& pushRelocation = irNode->pushRelocation;
                    bankStack.push_back(currentBank);
                    currentBank = pushRelocation.bank;

                    if (const auto address = pushRelocation.address.tryGet()) {
                        currentBank->absoluteSeek(report, *address, irNode->location);
                    }
                    break;
                }
                case IrNodeKind::PopRelocation: {
                    currentBank = bankStack.back();
                    bankStack.pop_back();
                    break;
                }
                case IrNodeKind::Label: {
                    const auto& label = irNode->label;
                    auto& funcDefinition = label.definition->variant.get<Definition::Func>();
                    funcDefinition.address = currentBank->getAddress();                    
                    break;
                }
                case IrNodeKind::Code: {
                    const auto& code = irNode->code;
                    const auto& instruction = code.instruction;
                    if (instruction->signature.extract(code.operandRoots, captureLists)) {
                        bool removed = false;

                        // Slight optimization: remove redundant jump if the destination label is immediately after this.
                        // (Also handle a checking multiple labels when defined with no code between)
                        // TODO: maybe move this optimization till the very end?
                        // There's some cases this can't catch, but doing this after addresses are calculated will make for a mess.
                        if (const auto branchKind = instruction->signature.type.tryGet<BranchKind>()) {
                            if (*branchKind == BranchKind::Goto) {
                                const auto& patterns = instruction->signature.operandPatterns;

                                if (patterns.size() >= 2 && patterns[1]->kind == InstructionOperandPatternKind::IntegerRange) {
                                    if (const auto resolvedIdentifier = code.operandRoots[1].expression->tryGet<Expression::ResolvedIdentifier>()) {
                                        std::size_t nextIndex = i + 1;

                                        while (nextIndex < irNodes.size()) {
                                            const auto& nextIrNode = irNodes[nextIndex];
                                            if (const auto nextLabel = nextIrNode->tryGet<IrNode::Label>()) {
                                                if (resolvedIdentifier->definition == nextLabel->definition) {
                                                    removed = true;
                                                    break;
                                                }
                                            } else {
                                                break;
                                            }

                                            nextIndex++;
                                        }
                                    }
                                }
                            }
                        }

                        if (removed) {
                            irNodeIndexesToRemove.insert(i);
                        } else {
                            const auto size = instruction->encoding->calculateSize(instruction->options, captureLists);
                            currentBank->reserveRom(report, "code"_sv, irNode.get(), irNode->location, size);
                        }
                    } else {
                        report->error("failed to extract instruction capture list during instruction selection pass", irNode->location, ReportErrorFlags::InternalError);
                    }
                    break;
                }
                case IrNodeKind::Var: {
                    const auto& var = irNode->var;
                    auto& varDefinition = var.definition->variant.get<Definition::Var>();

                    Optional<std::size_t> oldPosition;
                    if (varDefinition.addressExpression != nullptr) {
                        enterScope(var.definition->parentScope);

                        const auto address = resolveExplicitAddressExpression(varDefinition.addressExpression);
                        if (address.hasValue()) {
                            oldPosition = currentBank->getRelativePosition();
                            currentBank->absoluteSeek(report, address.get(), irNode->location);
                        } else {
                            break;
                        }

                        exitScope();
                    } else {
                        // FIXME: natural alignment requirements
                        const auto alignment = varDefinition.alignment != 0 ? varDefinition.alignment : 1;
                        if (alignment > 1) {
                            const auto unalignedAddress = currentBank->getAddress().absolutePosition.get();
                            currentBank->absoluteSeek(report, (unalignedAddress + alignment - 1) / alignment * alignment, irNode->location);
                        }
                    }
 
                    varDefinition.address = currentBank->getAddress();
 
                    if (!currentBank->reserveRom(report, "constant data"_sv, irNode.get(), irNode->location, varDefinition.storageSize.get())) {
                        break;
                    }
 
                    if (oldPosition.hasValue()) {
                        currentBank->setRelativePosition(oldPosition.get());
                    }
                    break;
                }
                default: std::abort(); return false;
            }
        }

        for (auto it = irNodeIndexesToRemove.rbegin(); it != irNodeIndexesToRemove.rend(); ++it) {
            irNodes.remove(*it);
        }

        irNodeIndexesToRemove.clear();

        if (!report->validate()) {
            return false;
        }

        for (auto& bank : registeredBanks) {
            bank->rewind();
        }

        std::vector<std::uint8_t> tempBuffer;
        std::vector<FwdUniquePtr<const Expression>> tempExpressions;
        std::vector<InstructionOperandRoot> tempOperandRoots;

        // Second pass: resolve all link-time expressions, write the instructions into the correct banks.
        for (const auto& irNode : irNodes) {
            switch (irNode->kind) {
                case IrNodeKind::PushRelocation: {
                    const auto& pushRelocation = irNode->pushRelocation;
                    bankStack.push_back(currentBank);
                    currentBank = pushRelocation.bank;

                    if (const auto address = pushRelocation.address.tryGet()) {
                        currentBank->absoluteSeek(report, *address, irNode->location);
                    }
                    break;
                }
                case IrNodeKind::PopRelocation: {
                    currentBank = bankStack.back();
                    bankStack.pop_back();
                    break;
                }
                case IrNodeKind::Label: {
                    const auto& label = irNode->label;
                    Address labelAddress;

                    auto& funcDefinition = label.definition->variant.get<Definition::Func>();
                    labelAddress = funcDefinition.address.get();

                    const auto currentBankAddress = currentBank->getAddress();
                    if (labelAddress != currentBankAddress) {
                        std::string message = "label `" + label.definition->name.toString() + "` was supposed to be at ";

                        if (labelAddress.absolutePosition.hasValue()) {
                            message += "absolute address 0x" + Int128(labelAddress.absolutePosition.get()).toString(16);
                        } else {
                            message += "relative position " + std::to_string(labelAddress.relativePosition.get());
                        }

                        message += ", but bank is at ";

                        if (currentBankAddress.absolutePosition.hasValue()) {
                            message += "absolute address 0x" + Int128(currentBankAddress.absolutePosition.get()).toString(16);
                        } else {
                            message += "relative position " + std::to_string(currentBankAddress.relativePosition.get());
                        }

                        report->error(message, irNode->location, ReportErrorFlags::InternalError);
                    }
                    break;
                }
                case IrNodeKind::Code: {
                    const auto& code = irNode->code;
                    const auto& instruction = code.instruction;

                    tempOperandRoots.clear();
                    tempExpressions.clear();

                    bool failed = false;

                    for (std::size_t i = 0; i != code.operandRoots.size() && !failed; ++i) {
                        const auto& operandRoot = code.operandRoots[i];
                        if (const auto expression = operandRoot.expression) {
                            if (auto reducedExpression = reduceExpression(expression)) {
                                if (auto operand = createOperandFromExpression(reducedExpression.get(), true)) {
                                    tempOperandRoots.push_back(InstructionOperandRoot(reducedExpression.get(), std::move(operand)));
                                    tempExpressions.push_back(std::move(reducedExpression));
                                } else {
                                    report->error("failed to create operand for reduced expresion", irNode->location, ReportErrorFlags::InternalError);
                                    failed = true;
                                    break;
                                }
                            } else {
                                failed = true;
                                break;
                            }
                        } else {
                            tempOperandRoots.push_back(InstructionOperandRoot(nullptr, operandRoot.operand->clone()));
                        }
                    }

                    if (failed) {
                        break;
                    }

                    if (instruction->signature.extract(tempOperandRoots, captureLists)) {
                        tempBuffer.clear();
                        instruction->encoding->write(report, currentBank, tempBuffer, instruction->options, captureLists, irNode->location);
                        if (!currentBank->write(report, "code"_sv, irNode.get(), irNode->location, tempBuffer)) {
                            break;
                        }
                    } else {
                        report->error("failed to extract instruction capture list during generation pass", irNode->location, ReportErrorFlags::InternalError);
                    }
                    break;
                }
                case IrNodeKind::Var: {
                    const auto& var = irNode->var;
                    auto& varDefinition = var.definition->variant.get<Definition::Var>();

                    Optional<std::size_t> oldPosition;
                    if (varDefinition.addressExpression != nullptr) {
                        oldPosition = currentBank->getRelativePosition();
                        currentBank->setRelativePosition(varDefinition.address.get().relativePosition.get());
                    } else {
                        // FIXME: natural alignment requirements
                        const auto alignment = varDefinition.alignment != 0 ? varDefinition.alignment : 1;
                        if (alignment > 1) {
                            const auto unalignedAddress = currentBank->getAddress().absolutePosition.get();
                            currentBank->absoluteSeek(report, (unalignedAddress + alignment - 1) / alignment * alignment, irNode->location);
                        }
                    }

                    FwdUniquePtr<const Expression> tempExpression;
                    const auto hasInitializer = varDefinition.initializerExpression.get() != nullptr;
                    const Expression* finalInitializerExpression = varDefinition.initializerExpression.get();
                    
                    if (hasInitializer && varDefinition.initializerExpression->info->context == EvaluationContext::LinkTime) {
                        if (auto reducedExpression = reduceExpression(varDefinition.initializerExpression.get())) {
                             tempExpression = createConvertedExpression(reducedExpression.get(), varDefinition.resolvedType);
                             finalInitializerExpression = tempExpression.get();
                        }
                    }
 
                    tempBuffer.clear();
                    tempBuffer.reserve(varDefinition.storageSize.get());

                    if (hasInitializer) {
                        if (!serializeConstantInitializer(finalInitializerExpression, tempBuffer)) {
                            report->error("constant initializer could not be resolved at compile-time", irNode->location, ReportErrorFlags::Fatal);
                            break;
                        }
                    } else {
                        tempBuffer.resize(varDefinition.storageSize.get());
                    }

                    if (!currentBank->write(report, "constant data"_sv, irNode.get(), irNode->location, tempBuffer)) {
                        break;
                    }
 
                    if (oldPosition.hasValue()) {
                        currentBank->setRelativePosition(oldPosition.get());
                    }
                    break;
                }
                default: std::abort(); return false;
            }
        }

        return report->validate();
    }
}