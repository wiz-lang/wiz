#include <wiz/ast/expression.h>
#include <wiz/ast/type_expression.h>

namespace wiz {
    template <>
    void FwdDeleter<TypeExpression>::operator()(const TypeExpression* ptr) {
        delete ptr;
    }

    TypeExpression::~TypeExpression() {
        switch (kind) {
            case TypeExpressionKind::Array: array.~Array(); break;
            case TypeExpressionKind::DesignatedStorage: designatedStorage.~DesignatedStorage(); break;
            case TypeExpressionKind::Function: function.~Function(); break;
            case TypeExpressionKind::Identifier: identifier.~Identifier(); break;
            case TypeExpressionKind::Pointer: pointer.~Pointer(); break;
            case TypeExpressionKind::ResolvedIdentifier: resolvedIdentifier.~ResolvedIdentifier(); break;
            case TypeExpressionKind::Tuple: tuple.~Tuple(); break;
            case TypeExpressionKind::TypeOf: typeOf.~TypeOf(); break;
            default: std::abort();
        }
    }

    FwdUniquePtr<const TypeExpression> TypeExpression::clone() const {
        switch (kind) {
            case TypeExpressionKind::Array: {
                return makeFwdUnique<const TypeExpression>(
                    Array(
                        array.elementType ? array.elementType->clone() : nullptr,
                        array.size ? array.size->clone() : nullptr),
                    location);
            }
            case TypeExpressionKind::DesignatedStorage: {
                return makeFwdUnique<const TypeExpression>(
                    DesignatedStorage(
                        designatedStorage.elementType ? designatedStorage.elementType->clone() : nullptr,
                        designatedStorage.holder ? designatedStorage.holder->clone() : nullptr),
                    location);
            }
            case TypeExpressionKind::Function: {
                std::vector<FwdUniquePtr<const TypeExpression>> clonedParameterTypes;
                clonedParameterTypes.reserve(function.parameterTypes.size());
                for (const auto& parameterType : function.parameterTypes) {
                    clonedParameterTypes.push_back(parameterType ? parameterType->clone() : nullptr);
                }
                return makeFwdUnique<const TypeExpression>(
                    Function(
                        function.far,
                        std::move(clonedParameterTypes),
                        function.returnType ? function.returnType->clone() : nullptr),
                    location);
            }
            case TypeExpressionKind::Identifier: {
                return makeFwdUnique<const TypeExpression>(
                    Identifier(identifier.pieces),
                    location);             
            }
            case TypeExpressionKind::Pointer: {
                return makeFwdUnique<const TypeExpression>(
                    Pointer(
                        pointer.elementType ? pointer.elementType->clone() : nullptr,
                        pointer.qualifiers),
                    location);
            }
            case TypeExpressionKind::ResolvedIdentifier: {
                return makeFwdUnique<const TypeExpression>(
                    ResolvedIdentifier(resolvedIdentifier.definition),
                    location);
            }
            case TypeExpressionKind::Tuple: {
                std::vector<FwdUniquePtr<const TypeExpression>> clonedElementTypes;
                clonedElementTypes.reserve(tuple.elementTypes.size());
                for (const auto& elementType : tuple.elementTypes) {
                    clonedElementTypes.push_back(elementType ? elementType->clone() : nullptr);
                }
                return makeFwdUnique<const TypeExpression>(
                    Tuple(std::move(clonedElementTypes)),
                    location);
            }
            case TypeExpressionKind::TypeOf: {
                return makeFwdUnique<const TypeExpression>(
                    TypeOf(typeOf.expression->clone()),
                    location);
            }
            default: std::abort(); return nullptr;
        }
    }
}