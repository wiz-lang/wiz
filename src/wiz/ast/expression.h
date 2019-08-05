#ifndef WIZ_AST_EXPRESSION_H
#define WIZ_AST_EXPRESSION_H

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <unordered_map>

#include <wiz/utility/int128.h>
#include <wiz/utility/variant.h>
#include <wiz/utility/optional.h>
#include <wiz/utility/bit_flags.h>
#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/source_location.h>

namespace wiz {
    struct Statement;
    struct Definition;
    struct TypeExpression;

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

    enum TypeQueryKind {
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

    enum class EvaluationContext {
        CompileTime,
        LinkTime,
        RunTime,

        Count,
    };

    struct ExpressionInfo {
        enum class Flag {
            LValue,
            Const,
            WriteOnly,
            Far,

            Count,
        };
        using Flags = BitFlags<Flag, Flag::Count>;

        ExpressionInfo()
        : context(EvaluationContext::Count),
        type(nullptr),
        flags() {}

        ExpressionInfo(
            EvaluationContext context,
            FwdUniquePtr<const TypeExpression> type,
            Flags flags)
        : context(context),
        type(std::move(type)),
        flags(flags) {}

        ExpressionInfo clone() const;

        EvaluationContext context;
        FwdUniquePtr<const TypeExpression> type;
        Flags flags;
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

            template <typename T>
            Expression(
                T&& variant,
                SourceLocation location,
                Optional<ExpressionInfo> info)
            : variant(std::forward<T>(variant)),
            location(location),
            info(std::move(info)) {}

            FwdUniquePtr<const Expression> clone() const;
            FwdUniquePtr<const Expression> clone(SourceLocation location, Optional<ExpressionInfo> info) const;

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

            using VariantType = Variant<
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
            >;

            VariantType variant;
            SourceLocation location;
            Optional<ExpressionInfo> info;
    };
}

#endif
