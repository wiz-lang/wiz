#ifndef WIZ_AST_STATEMENT_H
#define WIZ_AST_STATEMENT_H

#include <type_traits>
#include <memory>
#include <string>
#include <vector>
#include <cstddef>

#include <wiz/ast/modifiers.h>
#include <wiz/utility/variant.h>
#include <wiz/utility/bit_flags.h>
#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/source_location.h>

namespace wiz {
    enum class BranchKind {
        None,
        Break,
        Continue,
        Goto,
        IrqReturn,
        NmiReturn,
        Return,
        Call,
        FarGoto,
        FarReturn,
        FarCall,
    };

    enum class StructKind {
        Struct,
        Union,
    };

    struct Expression;
    struct TypeExpression;

    struct Statement {
        public:
            struct Attribution {
                struct Attribute {
                    Attribute(
                        StringView name,
                        std::vector<FwdUniquePtr<const Expression>> arguments,
                        SourceLocation location)
                    : name(name),
                    arguments(std::move(arguments)),
                    location(location) {}

                    StringView name;
                    std::vector<FwdUniquePtr<const Expression>> arguments;
                    SourceLocation location; 
                };

                Attribution(
                    std::vector<std::unique_ptr<const Attribute>> attributes,
                    FwdUniquePtr<const Statement> body)
                : attributes(std::move(attributes)),
                body(std::move(body)) {}

                std::vector<std::unique_ptr<const Attribute>> attributes;
                FwdUniquePtr<const Statement> body;
            };

            struct Bank {
                Bank(
                    const std::vector<StringView>& names,
                    std::vector<FwdUniquePtr<const Expression>> addresses,
                    FwdUniquePtr<const TypeExpression> typeExpression)
                : names(names),
                addresses(std::move(addresses)),
                typeExpression(std::move(typeExpression)) {}

                std::vector<StringView> names;
                std::vector<FwdUniquePtr<const Expression>> addresses;
                FwdUniquePtr<const TypeExpression> typeExpression;
            };

            struct Block {
                Block(
                    std::vector<FwdUniquePtr<const Statement>> items)
                : items(std::move(items)) {}

                std::vector<FwdUniquePtr<const Statement>> items;
            };

            struct Branch {
                Branch(
                    std::size_t distanceHint,
                    BranchKind kind,
                    FwdUniquePtr<const Expression> destination,
                    FwdUniquePtr<const Expression> returnValue,
                    FwdUniquePtr<const Expression> condition)
                : distanceHint(distanceHint),
                kind(kind),
                destination(std::move(destination)),
                returnValue(std::move(returnValue)),
                condition(std::move(condition)) {}

                std::size_t distanceHint;
                BranchKind kind;
                FwdUniquePtr<const Expression> destination;
                FwdUniquePtr<const Expression> returnValue;
                FwdUniquePtr<const Expression> condition;
            };

            struct Config {
                struct Item {
                    Item(
                        StringView name,
                        FwdUniquePtr<const Expression> value)
                    : name(name),
                    value(std::move(value)) {}

                    StringView name;
                    FwdUniquePtr<const Expression> value;
                };

                Config(
                    std::vector<std::unique_ptr<const Item>> items)
                : items(std::move(items)) {}

                std::vector<std::unique_ptr<const Item>> items;
            };

            struct DoWhile {
                DoWhile(
                    std::size_t distanceHint,
                    FwdUniquePtr<const Statement> body,
                    FwdUniquePtr<const Expression> condition)
                : distanceHint(distanceHint),
                body(std::move(body)),
                condition(std::move(condition)) {}

                std::size_t distanceHint;
                FwdUniquePtr<const Statement> body;
                FwdUniquePtr<const Expression> condition;
            };

            struct Enum {
                struct Item {
                    Item(
                        StringView name,
                        FwdUniquePtr<const Expression> value,
                        SourceLocation location)
                    : name(name),
                    value(std::move(value)),
                    location(location) {}

                    StringView name;
                    FwdUniquePtr<const Expression> value;
                    SourceLocation location;
                };

                Enum(
                    StringView name,
                    FwdUniquePtr<const TypeExpression> underlyingTypeExpression,
                    std::vector<std::unique_ptr<const Item>> items)
                : name(name),
                underlyingTypeExpression(std::move(underlyingTypeExpression)),
                items(std::move(items)) {}

                StringView name;
                FwdUniquePtr<const TypeExpression> underlyingTypeExpression;
                std::vector<std::unique_ptr<const Item>> items;
            };

            struct ExpressionStatement {
                ExpressionStatement(
                    FwdUniquePtr<const Expression> expression)
                : expression(std::move(expression)) {}

                FwdUniquePtr<const Expression> expression;
            };

            struct File {
                File(
                    std::vector<FwdUniquePtr<const Statement>> items,
                    StringView originalPath,
                    StringView canonicalPath,
                    StringView description)
                : items(std::move(items)),
                originalPath(originalPath),
                expandedPath(canonicalPath),
                description(description) {}

                std::vector<FwdUniquePtr<const Statement>> items;
                StringView originalPath;
                StringView expandedPath;
                StringView description;
            };

            struct For { 
                For(
                    std::size_t distanceHint,
                    FwdUniquePtr<const Expression> counter,
                    FwdUniquePtr<const Expression> sequence,
                    FwdUniquePtr<const Statement> body)
                : distanceHint(distanceHint),
                counter(std::move(counter)),
                sequence(std::move(sequence)),
                body(std::move(body)) {}

                std::size_t distanceHint;
                FwdUniquePtr<const Expression> counter;
                FwdUniquePtr<const Expression> sequence;
                FwdUniquePtr<const Statement> body;
            };

            struct Func {
                struct Parameter {
                    Parameter(
                        StringView name,
                        FwdUniquePtr<const TypeExpression> typeExpression,
                        const SourceLocation& location)
                    : name(name),
                    typeExpression(std::move(typeExpression)),
                    location(location) {}

                    StringView name;
                    FwdUniquePtr<const TypeExpression> typeExpression;
                    SourceLocation location;
                };

                Func(
                    bool inlined,
                    bool far,
                    StringView name,
                    std::vector<std::unique_ptr<const Parameter>> parameters,
                    FwdUniquePtr<const TypeExpression> returnTypeExpression,
                    FwdUniquePtr<const Statement> body)
                : inlined(inlined),
                far(far),
                name(name),
                parameters(std::move(parameters)),
                returnTypeExpression(std::move(returnTypeExpression)),
                body(std::move(body)) {}

                bool inlined;
                bool far;
                StringView name;
                std::vector<std::unique_ptr<const Parameter>> parameters;
                FwdUniquePtr<const TypeExpression> returnTypeExpression;
                FwdUniquePtr<const Statement> body;
            };

            struct If {
                If(
                    std::size_t distanceHint,
                    FwdUniquePtr<const Expression> condition,
                    FwdUniquePtr<const Statement> body,
                    FwdUniquePtr<const Statement> alternative)
                : distanceHint(distanceHint),
                condition(std::move(condition)),
                body(std::move(body)),
                alternative(std::move(alternative)) {}

                std::size_t distanceHint;
                FwdUniquePtr<const Expression> condition;
                FwdUniquePtr<const Statement> body;
                FwdUniquePtr<const Statement> alternative;
            };

            struct In {
                In(
                    const std::vector<StringView>& pieces,
                    FwdUniquePtr<const Expression> dest,
                    FwdUniquePtr<const Statement> body)
                : pieces(pieces),
                dest(std::move(dest)),
                body(std::move(body)) {}

                std::vector<StringView> pieces;
                FwdUniquePtr<const Expression> dest;
                FwdUniquePtr<const Statement> body;
            };    

            struct InlineFor { 
                InlineFor(
                    StringView name,
                    FwdUniquePtr<const Expression> sequence,
                    FwdUniquePtr<const Statement> body)
                : name(name),
                sequence(std::move(sequence)),
                body(std::move(body)) {}

                StringView name;
                FwdUniquePtr<const Expression> sequence;
                FwdUniquePtr<const Statement> body;
            };

            struct ImportReference {
                ImportReference(
                    StringView originalPath,
                    StringView canonicalPath,
                    StringView description)
                : originalPath(originalPath),
                expandedPath(canonicalPath),
                description(description) {}

                StringView originalPath;
                StringView expandedPath;
                StringView description;
            };

            struct InternalDeclaration {
                InternalDeclaration() {}
            };

            struct Label {
                Label(
                    bool far,
                    StringView name)
                : far(far),
                name(name) {}

                bool far;
                StringView name;
            };

            struct Let {
                Let(
                    StringView name,
                    bool isFunction,
                    const std::vector<StringView>& parameters,
                    FwdUniquePtr<const Expression> value)
                : name(name),
                isFunction(isFunction),
                parameters(parameters),
                value(std::move(value)) {}

                StringView name;
                bool isFunction;
                std::vector<StringView> parameters;
                FwdUniquePtr<const Expression> value;
            };

            struct Namespace {
                Namespace(
                    StringView name,
                    FwdUniquePtr<const Statement> body)
                : name(name),
                body(std::move(body)) {}

                StringView name;
                FwdUniquePtr<const Statement> body;
            };

            struct Struct {
                struct Item {
                    Item(
                        StringView name,
                        FwdUniquePtr<const TypeExpression> typeExpression,
                        SourceLocation location)
                    : name(name),
                    typeExpression(std::move(typeExpression)),
                    location(location) {}

                    StringView name;
                    FwdUniquePtr<const TypeExpression> typeExpression;
                    SourceLocation location;
                };

                Struct(
                    StructKind structKind,
                    StringView name,
                    std::vector<std::unique_ptr<const Item>> items)
                : kind(structKind),
                name(name),
                items(std::move(items)) {}

                StructKind kind;
                StringView name;
                std::vector<std::unique_ptr<const Item>> items;
            };

            struct TypeAlias {
                TypeAlias(
                    StringView name,
                    FwdUniquePtr<const TypeExpression> typeExpression)
                : name(name),
                typeExpression(std::move(typeExpression)) {}

                StringView name;
                FwdUniquePtr<const TypeExpression> typeExpression;
            };

            struct Var {
                Var(
                    Modifiers modifiers,
                    StringView name,
                    FwdUniquePtr<const Expression> address,
                    FwdUniquePtr<const TypeExpression> typeExpression,
                    FwdUniquePtr<const Expression> value)
                : modifiers(modifiers),
                names(),
                addresses(),
                typeExpression(std::move(typeExpression)),
                value(std::move(value)) {
                    names.push_back(name);
                    addresses.push_back(std::move(address));
                }

                Var(
                    Modifiers modifiers,
                    const std::vector<StringView>& names,
                    std::vector<FwdUniquePtr<const Expression>> addresses,
                    FwdUniquePtr<const TypeExpression> typeExpression,
                    FwdUniquePtr<const Expression> value)
                : modifiers(modifiers),
                names(names),
                addresses(std::move(addresses)),
                typeExpression(std::move(typeExpression)),
                value(std::move(value)) {}

                Modifiers modifiers;
                std::vector<StringView> names;
                std::vector<FwdUniquePtr<const Expression>> addresses;
                FwdUniquePtr<const TypeExpression> typeExpression;
                FwdUniquePtr<const Expression> value;
            };

            struct While {
                While(
                    std::size_t distanceHint,
                    FwdUniquePtr<const Expression> condition,
                    FwdUniquePtr<const Statement> body)
                : distanceHint(distanceHint),
                condition(std::move(condition)),
                body(std::move(body)) {}

                std::size_t distanceHint;
                FwdUniquePtr<const Expression> condition;
                FwdUniquePtr<const Statement> body;
            };

            template <typename T>
            Statement(
                T&& variant,
                SourceLocation location)
            : variant(std::forward<T>(variant)),
            location(location) {}

            FwdUniquePtr<const Statement> clone() const;
            StringView getDescription() const;

            using VariantType = Variant<
                Attribution,
                Bank,
                Block,
                Branch,
                Config,
                DoWhile,
                Enum,
                ExpressionStatement,
                File,
                For,
                Func,
                If,
                In,
                InlineFor,
                ImportReference,
                InternalDeclaration,
                Label,
                Let,
                Namespace,
                Struct,
                TypeAlias,
                Var,
                While
            >;

            VariantType variant;
            SourceLocation location;
    };
}

#endif
