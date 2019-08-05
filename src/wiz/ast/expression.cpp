#include <wiz/ast/expression.h>
#include <wiz/ast/statement.h>
#include <wiz/ast/type_expression.h>
#include <wiz/utility/overload.h>

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
        "`<:`",
        "`>:`",
        "`#:`",
        "`@`",
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

    ExpressionInfo ExpressionInfo::clone() const {
        return ExpressionInfo(context, type->clone(), flags);
    }

    template <>
    void FwdDeleter<Expression>::operator()(const Expression* ptr) {
        delete ptr;
    }

    FwdUniquePtr<const Expression> Expression::clone() const {
        return clone(location, info ? info->clone() : Optional<ExpressionInfo>());
    }

    FwdUniquePtr<const Expression> Expression::clone(SourceLocation location, Optional<ExpressionInfo> info) const {
        switch (variant.index()) {
            case VariantType::typeIndexOf<ArrayComprehension>(): {
                const auto& arrayComprehension = variant.get<ArrayComprehension>();
                return makeFwdUnique<const Expression>(
                    ArrayComprehension(
                        arrayComprehension.expression ? arrayComprehension.expression->clone() : nullptr,
                        arrayComprehension.name,
                        arrayComprehension.sequence ? arrayComprehension.sequence->clone() : nullptr),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<ArrayPadLiteral>(): {
                const auto& arrayPadLiteral = variant.get<ArrayPadLiteral>();
                return makeFwdUnique<const Expression>(
                    ArrayPadLiteral(
                        arrayPadLiteral.valueExpression ? arrayPadLiteral.valueExpression->clone() : nullptr,
                        arrayPadLiteral.sizeExpression ? arrayPadLiteral.sizeExpression->clone() : nullptr),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<ArrayLiteral>(): {
                const auto& arrayLiteral = variant.get<ArrayLiteral>();
                std::vector<FwdUniquePtr<const Expression>> clonedItems;
                for (const auto& item : arrayLiteral.items) {
                    clonedItems.push_back(item ? item->clone() : nullptr);
                }
                return makeFwdUnique<const Expression>(
                    ArrayLiteral(std::move(clonedItems)),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<BinaryOperator>(): {
                const auto& binaryOperator = variant.get<BinaryOperator>();
                return makeFwdUnique<const Expression>(
                    BinaryOperator(binaryOperator.op,
                        binaryOperator.left ? binaryOperator.left->clone() : nullptr,
                        binaryOperator.right ? binaryOperator.right->clone() : nullptr),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<BooleanLiteral>(): {
                const auto& booleanLiteral = variant.get<BooleanLiteral>();
                return makeFwdUnique<const Expression>(
                    BooleanLiteral(booleanLiteral.value),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<Call>(): {
                const auto& call = variant.get<Call>();
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
            case VariantType::typeIndexOf<Cast>(): {
                const auto& cast = variant.get<Cast>();
                return makeFwdUnique<const Expression>(
                    Cast(
                        cast.operand ? cast.operand->clone() : nullptr,
                        cast.type ? cast.type->clone() : nullptr),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<Embed>(): {
                const auto& embed = variant.get<Embed>();
                return makeFwdUnique<const Expression>(
                    Embed(
                        embed.originalPath),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<FieldAccess>(): {
                const auto& fieldAccess = variant.get<FieldAccess>();
                return makeFwdUnique<const Expression>(
                    FieldAccess(
                        fieldAccess.operand ? fieldAccess.operand->clone() : nullptr,
                        fieldAccess.field),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<Identifier>(): {
                const auto& identifier = variant.get<Identifier>();
                return makeFwdUnique<const Expression>(
                    Identifier(identifier.pieces),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<IntegerLiteral>(): {
                const auto& integerLiteral = variant.get<IntegerLiteral>();
                return makeFwdUnique<const Expression>(
                    IntegerLiteral(integerLiteral.value, integerLiteral.suffix),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<OffsetOf>(): {
                const auto& offsetOf = variant.get<OffsetOf>();
                return makeFwdUnique<const Expression>(
                    OffsetOf(
                        offsetOf.type ? offsetOf.type->clone() : nullptr,
                        offsetOf.field),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<RangeLiteral>(): {
                const auto& rangeLiteral = variant.get<RangeLiteral>();
                return makeFwdUnique<const Expression>(
                    RangeLiteral(
                        rangeLiteral.start ? rangeLiteral.start->clone() : nullptr,
                        rangeLiteral.end ? rangeLiteral.end->clone() : nullptr,
                        rangeLiteral.step ? rangeLiteral.step->clone() : nullptr),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<ResolvedIdentifier>(): {
                const auto& resolvedIdentifier = variant.get<ResolvedIdentifier>();
                return makeFwdUnique<const Expression>(
                    ResolvedIdentifier(resolvedIdentifier.definition, resolvedIdentifier.pieces),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<SideEffect>(): {
                const auto& sideEffect = variant.get<SideEffect>();
                return makeFwdUnique<const Expression>(
                    SideEffect(
                        sideEffect.statement ? sideEffect.statement->clone() : nullptr,
                        sideEffect.result ? sideEffect.result->clone() : nullptr),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<StringLiteral>(): {
                const auto& stringLiteral = variant.get<StringLiteral>();
                return makeFwdUnique<const Expression>(
                    StringLiteral(stringLiteral.value),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<StructLiteral>(): {
                const auto& structLiteral = variant.get<StructLiteral>();
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
            case VariantType::typeIndexOf<TupleLiteral>(): {
                const auto& tupleLiteral = variant.get<TupleLiteral>();
                std::vector<FwdUniquePtr<const Expression>> clonedItems;
                for (const auto& item : tupleLiteral.items) {
                    clonedItems.push_back(item ? item->clone() : nullptr);
                }
                return makeFwdUnique<const Expression>(
                    TupleLiteral(std::move(clonedItems)),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<TypeOf>(): {
                const auto& typeOf = variant.get<TypeOf>();
                return makeFwdUnique<const Expression>(
                    TypeOf(typeOf.expression->clone()),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<TypeQuery>(): {
                const auto& typeQuery = variant.get<TypeQuery>();
                return makeFwdUnique<const Expression>(
                    TypeQuery(typeQuery.kind, typeQuery.type->clone()),
                    location, std::move(info));
            }
            case VariantType::typeIndexOf<UnaryOperator>(): {
                const auto& unaryOperator = variant.get<UnaryOperator>();
                return makeFwdUnique<const Expression>(
                    UnaryOperator(unaryOperator.op,
                        unaryOperator.operand ? unaryOperator.operand->clone() : nullptr),
                    location, std::move(info));
            }
            default: std::abort(); break;
        }
    }
}