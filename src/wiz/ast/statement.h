#ifndef WIZ_AST_STATEMENT_H
#define WIZ_AST_STATEMENT_H

#include <type_traits>
#include <memory>
#include <string>
#include <vector>
#include <cstddef>

#include <wiz/ast/qualifiers.h>
#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/source_location.h>
#include <wiz/utility/macros.h>

namespace wiz {
    enum class StatementKind {
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
        While,
    };

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
                Qualifiers qualifiers,
                StringView name,
                FwdUniquePtr<const Expression> address,
                FwdUniquePtr<const TypeExpression> typeExpression,
                FwdUniquePtr<const Expression> value)
            : qualifiers(qualifiers),
            names(),
            addresses(),
            typeExpression(std::move(typeExpression)),
            value(std::move(value)) {
                names.push_back(name);
                addresses.push_back(std::move(address));
            }

            Var(
                Qualifiers qualifiers,
                const std::vector<StringView>& names,
                std::vector<FwdUniquePtr<const Expression>> addresses,
                FwdUniquePtr<const TypeExpression> typeExpression,
                FwdUniquePtr<const Expression> value)
            : qualifiers(qualifiers),
            names(names),
            addresses(std::move(addresses)),
            typeExpression(std::move(typeExpression)),
            value(std::move(value)) {}

            Qualifiers qualifiers;
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

        Statement(
            Attribution attribution,
            SourceLocation location)
        : kind(StatementKind::Attribution),
        attribution(std::move(attribution)),
        location(location) {}

        Statement(
            Bank bank,
            SourceLocation location)
        : kind(StatementKind::Bank),
        bank(std::move(bank)),
        location(location) {}

        Statement(
            Block block,
            SourceLocation location)
        : kind(StatementKind::Block),
        block(std::move(block)),
        location(location) {}

        Statement(
            Branch branch,
            SourceLocation location)
        : kind(StatementKind::Branch),
        branch(std::move(branch)),
        location(location) {}

        Statement(
            Config config,
            SourceLocation location)
        : kind(StatementKind::Config),
        config(std::move(config)),
        location(location) {}

        Statement(
            DoWhile doWhile,
            SourceLocation location)
        : kind(StatementKind::DoWhile),
        doWhile(std::move(doWhile)),
        location(location) {}

        Statement(
            Enum enum_,
            SourceLocation location)
        : kind(StatementKind::Enum),
        enum_(std::move(enum_)),
        location(location) {}

        Statement(
            ExpressionStatement expressionStatement,
            SourceLocation location)
        : kind(StatementKind::ExpressionStatement),
        expressionStatement(std::move(expressionStatement)),
        location(location) {}

        Statement(
            File file,
            SourceLocation location)
        : kind(StatementKind::File),
        file(std::move(file)),
        location(location) {}

        Statement(
            For for_,
            SourceLocation location)
        : kind(StatementKind::For),
        for_(std::move(for_)),
        location(location) {}

        Statement(
            Func func,
            SourceLocation location)
        : kind(StatementKind::Func),
        func(std::move(func)),
        location(location) {}

        Statement(
            If if_,
            SourceLocation location)
        : kind(StatementKind::If),
        if_(std::move(if_)),
        location(location) {}        

        Statement(
            In in,
            SourceLocation location)
        : kind(StatementKind::In),
        in(std::move(in)),
        location(location) {}

        Statement(
            InlineFor inlineFor,
            SourceLocation location)
        : kind(StatementKind::InlineFor),
        inlineFor(std::move(inlineFor)),
        location(location) {}

        Statement(
            ImportReference importReference,
            SourceLocation location)
        : kind(StatementKind::ImportReference),
        importReference(std::move(importReference)),
        location(location) {}

        Statement(
            InternalDeclaration internalDeclaration,
            SourceLocation location)
        : kind(StatementKind::InternalDeclaration),
        internalDeclaration(std::move(internalDeclaration)),
        location(location) {}

        Statement(
            Label label,
            SourceLocation location)
        : kind(StatementKind::Label),
        label(std::move(label)),
        location(location) {}

        Statement(
            Let let,
            SourceLocation location)
        : kind(StatementKind::Let),
        let(std::move(let)),
        location(location) {}

        Statement(
            Namespace namespace_,
            SourceLocation location)
        : kind(StatementKind::Namespace),
        namespace_(std::move(namespace_)),
        location(location) {}

        Statement(
            Struct struct_,
            SourceLocation location)
        : kind(StatementKind::Struct),
        struct_(std::move(struct_)),
        location(location) {}

        Statement(
            TypeAlias typeAlias,
            SourceLocation location)
        : kind(StatementKind::TypeAlias),
        typeAlias(std::move(typeAlias)),
        location(location) {}

        Statement(
            Var var,
            SourceLocation location)
        : kind(StatementKind::Var),
        var(std::move(var)),
        location(location) {}

        Statement(
            While while_,
            SourceLocation location)
        : kind(StatementKind::While),
        while_(std::move(while_)),
        location(location) {}        

        ~Statement();

        template <typename T> const T* tryGet() const;

        template <> WIZ_FORCE_INLINE const Attribution* tryGet<Attribution>() const {
            kind == StatementKind::Attribution ? &attribution : nullptr;
        }
        template <> WIZ_FORCE_INLINE const Bank* tryGet<Bank>() const {
            kind == StatementKind::Bank ? &bank : nullptr;
        }
        template <> WIZ_FORCE_INLINE const Block* tryGet<Block>() const {
            kind == StatementKind::Block ? &block : nullptr;
        }
        template <> WIZ_FORCE_INLINE const Branch* tryGet<Branch>() const {
            kind == StatementKind::Branch ? &branch : nullptr;
        }
        template <> WIZ_FORCE_INLINE const Config* tryGet<Config>() const {
            kind == StatementKind::Config ? &config : nullptr;
        }
        template <> WIZ_FORCE_INLINE const DoWhile* tryGet<DoWhile>() const {
            kind == StatementKind::DoWhile ? &doWhile : nullptr;
        }
        template <> WIZ_FORCE_INLINE const Enum* tryGet<Enum>() const {
            kind == StatementKind::Enum ? &enum_ : nullptr;
        }
        template <> WIZ_FORCE_INLINE const ExpressionStatement* tryGet<ExpressionStatement>() const {
            kind == StatementKind::ExpressionStatement ? &expressionStatement : nullptr;
        }
        template <> WIZ_FORCE_INLINE const File* tryGet<File>() const {
            kind == StatementKind::File ? &file : nullptr;
        }
        template <> WIZ_FORCE_INLINE const For* tryGet<For>() const {
            kind == StatementKind::For ? &for_ : nullptr;
        }
        template <> WIZ_FORCE_INLINE const Func* tryGet<Func>() const {
            kind == StatementKind::Func ? &func : nullptr;
        }
        template <> WIZ_FORCE_INLINE const If* tryGet<If>() const {
            kind == StatementKind::If ? &if_ : nullptr;
        }
        template <> WIZ_FORCE_INLINE const In* tryGet<In>() const {
            kind == StatementKind::In ? &in : nullptr;
        }
        template <> WIZ_FORCE_INLINE const InlineFor* tryGet<InlineFor>() const {
            kind == StatementKind::InlineFor ? &inlineFor : nullptr;
        }
        template <> WIZ_FORCE_INLINE const ImportReference* tryGet<ImportReference>() const {
            kind == StatementKind::ImportReference ? &importReference : nullptr;
        }
        template <> WIZ_FORCE_INLINE const InternalDeclaration* tryGet<InternalDeclaration>() const {
            kind == StatementKind::InternalDeclaration ? &internalDeclaration : nullptr;
        }
        template <> WIZ_FORCE_INLINE const Label* tryGet<Label>() const {
            kind == StatementKind::Label ? &label : nullptr;
        }
        template <> WIZ_FORCE_INLINE const Let* tryGet<Let>() const {
            kind == StatementKind::Let ? &let : nullptr;
        }
        template <> WIZ_FORCE_INLINE const Namespace* tryGet<Namespace>() const {
            kind == StatementKind::Namespace ? &namespace_ : nullptr;
        }
        template <> WIZ_FORCE_INLINE const Struct* tryGet<Struct>() const {
            kind == StatementKind::Struct ? &struct_ : nullptr;
        }
        template <> WIZ_FORCE_INLINE const TypeAlias* tryGet<TypeAlias>() const {
            kind == StatementKind::TypeAlias ? &typeAlias : nullptr;
        }
        template <> WIZ_FORCE_INLINE const Var* tryGet<Var>() const {
            kind == StatementKind::Var ? &var : nullptr;
        }
        template <> WIZ_FORCE_INLINE const While* tryGet<While>() const {
            kind == StatementKind::While ? &while_ : nullptr;
        }

        FwdUniquePtr<const Statement> clone() const;
        StringView getDescription() const;

        StatementKind kind;

        union {
            Attribution attribution;
            Bank bank;
            Block block;
            Branch branch;
            Config config;
            DoWhile doWhile;
            Enum enum_;
            ExpressionStatement expressionStatement;
            File file;
            For for_;
            Func func;
            If if_;
            In in;
            InlineFor inlineFor;
            ImportReference importReference;
            InternalDeclaration internalDeclaration;
            Label label;
            Let let;
            Namespace namespace_;
            Struct struct_;
            TypeAlias typeAlias;
            Var var;
            While while_;
        };

        SourceLocation location;
    };
}

#endif
