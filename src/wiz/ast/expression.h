#ifndef WIZ_AST_EXPRESSION_H
#define WIZ_AST_EXPRESSION_H

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include <wiz/ast/qualifiers.h>
#include <wiz/utility/int128.h>
#include <wiz/utility/macros.h>
#include <wiz/utility/optional.h>
#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/source_location.h>

namespace wiz {
    struct Statement;
    struct Definition;
    struct TypeExpression;

    enum class ExpressionKind { 
        ArrayComprehension,
        ArrayPadLiteral,
        ArrayLiteral,
        BinaryOperator,
        BooleanLiteral,
        Call,
        Cast,
        Embed,
        FieldAccess,
        Identifier,
        IntegerLiteral,
        OffsetOf,
        RangeLiteral,
        ResolvedIdentifier,
        SideEffect,
        StringLiteral,
        StructLiteral,
        TupleLiteral,
        TypeOf,
        TypeQuery,
        UnaryOperator        
    };

    enum class BinaryOperatorKind {
        None,
        Addition,
        AdditionWithCarry,
        Assignment,
        BitIndexing,
        BitwiseAnd,
        BitwiseOr,
        BitwiseXor,
        Concatenation,
        Division,
        Equal,
        GreaterThan,
        GreaterThanOrEqual,
        Indexing,
        LeftRotate,
        LeftRotateWithCarry,                    
        LeftShift,
        LessThan,
        LessThanOrEqual,
        LogicalAnd,
        LogicalLeftShift,
        LogicalOr,
        LogicalRightShift,
        Modulo,
        Multiplication,
        NotEqual,
        RightRotate,
        RightRotateWithCarry,
        RightShift,
        Subtraction,
        SubtractionWithCarry,

        Count,
    };

    enum class UnaryOperatorKind {
        None,
        AddressOf,
        FarAddressOf,
        BitwiseNegation,
        Grouping,
        Indirection,
        LogicalNegation,
        PostDecrement,
        PostIncrement,
        PreDecrement,
        PreIncrement,
        SignedNegation,
        LowByte,
        HighByte,
        BankByte,
        AddressReserve,

        Count,
    };

    enum class TypeQueryKind {
        None,
        SizeOf,
        AlignOf,

        Count,
    };

    StringView getBinaryOperatorSymbol(BinaryOperatorKind op);
    StringView getBinaryOperatorName(BinaryOperatorKind op);
    BinaryOperatorKind getBinaryOperatorLogicalNegation(BinaryOperatorKind op);
    StringView getUnaryOperatorSymbol(UnaryOperatorKind op);
    StringView getUnaryOperatorName(UnaryOperatorKind op);
    bool isUnaryIncrementOperator(UnaryOperatorKind op);
    bool isUnaryPreIncrementOperator(UnaryOperatorKind op);
    bool isUnaryPostIncrementOperator(UnaryOperatorKind op);
    UnaryOperatorKind getUnaryPreIncrementEquivalent(UnaryOperatorKind op);

    enum class EvaluationContext {
        // Has not been determined yet.
        Unknown,
        // A constant expression that can be evaluated during compilation.
        CompileTime,
        // A constant expression that will be evaluated when program is linked. Can't be done at compile-time because it relies on unknown addresses/sizes.
        LinkTime,
        // An expression that can't be evaluated until the program executes.
        RunTime,
    };

    struct ExpressionInfo {
        ExpressionInfo()
        : context(EvaluationContext::Unknown),
        type(nullptr),
        qualifiers() {}

        ExpressionInfo(
            EvaluationContext context,
            FwdUniquePtr<const TypeExpression> type,
            Qualifiers qualifiers)
        : context(context),
        type(std::move(type)),
        qualifiers(qualifiers) {}

        ExpressionInfo clone() const;

        EvaluationContext context;
        FwdUniquePtr<const TypeExpression> type;
        Qualifiers qualifiers;
    };

    struct Expression {
        public:
            struct ArrayComprehension {
                ArrayComprehension(
                    FwdUniquePtr<const Expression> expression,
                    StringView name,
                    FwdUniquePtr<const Expression> sequence)
                : expression(std::move(expression)),
                name(name),
                sequence(std::move(sequence)) {}

                FwdUniquePtr<const Expression> expression;
                StringView name;
                FwdUniquePtr<const Expression> sequence;
            };

            struct ArrayPadLiteral {
                ArrayPadLiteral(
                    FwdUniquePtr<const Expression> valueExpression,
                    FwdUniquePtr<const Expression> sizeExpression)
                : valueExpression(std::move(valueExpression)),
                sizeExpression(std::move(sizeExpression)) {}

                FwdUniquePtr<const Expression> valueExpression;
                FwdUniquePtr<const Expression> sizeExpression;
            };

            struct ArrayLiteral {
                ArrayLiteral() {}
                ArrayLiteral(
                    std::vector<FwdUniquePtr<const Expression>> items)
                : items(std::move(items)) {}
            
                std::vector<FwdUniquePtr<const Expression>> items;
            };

            struct BinaryOperator {
                BinaryOperator(
                    BinaryOperatorKind op,
                    FwdUniquePtr<const Expression> left,
                    FwdUniquePtr<const Expression> right)
                : op(op),
                left(std::move(left)),
                right(std::move(right)) {}

                BinaryOperatorKind op;
                FwdUniquePtr<const Expression> left;
                FwdUniquePtr<const Expression> right;
            };

            struct BooleanLiteral {
                BooleanLiteral(
                    bool value)
                : value(value) {}
            
                bool value;
            };

            struct Call {
                Call(
                    bool inlined,
                    FwdUniquePtr<const Expression> function,
                    std::vector<FwdUniquePtr<const Expression>> arguments)
                : inlined(inlined),
                function(std::move(function)),
                arguments(std::move(arguments)) {}

                bool inlined;
                FwdUniquePtr<const Expression> function;
                std::vector<FwdUniquePtr<const Expression>> arguments;
            };

            struct Cast {
                Cast(
                    FwdUniquePtr<const Expression> operand,
                    FwdUniquePtr<const TypeExpression> type)
                : operand(std::move(operand)),
                type(std::move(type)) {}

                FwdUniquePtr<const Expression> operand;
                FwdUniquePtr<const TypeExpression> type;
            };

            struct Embed {
                Embed(
                    StringView originalPath)
                : originalPath(originalPath) {}

                StringView originalPath;
            };

            struct FieldAccess {
                FieldAccess(
                    FwdUniquePtr<const Expression> operand,
                    StringView field)
                : operand(std::move(operand)),
                field(field) {}

                FwdUniquePtr<const Expression> operand;
                StringView field;
            };

            struct Identifier {
                Identifier(
                    const std::vector<StringView>& pieces)
                : pieces(pieces) {}

                std::vector<StringView> pieces;
            };

            struct IntegerLiteral {
                IntegerLiteral(
                    Int128 value)
                : value(value) {}

                IntegerLiteral(
                    Int128 value,
                    StringView suffix)
                : value(value),
                suffix(suffix) {}

                Int128 value;
                StringView suffix;
            };

            struct OffsetOf {
                OffsetOf(
                    FwdUniquePtr<const TypeExpression> type,
                    StringView field)
                : type(std::move(type)),
                field(field) {}

                FwdUniquePtr<const TypeExpression> type;
                StringView field;
            };

            struct RangeLiteral {
                RangeLiteral(
                    FwdUniquePtr<const Expression> start,
                    FwdUniquePtr<const Expression> end,
                    FwdUniquePtr<const Expression> step)
                : start(std::move(start)),
                end(std::move(end)),
                step(std::move(step)) {}

                FwdUniquePtr<const Expression> start;
                FwdUniquePtr<const Expression> end;
                FwdUniquePtr<const Expression> step;
            };

            struct ResolvedIdentifier {
                ResolvedIdentifier(
                    Definition* definition,
                    const std::vector<StringView>& pieces)
                : definition(definition),
                pieces(pieces) {}

                Definition* definition;
                std::vector<StringView> pieces;
            };

            struct SideEffect {
                SideEffect(
                    FwdUniquePtr<const Statement> statement,
                    FwdUniquePtr<const Expression> result)
                : statement(std::move(statement)),
                result(std::move(result)) {}

                FwdUniquePtr<const Statement> statement;
                FwdUniquePtr<const Expression> result;
            };

            struct StringLiteral {
                StringLiteral(
                    StringView value)
                : value(value) {}

                StringView value;
            };

            struct StructLiteral {
                struct Item {
                    Item(
                        FwdUniquePtr<const Expression> value,
                        SourceLocation location)
                    : value(std::move(value)),
                    location(location) {}

                    FwdUniquePtr<const Expression> value;
                    SourceLocation location;
                };

                StructLiteral(
                    FwdUniquePtr<const TypeExpression> type,
                    std::unordered_map<StringView, std::unique_ptr<const Item>> items)
                : type(std::move(type)),
                items(std::move(items)) {}

                FwdUniquePtr<const TypeExpression> type;
                std::unordered_map<StringView, std::unique_ptr<const Item>> items;
            };

            struct TupleLiteral {
                TupleLiteral(
                    std::vector<FwdUniquePtr<const Expression>> items)
                : items(std::move(items)) {}

                std::vector<FwdUniquePtr<const Expression>> items;
            };

            struct TypeOf {
                TypeOf(
                    FwdUniquePtr<const Expression> expression)
                : expression(std::move(expression)) {}

                FwdUniquePtr<const Expression> expression;
            };

            struct TypeQuery {
                TypeQuery(
                    TypeQueryKind kind,
                    FwdUniquePtr<const TypeExpression> type)
                : kind(kind),
                type(std::move(type)) {}

                TypeQueryKind kind;
                FwdUniquePtr<const TypeExpression> type;
            };

            struct UnaryOperator {
                UnaryOperator(
                    UnaryOperatorKind op,
                    FwdUniquePtr<const Expression> operand)
                : op(op),
                operand(std::move(operand)) {}

                UnaryOperatorKind op;
                FwdUniquePtr<const Expression> operand;
            };

            Expression(
                ArrayComprehension arrayComprehension,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::ArrayComprehension),
            arrayComprehension(std::move(arrayComprehension)),
            location(location),
            info(std::move(info)) {}

            Expression(
                ArrayPadLiteral arrayPadLiteral,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::ArrayPadLiteral),
            arrayPadLiteral(std::move(arrayPadLiteral)),
            location(location),
            info(std::move(info)) {}

            Expression(
                ArrayLiteral arrayLiteral,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::ArrayLiteral),
            arrayLiteral(std::move(arrayLiteral)),
            location(location),
            info(std::move(info)) {}

            Expression(
                BinaryOperator binaryOperator,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::BinaryOperator),
            binaryOperator(std::move(binaryOperator)),
            location(location),
            info(std::move(info)) {}

            Expression(
                BooleanLiteral booleanLiteral,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::BooleanLiteral),
            booleanLiteral(std::move(booleanLiteral)),
            location(location),
            info(std::move(info)) {}

            Expression(
                Call call,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::Call),
            call(std::move(call)),
            location(location),
            info(std::move(info)) {}

            Expression(
                Cast cast,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::Cast),
            cast(std::move(cast)),
            location(location),
            info(std::move(info)) {}

            Expression(
                Embed embed,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::Embed),
            embed(std::move(embed)),
            location(location),
            info(std::move(info)) {}

            Expression(
                FieldAccess fieldAccess,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::FieldAccess),
            fieldAccess(std::move(fieldAccess)),
            location(location),
            info(std::move(info)) {}

            Expression(
                Identifier identifier,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::Identifier),
            identifier(std::move(identifier)),
            location(location),
            info(std::move(info)) {}

            Expression(
                IntegerLiteral integerLiteral,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::IntegerLiteral),
            integerLiteral(std::move(integerLiteral)),
            location(location),
            info(std::move(info)) {}

            Expression(
                OffsetOf offsetOf,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::OffsetOf),
            offsetOf(std::move(offsetOf)),
            location(location),
            info(std::move(info)) {}

            Expression(
                RangeLiteral rangeLiteral,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::RangeLiteral),
            rangeLiteral(std::move(rangeLiteral)),
            location(location),
            info(std::move(info)) {}

            Expression(
                ResolvedIdentifier resolvedIdentifier,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::ResolvedIdentifier),
            resolvedIdentifier(std::move(resolvedIdentifier)),
            location(location),
            info(std::move(info)) {}

            Expression(
                SideEffect sideEffect,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::SideEffect),
            sideEffect(std::move(sideEffect)),
            location(location),
            info(std::move(info)) {}

            Expression(
                StringLiteral stringLiteral,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::StringLiteral),
            stringLiteral(std::move(stringLiteral)),
            location(location),
            info(std::move(info)) {}

            Expression(
                StructLiteral structLiteral,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::StructLiteral),
            structLiteral(std::move(structLiteral)),
            location(location),
            info(std::move(info)) {}

            Expression(
                TupleLiteral tupleLiteral,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::TupleLiteral),
            tupleLiteral(std::move(tupleLiteral)),
            location(location),
            info(std::move(info)) {}

            Expression(
                TypeOf typeOf,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::TypeOf),
            typeOf(std::move(typeOf)),
            location(location),
            info(std::move(info)) {}

            Expression(
                TypeQuery typeQuery,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::TypeQuery),
            typeQuery(std::move(typeQuery)),
            location(location),
            info(std::move(info)) {}

            Expression(
                UnaryOperator unaryOperator,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : kind(ExpressionKind::UnaryOperator),
            unaryOperator(std::move(unaryOperator)),
            location(location),
            info(std::move(info)) {}

            ~Expression();

            template <typename T> const T* tryGet() const;

            FwdUniquePtr<const Expression> clone() const;
            FwdUniquePtr<const Expression> clone(SourceLocation location, Optional<ExpressionInfo> info) const;

            ExpressionKind kind;
            union {
                ArrayComprehension arrayComprehension;
                ArrayPadLiteral arrayPadLiteral;
                ArrayLiteral arrayLiteral;
                BinaryOperator binaryOperator;
                BooleanLiteral booleanLiteral;
                Call call;
                Cast cast;
                Embed embed;
                FieldAccess fieldAccess;
                Identifier identifier;
                IntegerLiteral integerLiteral;
                OffsetOf offsetOf;
                RangeLiteral rangeLiteral;
                ResolvedIdentifier resolvedIdentifier;
                SideEffect sideEffect;
                StringLiteral stringLiteral;
                StructLiteral structLiteral;
                TupleLiteral tupleLiteral;
                TypeOf typeOf;
                TypeQuery typeQuery;
                UnaryOperator unaryOperator;
            };

            SourceLocation location;
            Optional<ExpressionInfo> info;
    };

    template <> WIZ_FORCE_INLINE const Expression::ArrayComprehension* Expression::tryGet<Expression::ArrayComprehension>() const {
        return kind == ExpressionKind::ArrayComprehension ? &arrayComprehension : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::ArrayPadLiteral* Expression::tryGet<Expression::ArrayPadLiteral>() const {
        return kind == ExpressionKind::ArrayPadLiteral ? &arrayPadLiteral : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::ArrayLiteral* Expression::tryGet<Expression::ArrayLiteral>() const {
        return kind == ExpressionKind::ArrayLiteral ? &arrayLiteral : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::BinaryOperator* Expression::tryGet<Expression::BinaryOperator>() const {
        return kind == ExpressionKind::BinaryOperator ? &binaryOperator : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::BooleanLiteral* Expression::tryGet<Expression::BooleanLiteral>() const {
        return kind == ExpressionKind::BooleanLiteral ? &booleanLiteral : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::Call* Expression::tryGet<Expression::Call>() const {
        return kind == ExpressionKind::Call ? &call : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::Cast* Expression::tryGet<Expression::Cast>() const {
        return kind == ExpressionKind::Cast ? &cast : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::Embed* Expression::tryGet<Expression::Embed>() const {
        return kind == ExpressionKind::Embed ? &embed : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::FieldAccess* Expression::tryGet<Expression::FieldAccess>() const {
        return kind == ExpressionKind::FieldAccess ? &fieldAccess : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::Identifier* Expression::tryGet<Expression::Identifier>() const {
        return kind == ExpressionKind::Identifier ? &identifier : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::IntegerLiteral* Expression::tryGet<Expression::IntegerLiteral>() const {
        return kind == ExpressionKind::IntegerLiteral ? &integerLiteral : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::OffsetOf* Expression::tryGet<Expression::OffsetOf>() const {
        return kind == ExpressionKind::OffsetOf ? &offsetOf : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::RangeLiteral* Expression::tryGet<Expression::RangeLiteral>() const {
        return kind == ExpressionKind::RangeLiteral ? &rangeLiteral : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::ResolvedIdentifier* Expression::tryGet<Expression::ResolvedIdentifier>() const {
        return kind == ExpressionKind::ResolvedIdentifier ? &resolvedIdentifier : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::SideEffect* Expression::tryGet<Expression::SideEffect>() const {
        return kind == ExpressionKind::SideEffect ? &sideEffect : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::StringLiteral* Expression::tryGet<Expression::StringLiteral>() const {
        return kind == ExpressionKind::StringLiteral ? &stringLiteral : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::StructLiteral* Expression::tryGet<Expression::StructLiteral>() const {
        return kind == ExpressionKind::StructLiteral ? &structLiteral : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::TupleLiteral* Expression::tryGet<Expression::TupleLiteral>() const {
        return kind == ExpressionKind::TupleLiteral ? &tupleLiteral : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::TypeOf* Expression::tryGet<Expression::TypeOf>() const {
        return kind == ExpressionKind::TypeOf ? &typeOf : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::TypeQuery* Expression::tryGet<Expression::TypeQuery>() const {
        return kind == ExpressionKind::TypeQuery ? &typeQuery : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Expression::UnaryOperator* Expression::tryGet<Expression::UnaryOperator>() const {
        return kind == ExpressionKind::UnaryOperator ? &unaryOperator : nullptr;
    }
}

#endif
