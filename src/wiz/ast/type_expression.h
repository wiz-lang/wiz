#ifndef WIZ_AST_TYPE_EXPRESION_H
#define WIZ_AST_TYPE_EXPRESION_H

#include <memory>
#include <vector>
#include <cstddef>

#include <wiz/utility/macros.h>
#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/source_location.h>

namespace wiz {
    struct Expression;
    struct Definition;

    enum class TypeExpressionKind {
        Array,
        DesignatedStorage,
        Function,
        Identifier,
        Pointer,
        ResolvedIdentifier,
        Tuple,
        TypeOf,
    };

    struct TypeExpression {
        struct Array {
            Array(
                FwdUniquePtr<const TypeExpression> elementType,
                FwdUniquePtr<const Expression> size)
            : elementType(std::move(elementType)),
            size(std::move(size)) {}

            FwdUniquePtr<const TypeExpression> elementType;
            FwdUniquePtr<const Expression> size;
        };

        struct DesignatedStorage {
            DesignatedStorage(
                FwdUniquePtr<const TypeExpression> elementType,
                FwdUniquePtr<const Expression> holder)
            : elementType(std::move(elementType)),
            holder(std::move(holder)) {}

            FwdUniquePtr<const TypeExpression> elementType;
            FwdUniquePtr<const Expression> holder;
        };

        struct Function {
            struct Parameter {
                Parameter(
                    StringView name,
                    FwdUniquePtr<const TypeExpression> parameterType)
                : name(name),
                parameterType(std::move(parameterType)) {}

                StringView name;
                FwdUniquePtr<const TypeExpression> parameterType;
            };

            Function(
                bool far,
                std::vector<UniquePtr<const Parameter>> parameters,
                FwdUniquePtr<const TypeExpression> returnType)
            : far(far),
            parameters(std::move(parameters)),
            returnType(std::move(returnType)) {}

            bool far;
            std::vector<UniquePtr<const Parameter>> parameters;
            FwdUniquePtr<const TypeExpression> returnType;
        };

        struct Identifier {
            Identifier(
                const std::vector<StringView>& pieces)
            : pieces(pieces) {}

            std::vector<StringView> pieces;
        };

        struct Pointer {
            Pointer(
                FwdUniquePtr<const TypeExpression> elementType,
                Qualifiers qualifiers)
            : elementType(std::move(elementType)),
            qualifiers(qualifiers) {}

            FwdUniquePtr<const TypeExpression> elementType;
            Qualifiers qualifiers;
        };

        struct ResolvedIdentifier {
            ResolvedIdentifier(
                Definition* definition)
            : definition(definition),
            pieces() {}

            ResolvedIdentifier(
                Definition* definition,
                const std::vector<StringView>& pieces)
            : definition(definition),
            pieces(pieces) {}

            Definition* definition;
            std::vector<StringView> pieces;
        };

        struct Tuple {
            Tuple(
                std::vector<FwdUniquePtr<const TypeExpression>> elementTypes)
            : elementTypes(std::move(elementTypes)) {}

            std::vector<FwdUniquePtr<const TypeExpression>> elementTypes;
        };

        struct TypeOf {
            TypeOf(
                FwdUniquePtr<const Expression> expression)
            : expression(std::move(expression)) {}

            FwdUniquePtr<const Expression> expression;
        };

        TypeExpression(
            Array array,
            SourceLocation location)
        : kind(TypeExpressionKind::Array),
        array(std::move(array)),
        location(location) {}

        TypeExpression(
            DesignatedStorage designatedStorage,
            SourceLocation location)
        : kind(TypeExpressionKind::DesignatedStorage),
        designatedStorage(std::move(designatedStorage)),
        location(location) {}

        TypeExpression(
            Function function,
            SourceLocation location)
        : kind(TypeExpressionKind::Function),
        function(std::move(function)),
        location(location) {}

        TypeExpression(
            Identifier identifier,
            SourceLocation location)
        : kind(TypeExpressionKind::Identifier),
        identifier(std::move(identifier)),
        location(location) {}

        TypeExpression(
            Pointer pointer,
            SourceLocation location)
        : kind(TypeExpressionKind::Pointer),
        pointer(std::move(pointer)),
        location(location) {}

        TypeExpression(
            ResolvedIdentifier resolvedIdentifier,
            SourceLocation location)
        : kind(TypeExpressionKind::ResolvedIdentifier),
        resolvedIdentifier(std::move(resolvedIdentifier)),
        location(location) {}

        TypeExpression(
            Tuple tuple,
            SourceLocation location)
        : kind(TypeExpressionKind::Tuple),
        tuple(std::move(tuple)),
        location(location) {}

        TypeExpression(
            TypeOf typeOf,
            SourceLocation location)
        : kind(TypeExpressionKind::TypeOf),
        typeOf(std::move(typeOf)),
        location(location) {}

        ~TypeExpression();

        template <typename T> const T* tryGet() const;

        FwdUniquePtr<const TypeExpression> clone() const;

        TypeExpressionKind kind;
        union {
            Array array;
            DesignatedStorage designatedStorage;
            Function function;
            Identifier identifier;
            Pointer pointer;
            ResolvedIdentifier resolvedIdentifier;
            Tuple tuple;
            TypeOf typeOf;
        };

        SourceLocation location;
    };

    template <> WIZ_FORCE_INLINE const TypeExpression::Array* TypeExpression::tryGet<TypeExpression::Array>() const {
        return kind == TypeExpressionKind::Array ? &array : nullptr;
    }
    template <> WIZ_FORCE_INLINE const TypeExpression::DesignatedStorage* TypeExpression::tryGet<TypeExpression::DesignatedStorage>() const {
        return kind == TypeExpressionKind::DesignatedStorage ? &designatedStorage : nullptr;
    }
    template <> WIZ_FORCE_INLINE const TypeExpression::Function* TypeExpression::tryGet<TypeExpression::Function>() const {
        return kind == TypeExpressionKind::Function ? &function : nullptr;
    }
    template <> WIZ_FORCE_INLINE const TypeExpression::Identifier* TypeExpression::tryGet<TypeExpression::Identifier>() const {
        return kind == TypeExpressionKind::Identifier ? &identifier : nullptr;
    }
    template <> WIZ_FORCE_INLINE const TypeExpression::Pointer* TypeExpression::tryGet<TypeExpression::Pointer>() const {
        return kind == TypeExpressionKind::Pointer ? &pointer : nullptr;
    }
    template <> WIZ_FORCE_INLINE const TypeExpression::ResolvedIdentifier* TypeExpression::tryGet<TypeExpression::ResolvedIdentifier>() const {
        return kind == TypeExpressionKind::ResolvedIdentifier ? &resolvedIdentifier : nullptr;
    }
    template <> WIZ_FORCE_INLINE const TypeExpression::Tuple* TypeExpression::tryGet<TypeExpression::Tuple>() const {
        return kind == TypeExpressionKind::Tuple ? &tuple : nullptr;
    }
    template <> WIZ_FORCE_INLINE const TypeExpression::TypeOf* TypeExpression::tryGet<TypeExpression::TypeOf>() const {
        return kind == TypeExpressionKind::TypeOf ? &typeOf : nullptr;
    }
}
#endif
