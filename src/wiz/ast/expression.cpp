#include <wiz/ast/expression.h>
#include <wiz/ast/statement.h>
#include <wiz/ast/type_expression.h>

namespace wiz {
    const char* const binaryOperatorSymbols[] = {
        "(none)",
        "+",
        "+#",
        "=",
        "$",
        "&",
        "|",
        "^",
        "~",
        "/",
        "==",
        ">",
        ">=",
        "[]",
        "<<<<",
        "<<<<#",
        "<<",
        "<",
        "<=",
        "&&",
        "<<<",
        "||",
        ">>>",
        "%",
        "*",
        "!=",
        ">>>>",
        ">>>>#",
        ">>",
        "-",
        "-#",
    };

    static_assert(sizeof(binaryOperatorSymbols) / sizeof(*binaryOperatorSymbols) == static_cast<std::size_t>(BinaryOperatorKind::Count), "`binaryOperatorSymbols` table must have an entry for every `BinaryOperatorKind`");

    const char* const binaryOperatorNames[] = {
        "(none)",
        "addition `+`",
        "addition-with-carry `+#`",
        "assignment `=`",
        "bit indexing `$`",
        "bitwise and `&`",
        "bitwise or `|`",
        "bitwise xor `^`",
        "concatenation `~`",
        "division `/`",
        "equality comparison `==`",
        "greater-than comparison `>`",
        "greater-than-or-equal comparison `>=`",
        "indexing `[]`",
        "left rotate `<<<<`",
        "left rotate-with-carry `<<<<#`",
        "arithmetic left shift `<<`",
        "less-than comparison `<`",
        "less-than-or-equal comparison `<=`",
        "logical and `&&`",
        "logical left shift `<<<`",
        "logical or `||`",
        "logical right shift `>>>`",
        "modulo `%`",
        "multiplication `*`",
        "inequality comparison `!=`",
        "right rotate `>>>>`",
        "right rotate-with-carry `>>>>#`",
        "arithmetic right shift `>>`",
        "subtraction `-`",
        "subtraction-with-carry `-#`",
    };

    static_assert(sizeof(binaryOperatorNames) / sizeof(*binaryOperatorNames) == static_cast<std::size_t>(BinaryOperatorKind::Count), "`binaryOperatorNames` table must have an entry for every `BinaryOperatorKind`");

    const char* const unaryOperatorSymbols[] = {
        "(none)",
        "&",
        "far &",
        "~",
        "()",
        "*",
        "!",
        "--",
        "++",
        "--",
        "++",
        "-",
        "<:",
        ">:",
        "#:",
        "@",
    };

    static_assert(sizeof(unaryOperatorSymbols) / sizeof(*unaryOperatorSymbols) == static_cast<std::size_t>(UnaryOperatorKind::Count), "`unaryOperatorSymbols` table must have an entry for every `UnaryOperatorKind`");

    const char* const unaryOperatorNames[] = {
        "(none)",
        "address-of `&`",
        "far address-of `far &`",
        "bitwise negation `~`",
        "grouping `()`",
        "indirection `*`",
        "logical negation `!`",
        "post-decrement `--`",
        "post-increment `++`",
        "pre-decrement `--`",
        "pre-increment `++`",
        "signed negation `-`",
        "low-byte access `<:`",
        "high-byte access `>:`",
        "bank-byte access `#:`",
        "address-reserve `@`",
    };

    static_assert(sizeof(unaryOperatorNames) / sizeof(*unaryOperatorNames) == static_cast<std::size_t>(UnaryOperatorKind::Count), "`unaryOperatorNames` table must have an entry for every `UnaryOperatorKind`");

    StringView getBinaryOperatorSymbol(BinaryOperatorKind op) {
        return StringView(binaryOperatorSymbols[static_cast<std::size_t>(op)]);
    }

    StringView getBinaryOperatorName(BinaryOperatorKind op) {
        return StringView(binaryOperatorNames[static_cast<std::size_t>(op)]);
    }

    BinaryOperatorKind getBinaryOperatorLogicalNegation(BinaryOperatorKind op) {
        switch (op) {
            case BinaryOperatorKind::Equal: return BinaryOperatorKind::NotEqual;
            case BinaryOperatorKind::NotEqual: return BinaryOperatorKind::Equal;
            case BinaryOperatorKind::LessThan: return BinaryOperatorKind::GreaterThanOrEqual;
            case BinaryOperatorKind::GreaterThan: return BinaryOperatorKind::LessThanOrEqual;
            case BinaryOperatorKind::LessThanOrEqual: return BinaryOperatorKind::GreaterThan;
            case BinaryOperatorKind::GreaterThanOrEqual: return BinaryOperatorKind::LessThan;
            default: return BinaryOperatorKind::None;
        }
    }

    StringView getUnaryOperatorSymbol(UnaryOperatorKind op) {
        return StringView(unaryOperatorSymbols[static_cast<std::size_t>(op)]);    
    }

    StringView getUnaryOperatorName(UnaryOperatorKind op) {
        return StringView(unaryOperatorNames[static_cast<std::size_t>(op)]);    
    }

    bool isUnaryIncrementOperator(UnaryOperatorKind op) {
        return isUnaryPreIncrementOperator(op) || isUnaryPostIncrementOperator(op);
    }

    bool isUnaryPreIncrementOperator(UnaryOperatorKind op) {
        return op == UnaryOperatorKind::PreIncrement || op == UnaryOperatorKind::PreDecrement;
    }

    bool isUnaryPostIncrementOperator(UnaryOperatorKind op) {
        return op == UnaryOperatorKind::PostIncrement || op == UnaryOperatorKind::PostDecrement;
    }

    UnaryOperatorKind getUnaryPreIncrementEquivalent(UnaryOperatorKind op) {
        switch (op) {
            case UnaryOperatorKind::PreIncrement:
            case UnaryOperatorKind::PostIncrement:
                return UnaryOperatorKind::PreIncrement;
            case UnaryOperatorKind::PreDecrement:
            case UnaryOperatorKind::PostDecrement:
                return UnaryOperatorKind::PreDecrement;
            default:
                return UnaryOperatorKind::None;
        }
    }

    ExpressionInfo ExpressionInfo::clone() const {
        return ExpressionInfo(context, type->clone(), qualifiers);
    }

    template <>
    void FwdDeleter<Expression>::operator()(const Expression* ptr) {
        delete ptr;
    }

    Expression::~Expression() {
        switch (kind) {
            case ExpressionKind::ArrayComprehension: arrayComprehension.~ArrayComprehension(); break;
            case ExpressionKind::ArrayPadLiteral: arrayPadLiteral.~ArrayPadLiteral(); break;
            case ExpressionKind::ArrayLiteral: arrayLiteral.~ArrayLiteral(); break;
            case ExpressionKind::BinaryOperator: binaryOperator.~BinaryOperator(); break;
            case ExpressionKind::BooleanLiteral: booleanLiteral.~BooleanLiteral(); break;
            case ExpressionKind::Call: call.~Call(); break;
            case ExpressionKind::Cast: cast.~Cast(); break;
            case ExpressionKind::Embed: embed.~Embed(); break;
            case ExpressionKind::FieldAccess: fieldAccess.~FieldAccess(); break;
            case ExpressionKind::Identifier: identifier.~Identifier(); break;
            case ExpressionKind::IntegerLiteral: integerLiteral.~IntegerLiteral(); break;
            case ExpressionKind::OffsetOf: offsetOf.~OffsetOf(); break;
            case ExpressionKind::RangeLiteral: rangeLiteral.~RangeLiteral(); break;
            case ExpressionKind::ResolvedIdentifier: resolvedIdentifier.~ResolvedIdentifier(); break;
            case ExpressionKind::SideEffect: sideEffect.~SideEffect(); break;
            case ExpressionKind::StringLiteral: stringLiteral.~StringLiteral(); break;
            case ExpressionKind::StructLiteral: structLiteral.~StructLiteral(); break;
            case ExpressionKind::TupleLiteral: tupleLiteral.~TupleLiteral(); break;
            case ExpressionKind::TypeOf: typeOf.~TypeOf(); break;
            case ExpressionKind::TypeQuery: typeQuery.~TypeQuery(); break;
            case ExpressionKind::UnaryOperator: unaryOperator.~UnaryOperator(); break;
            default: std::abort(); break;
        }
    }

    FwdUniquePtr<const Expression> Expression::clone() const {
        return clone(location, info ? info->clone() : Optional<ExpressionInfo>());
    }

    FwdUniquePtr<const Expression> Expression::clone(SourceLocation location, Optional<ExpressionInfo> info) const {
        switch (kind) {
            case ExpressionKind::ArrayComprehension: {
                return makeFwdUnique<const Expression>(
                    ArrayComprehension(
                        arrayComprehension.expression ? arrayComprehension.expression->clone() : nullptr,
                        arrayComprehension.name,
                        arrayComprehension.sequence ? arrayComprehension.sequence->clone() : nullptr),
                    location, std::move(info));
            }
            case ExpressionKind::ArrayPadLiteral: {
                return makeFwdUnique<const Expression>(
                    ArrayPadLiteral(
                        arrayPadLiteral.valueExpression ? arrayPadLiteral.valueExpression->clone() : nullptr,
                        arrayPadLiteral.sizeExpression ? arrayPadLiteral.sizeExpression->clone() : nullptr),
                    location, std::move(info));
            }
            case ExpressionKind::ArrayLiteral: {
                std::vector<FwdUniquePtr<const Expression>> clonedItems;
                for (const auto& item : arrayLiteral.items) {
                    clonedItems.push_back(item ? item->clone() : nullptr);
                }
                return makeFwdUnique<const Expression>(
                    ArrayLiteral(std::move(clonedItems)),
                    location, std::move(info));
            }
            case ExpressionKind::BinaryOperator: {
                return makeFwdUnique<const Expression>(
                    BinaryOperator(binaryOperator.op,
                        binaryOperator.left ? binaryOperator.left->clone() : nullptr,
                        binaryOperator.right ? binaryOperator.right->clone() : nullptr),
                    location, std::move(info));
            }
            case ExpressionKind::BooleanLiteral: {
                return makeFwdUnique<const Expression>(
                    BooleanLiteral(booleanLiteral.value),
                    location, std::move(info));
            }
            case ExpressionKind::Call: {
                std::vector<FwdUniquePtr<const Expression>> clonedArguments;
                for (const auto& argument : call.arguments) {
                    clonedArguments.push_back(argument ? argument->clone() : nullptr);
                }
                return makeFwdUnique<const Expression>(
                    Call(
                        call.inlined,
                        call.function ? call.function->clone() : nullptr,
                        std::move(clonedArguments)),
                    location, std::move(info));
            }
            case ExpressionKind::Cast: {
                return makeFwdUnique<const Expression>(
                    Cast(
                        cast.operand ? cast.operand->clone() : nullptr,
                        cast.type ? cast.type->clone() : nullptr),
                    location, std::move(info));
            }
            case ExpressionKind::Embed: {
                return makeFwdUnique<const Expression>(
                    Embed(
                        embed.originalPath),
                    location, std::move(info));
            }
            case ExpressionKind::FieldAccess: {
                return makeFwdUnique<const Expression>(
                    FieldAccess(
                        fieldAccess.operand ? fieldAccess.operand->clone() : nullptr,
                        fieldAccess.field),
                    location, std::move(info));
            }
            case ExpressionKind::Identifier: {
                return makeFwdUnique<const Expression>(
                    Identifier(identifier.pieces),
                    location, std::move(info));
            }
            case ExpressionKind::IntegerLiteral: {
                return makeFwdUnique<const Expression>(
                    IntegerLiteral(integerLiteral.value, integerLiteral.suffix),
                    location, std::move(info));
            }
            case ExpressionKind::OffsetOf: {
                return makeFwdUnique<const Expression>(
                    OffsetOf(
                        offsetOf.type ? offsetOf.type->clone() : nullptr,
                        offsetOf.field),
                    location, std::move(info));
            }
            case ExpressionKind::RangeLiteral: {
                return makeFwdUnique<const Expression>(
                    RangeLiteral(
                        rangeLiteral.start ? rangeLiteral.start->clone() : nullptr,
                        rangeLiteral.end ? rangeLiteral.end->clone() : nullptr,
                        rangeLiteral.step ? rangeLiteral.step->clone() : nullptr),
                    location, std::move(info));
            }
            case ExpressionKind::ResolvedIdentifier: {
                return makeFwdUnique<const Expression>(
                    ResolvedIdentifier(resolvedIdentifier.definition, resolvedIdentifier.pieces),
                    location, std::move(info));
            }
            case ExpressionKind::SideEffect: {
                return makeFwdUnique<const Expression>(
                    SideEffect(
                        sideEffect.statement ? sideEffect.statement->clone() : nullptr,
                        sideEffect.result ? sideEffect.result->clone() : nullptr),
                    location, std::move(info));
            }
            case ExpressionKind::StringLiteral: {
                return makeFwdUnique<const Expression>(
                    StringLiteral(stringLiteral.value),
                    location, std::move(info));
            }
            case ExpressionKind::StructLiteral: {
                std::unordered_map<StringView, std::unique_ptr<const StructLiteral::Item>> clonedItems;
                for (const auto& it : structLiteral.items) {
                    const auto& name = it.first; 
                    const auto& item = it.second;

                    clonedItems[name] = std::make_unique<const StructLiteral::Item>(
                        item->value->clone(),
                        item->location);
                }
                return makeFwdUnique<const Expression>(
                    StructLiteral(structLiteral.type->clone(), std::move(clonedItems)),
                    location, std::move(info));
            }
            case ExpressionKind::TupleLiteral: {
                std::vector<FwdUniquePtr<const Expression>> clonedItems;
                for (const auto& item : tupleLiteral.items) {
                    clonedItems.push_back(item ? item->clone() : nullptr);
                }
                return makeFwdUnique<const Expression>(
                    TupleLiteral(std::move(clonedItems)),
                    location, std::move(info));
            }
            case ExpressionKind::TypeOf: {
                return makeFwdUnique<const Expression>(
                    TypeOf(typeOf.expression->clone()),
                    location, std::move(info));
            }
            case ExpressionKind::TypeQuery: {
                return makeFwdUnique<const Expression>(
                    TypeQuery(typeQuery.kind, typeQuery.type->clone()),
                    location, std::move(info));
            }
            case ExpressionKind::UnaryOperator: {
                return makeFwdUnique<const Expression>(
                    UnaryOperator(unaryOperator.op,
                        unaryOperator.operand ? unaryOperator.operand->clone() : nullptr),
                    location, std::move(info));
            }
            default: std::abort(); break;
        }
    }
}