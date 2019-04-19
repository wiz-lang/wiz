#ifndef WIZ_COMPILER_DEFINITION_H
#define WIZ_COMPILER_DEFINITION_H

#include <type_traits>
#include <memory>
#include <cstddef>
#include <cstdint>

#include <wiz/ast/modifiers.h>
#include <wiz/compiler/address.h>
#include <wiz/platform/platform.h>
#include <wiz/utility/int128.h>
#include <wiz/utility/variant.h>
#include <wiz/utility/optional.h>
#include <wiz/utility/string_view.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    struct Statement;
    struct Expression;
    struct TypeExpression;

    class Bank;
    class SymbolTable;

    enum class BankKind;
    enum class StructKind;

    struct Definition {
        struct BuiltinBankType {
            BuiltinBankType(
                BankKind kind)
            : kind(kind) {}

            BankKind kind;
        };

        struct BuiltinBoolType {};

        struct BuiltinIntegerType {
            BuiltinIntegerType(                
                Int128 min,
                Int128 max,
                std::size_t size)
            : min(min), max(max), size(size) {}

            Int128 min;
            Int128 max;
            std::size_t size;
        };

        struct BuiltinIntegerExpressionType {};
        struct BuiltinIntrinsicType {};
        struct BuiltinLetType {};

        struct BuiltinLoadIntrinsic {
            BuiltinLoadIntrinsic(
                Definition* type)
            : type(type) {}

            Definition* type;
        };

        struct BuiltinRangeType {};

        struct BuiltinRegister {
            BuiltinRegister(
                Definition* type)
            : type(type) {}

            Definition* type;
        };

        struct BuiltinTypeOfType {};
        struct BuiltinVoidIntrinsic {};

        struct Bank {
            Bank(const Expression* addressExpr, const TypeExpression* typeExpression)
            : addressExpression(addressExpr),
            typeExpression(typeExpression),
            bank(nullptr) {}

            const Expression* addressExpression;
            const TypeExpression* typeExpression;
            wiz::Bank* bank;

            FwdUniquePtr<const TypeExpression> resolvedType;
        };

        struct Enum {
            Enum(
                const TypeExpression* underlyingTypeExpression,
                SymbolTable* environment)
            : underlyingTypeExpression(underlyingTypeExpression),
            environment(environment) {}

            const TypeExpression* underlyingTypeExpression;
            SymbolTable* environment;

            FwdUniquePtr<const TypeExpression> resolvedUnderlyingType;
            std::vector<Definition*> members;
        };

        struct EnumMember {
            EnumMember(
                const Expression* expression,
                std::size_t offset)
            : expression(expression),
            offset(offset) {}

            const Expression* expression;
            std::size_t offset;

            FwdUniquePtr<const Expression> reducedExpression;
        };

        struct Func {
            Func(bool fallthrough,
                bool inlined,
                bool far,
                BranchKind returnKind,
                const TypeExpression* returnTypeExpression,
                SymbolTable* enclosingScope,
                const Statement* body)
            : fallthrough(fallthrough),
            inlined(inlined),
            far(far),
            returnKind(returnKind),
            returnTypeExpression(returnTypeExpression),
            enclosingScope(enclosingScope),
            body(body) {}

            bool fallthrough;
            bool inlined;
            bool far;
            BranchKind returnKind;
            const TypeExpression* returnTypeExpression;
            SymbolTable* enclosingScope;
            const Statement* body;

            Optional<Address> address;
            FwdUniquePtr<const TypeExpression> resolvedSignatureType;
            
            std::vector<Definition*> parameters;
            std::vector<Definition*> locals;
            bool hasUnconditionalReturn = false;
        };

        struct Let {
            Let(
                const std::vector<StringView>& parameters,
                const Expression* expression)
            : parameters(parameters),
            expression(expression) {}

            std::vector<StringView> parameters;
            const Expression* expression;
        };

        struct Namespace {
            Namespace(
                SymbolTable* environment)
            : environment(environment) {}

            SymbolTable* environment;
        };

        struct Struct {
            Struct(
                StructKind kind,
                SymbolTable* environment)
            : kind(kind),
            environment(environment) {}

            StructKind kind;
            SymbolTable* environment;

            std::vector<Definition*> members;
            Optional<std::size_t> size;
        };

        struct StructMember {
            StructMember(const TypeExpression* typeExpression)
            : typeExpression(typeExpression) {}

            const TypeExpression* typeExpression;

            FwdUniquePtr<const TypeExpression> resolvedType;
            Optional<std::size_t> offset;
        };

        struct TypeAlias {
            TypeAlias(const TypeExpression* typeExpression)
            : typeExpression(typeExpression) {}

            const TypeExpression* typeExpression;

            FwdUniquePtr<const TypeExpression> resolvedType;
        };

        struct Var {
            Var(
                Modifiers modifiers,
                Definition* enclosingFunction,
                const Expression* addressExpression,
                const TypeExpression* typeExpression)
            : modifiers(modifiers),
            enclosingFunction(enclosingFunction),
            addressExpression(addressExpression),
            typeExpression(typeExpression) {}

            Modifiers modifiers;
            Definition* enclosingFunction;
            const Expression* addressExpression;
            const TypeExpression* typeExpression;

            const TypeExpression* resolvedType = nullptr;
            Optional<Address> address;
            Optional<std::size_t> storageSize;
            FwdUniquePtr<const TypeExpression> reducedTypeExpression;
            FwdUniquePtr<const Expression> initializerExpression;            
        };

        template <typename T>
        Definition(
            T&& variant,
            StringView name,
            const Statement* declaration)
        : variant(std::forward<T>(variant)),
        name(name),
        declaration(declaration),
        parentScope(nullptr) {}

        using VariantType = Variant<
            BuiltinBankType,
            BuiltinBoolType,
            BuiltinIntegerType,
            BuiltinIntegerExpressionType,
            BuiltinIntrinsicType,
            BuiltinLetType,
            BuiltinLoadIntrinsic,
            BuiltinRangeType,
            BuiltinRegister,
            BuiltinTypeOfType,
            BuiltinVoidIntrinsic,
            Bank,
            Enum,
            EnumMember,
            Func,
            Let,
            Namespace,
            Struct,
            StructMember,
            TypeAlias,
            Var
        >;
            
        VariantType variant;
        StringView name;
        const Statement* declaration;
        SymbolTable* parentScope;
    };
}

#endif
