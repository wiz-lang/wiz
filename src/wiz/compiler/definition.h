#ifndef WIZ_COMPILER_DEFINITION_H
#define WIZ_COMPILER_DEFINITION_H

#include <type_traits>
#include <memory>
#include <cstddef>
#include <cstdint>

#include <wiz/ast/qualifiers.h>
#include <wiz/compiler/address.h>
#include <wiz/platform/platform.h>
#include <wiz/utility/int128.h>
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

    enum class DefinitionKind {
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
        Var,
    };

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
                Qualifiers qualifiers,
                Definition* enclosingFunction,
                const Expression* addressExpression,
                const TypeExpression* typeExpression,
                std::size_t alignment)
            : qualifiers(qualifiers),
            enclosingFunction(enclosingFunction),
            addressExpression(addressExpression),
            typeExpression(typeExpression),
            alignment(alignment) {}

            Qualifiers qualifiers;
            Definition* enclosingFunction;
            const Expression* addressExpression;
            const TypeExpression* typeExpression;
            std::size_t alignment;

            const TypeExpression* resolvedType = nullptr;
            Optional<Address> address;
            Optional<std::size_t> storageSize;
            FwdUniquePtr<const TypeExpression> reducedTypeExpression;
            FwdUniquePtr<const Expression> initializerExpression;
            std::vector<Definition*> nestedConstants;
        };

        Definition(
            BuiltinBankType builtinBankType,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::BuiltinBankType),
        builtinBankType(std::move(builtinBankType)),
        name(name),
        declaration(declaration) {}

        Definition(
            BuiltinBoolType builtinBoolType,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::BuiltinBoolType),
        builtinBoolType(std::move(builtinBoolType)),
        name(name),
        declaration(declaration) {}

        Definition(
            BuiltinIntegerType builtinIntegerType,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::BuiltinIntegerType),
        builtinIntegerType(std::move(builtinIntegerType)),
        name(name),
        declaration(declaration) {}

        Definition(
            BuiltinIntegerExpressionType builtinIntegerExpressionType,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::BuiltinIntegerExpressionType),
        builtinIntegerExpressionType(std::move(builtinIntegerExpressionType)),
        name(name),
        declaration(declaration) {}

        Definition(
            BuiltinIntrinsicType builtinIntrinsicType,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::BuiltinIntrinsicType),
        builtinIntrinsicType(std::move(builtinIntrinsicType)),
        name(name),
        declaration(declaration) {}

        Definition(
            BuiltinLetType builtinLetType,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::BuiltinLetType),
        builtinLetType(std::move(builtinLetType)),
        name(name),
        declaration(declaration) {}

        Definition(
            BuiltinLoadIntrinsic builtinLoadIntrinsic,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::BuiltinLoadIntrinsic),
        builtinLoadIntrinsic(std::move(builtinLoadIntrinsic)),
        name(name),
        declaration(declaration) {}

        Definition(
            BuiltinRangeType builtinRangeType,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::BuiltinRangeType),
        builtinRangeType(std::move(builtinRangeType)),
        name(name),
        declaration(declaration) {}

        Definition(
            BuiltinRegister builtinRegister,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::BuiltinRegister),
        builtinRegister(std::move(builtinRegister)),
        name(name),
        declaration(declaration) {}

        Definition(
            BuiltinTypeOfType builtinTypeOfType,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::BuiltinTypeOfType),
        builtinTypeOfType(std::move(builtinTypeOfType)),
        name(name),
        declaration(declaration) {}

        Definition(
            BuiltinVoidIntrinsic builtinVoidIntrinsic,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::BuiltinVoidIntrinsic),
        builtinVoidIntrinsic(std::move(builtinVoidIntrinsic)),
        name(name),
        declaration(declaration) {}

        Definition(
            Bank bank,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::Bank),
        bank(std::move(bank)),
        name(name),
        declaration(declaration) {}

        Definition(
            Enum enum_,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::Enum),
        enum_(std::move(enum_)),
        name(name),
        declaration(declaration) {}

        Definition(
            EnumMember enumMember,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::EnumMember),
        enumMember(std::move(enumMember)),
        name(name),
        declaration(declaration) {}

        Definition(
            Func func,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::Func),
        func(std::move(func)),
        name(name),
        declaration(declaration) {}

        Definition(
            Let let,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::Let),
        let(std::move(let)),
        name(name),
        declaration(declaration) {}

        Definition(
            Namespace namespace_,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::Namespace),
        namespace_(std::move(namespace_)),
        name(name),
        declaration(declaration) {}

        Definition(
            Struct struct_,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::Struct),
        struct_(std::move(struct_)),
        name(name),
        declaration(declaration) {}

        Definition(
            StructMember structMember,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::StructMember),
        structMember(std::move(structMember)),
        name(name),
        declaration(declaration) {}

        Definition(
            TypeAlias typeAlias,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::TypeAlias),
        typeAlias(std::move(typeAlias)),
        name(name),
        declaration(declaration) {}

        Definition(
            Var var,
            StringView name,
            const Statement* declaration)
        : kind (DefinitionKind::Var),
        var(std::move(var)),
        name(name),
        declaration(declaration) {}
        
        ~Definition();

        template <typename T> T* tryGet();
        template <typename T> const T* tryGet() const;

        DefinitionKind kind;
        union {
            BuiltinBankType builtinBankType;
            BuiltinBoolType builtinBoolType;
            BuiltinIntegerType builtinIntegerType;
            BuiltinIntegerExpressionType builtinIntegerExpressionType;
            BuiltinIntrinsicType builtinIntrinsicType;
            BuiltinLetType builtinLetType;
            BuiltinLoadIntrinsic builtinLoadIntrinsic;
            BuiltinRangeType builtinRangeType;
            BuiltinRegister builtinRegister;
            BuiltinTypeOfType builtinTypeOfType;
            BuiltinVoidIntrinsic builtinVoidIntrinsic;
            Bank bank;
            Enum enum_;
            EnumMember enumMember;
            Func func;
            Let let;
            Namespace namespace_;
            Struct struct_;
            StructMember structMember;
            TypeAlias typeAlias;
            Var var;
        };

        StringView name;
        const Statement* declaration;

        SymbolTable* parentScope = nullptr;
    };

    template <> WIZ_FORCE_INLINE Definition::BuiltinBankType* Definition::tryGet<Definition::BuiltinBankType>() {
        return kind == DefinitionKind::BuiltinBankType ? &builtinBankType : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::BuiltinBoolType* Definition::tryGet<Definition::BuiltinBoolType>() {
        return kind == DefinitionKind::BuiltinBoolType ? &builtinBoolType : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::BuiltinIntegerType* Definition::tryGet<Definition::BuiltinIntegerType>() {
        return kind == DefinitionKind::BuiltinIntegerType ? &builtinIntegerType : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::BuiltinIntegerExpressionType* Definition::tryGet<Definition::BuiltinIntegerExpressionType>() {
        return kind == DefinitionKind::BuiltinIntegerExpressionType ? &builtinIntegerExpressionType : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::BuiltinIntrinsicType* Definition::tryGet<Definition::BuiltinIntrinsicType>() {
        return kind == DefinitionKind::BuiltinIntrinsicType ? &builtinIntrinsicType : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::BuiltinLetType* Definition::tryGet<Definition::BuiltinLetType>() {
        return kind == DefinitionKind::BuiltinLetType ? &builtinLetType : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::BuiltinLoadIntrinsic* Definition::tryGet<Definition::BuiltinLoadIntrinsic>() {
        return kind == DefinitionKind::BuiltinLoadIntrinsic ? &builtinLoadIntrinsic : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::BuiltinRangeType* Definition::tryGet<Definition::BuiltinRangeType>() {
        return kind == DefinitionKind::BuiltinRangeType ? &builtinRangeType : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::BuiltinRegister* Definition::tryGet<Definition::BuiltinRegister>() {
        return kind == DefinitionKind::BuiltinRegister ? &builtinRegister : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::BuiltinTypeOfType* Definition::tryGet<Definition::BuiltinTypeOfType>() {
        return kind == DefinitionKind::BuiltinTypeOfType ? &builtinTypeOfType : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::BuiltinVoidIntrinsic* Definition::tryGet<Definition::BuiltinVoidIntrinsic>() {
        return kind == DefinitionKind::BuiltinVoidIntrinsic ? &builtinVoidIntrinsic : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::Bank* Definition::tryGet<Definition::Bank>() {
        return kind == DefinitionKind::Bank ? &bank : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::Enum* Definition::tryGet<Definition::Enum>() {
        return kind == DefinitionKind::Enum ? &enum_ : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::EnumMember* Definition::tryGet<Definition::EnumMember>() {
        return kind == DefinitionKind::EnumMember ? &enumMember : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::Func* Definition::tryGet<Definition::Func>() {
        return kind == DefinitionKind::Func ? &func : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::Let* Definition::tryGet<Definition::Let>() {
        return kind == DefinitionKind::Let ? &let : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::Namespace* Definition::tryGet<Definition::Namespace>() {
        return kind == DefinitionKind::Namespace ? &namespace_ : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::Struct* Definition::tryGet<Definition::Struct>() {
        return kind == DefinitionKind::Struct ? &struct_ : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::StructMember* Definition::tryGet<Definition::StructMember>() {
        return kind == DefinitionKind::StructMember ? &structMember : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::TypeAlias* Definition::tryGet<Definition::TypeAlias>() {
        return kind == DefinitionKind::TypeAlias ? &typeAlias : nullptr;
    }
    template <> WIZ_FORCE_INLINE Definition::Var* Definition::tryGet<Definition::Var>() {
        return kind == DefinitionKind::Var ? &var : nullptr;
    }


    template <> WIZ_FORCE_INLINE const Definition::BuiltinBankType* Definition::tryGet<Definition::BuiltinBankType>() const {
        return kind == DefinitionKind::BuiltinBankType ? &builtinBankType : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::BuiltinBoolType* Definition::tryGet<Definition::BuiltinBoolType>() const {
        return kind == DefinitionKind::BuiltinBoolType ? &builtinBoolType : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::BuiltinIntegerType* Definition::tryGet<Definition::BuiltinIntegerType>() const {
        return kind == DefinitionKind::BuiltinIntegerType ? &builtinIntegerType : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::BuiltinIntegerExpressionType* Definition::tryGet<Definition::BuiltinIntegerExpressionType>() const {
        return kind == DefinitionKind::BuiltinIntegerExpressionType ? &builtinIntegerExpressionType : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::BuiltinIntrinsicType* Definition::tryGet<Definition::BuiltinIntrinsicType>() const {
        return kind == DefinitionKind::BuiltinIntrinsicType ? &builtinIntrinsicType : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::BuiltinLetType* Definition::tryGet<Definition::BuiltinLetType>() const {
        return kind == DefinitionKind::BuiltinLetType ? &builtinLetType : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::BuiltinLoadIntrinsic* Definition::tryGet<Definition::BuiltinLoadIntrinsic>() const {
        return kind == DefinitionKind::BuiltinLoadIntrinsic ? &builtinLoadIntrinsic : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::BuiltinRangeType* Definition::tryGet<Definition::BuiltinRangeType>() const {
        return kind == DefinitionKind::BuiltinRangeType ? &builtinRangeType : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::BuiltinRegister* Definition::tryGet<Definition::BuiltinRegister>() const {
        return kind == DefinitionKind::BuiltinRegister ? &builtinRegister : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::BuiltinTypeOfType* Definition::tryGet<Definition::BuiltinTypeOfType>() const {
        return kind == DefinitionKind::BuiltinTypeOfType ? &builtinTypeOfType : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::BuiltinVoidIntrinsic* Definition::tryGet<Definition::BuiltinVoidIntrinsic>() const {
        return kind == DefinitionKind::BuiltinVoidIntrinsic ? &builtinVoidIntrinsic : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::Bank* Definition::tryGet<Definition::Bank>() const {
        return kind == DefinitionKind::Bank ? &bank : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::Enum* Definition::tryGet<Definition::Enum>() const {
        return kind == DefinitionKind::Enum ? &enum_ : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::EnumMember* Definition::tryGet<Definition::EnumMember>() const {
        return kind == DefinitionKind::EnumMember ? &enumMember : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::Func* Definition::tryGet<Definition::Func>() const {
        return kind == DefinitionKind::Func ? &func : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::Let* Definition::tryGet<Definition::Let>() const {
        return kind == DefinitionKind::Let ? &let : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::Namespace* Definition::tryGet<Definition::Namespace>() const {
        return kind == DefinitionKind::Namespace ? &namespace_ : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::Struct* Definition::tryGet<Definition::Struct>() const {
        return kind == DefinitionKind::Struct ? &struct_ : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::StructMember* Definition::tryGet<Definition::StructMember>() const {
        return kind == DefinitionKind::StructMember ? &structMember : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::TypeAlias* Definition::tryGet<Definition::TypeAlias>() const {
        return kind == DefinitionKind::TypeAlias ? &typeAlias : nullptr;
    }
    template <> WIZ_FORCE_INLINE const Definition::Var* Definition::tryGet<Definition::Var>() const {
        return kind == DefinitionKind::Var ? &var : nullptr;
    }
}

#endif
