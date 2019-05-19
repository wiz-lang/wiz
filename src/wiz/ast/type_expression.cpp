#include <wiz/ast/expression.h>
#include <wiz/ast/type_expression.h>
#include <wiz/utility/overload.h>

namespace wiz {
    template <>
    void FwdDeleter<TypeExpression>::operator()(const TypeExpression* ptr) {
        delete ptr;
    }

    FwdUniquePtr<const TypeExpression> TypeExpression::clone() const {
        switch (variant.index()) {
            case VariantType::typeIndexOf<Array>(): {
                const auto& arrayType = variant.get<Array>();
                return makeFwdUnique<const TypeExpression>(
                    Array(
                        arrayType.elementType ? arrayType.elementType->clone() : nullptr,
                        arrayType.size ? arrayType.size->clone() : nullptr),
                    location);
            }
            case VariantType::typeIndexOf<DesignatedStorage>(): {
                const auto& dsType = variant.get<DesignatedStorage>();
                return makeFwdUnique<const TypeExpression>(
                    DesignatedStorage(
                        dsType.elementType ? dsType.elementType->clone() : nullptr,
                        dsType.holder ? dsType.holder->clone() : nullptr),
                    location);
            }
            case VariantType::typeIndexOf<Function>(): {
                const auto& functionType = variant.get<Function>();
                std::vector<FwdUniquePtr<const TypeExpression>> clonedParameterTypes;
                clonedParameterTypes.reserve(functionType.parameterTypes.size());
                for (const auto& parameterType : functionType.parameterTypes) {
                    clonedParameterTypes.push_back(parameterType ? parameterType->clone() : nullptr);
                }
                return makeFwdUnique<const TypeExpression>(
                    Function(
                        functionType.far,
                        std::move(clonedParameterTypes),
                        functionType.returnType ? functionType.returnType->clone() : nullptr),
                    location);
            }
            case VariantType::typeIndexOf<Identifier>(): {
                const auto& identifierType = variant.get<Identifier>();
                return makeFwdUnique<const TypeExpression>(
                    Identifier(identifierType.pieces),
                    location);             
            }
            case VariantType::typeIndexOf<Pointer>(): {
                const auto& pointerType = variant.get<Pointer>();
                return makeFwdUnique<const TypeExpression>(
                    Pointer(
                        pointerType.elementType ? pointerType.elementType->clone() : nullptr,
                        pointerType.qualifiers),
                    location);
            }
            case VariantType::typeIndexOf<ResolvedIdentifier>(): {
                const auto& resolvedIdentifierType = variant.get<ResolvedIdentifier>();
                return makeFwdUnique<const TypeExpression>(
                    ResolvedIdentifier(resolvedIdentifierType.definition),
                    location);
            }
            case VariantType::typeIndexOf<Tuple>(): {
                const auto& tupleType = variant.get<Tuple>();
                std::vector<FwdUniquePtr<const TypeExpression>> clonedElementTypes;
                clonedElementTypes.reserve(tupleType.elementTypes.size());
                for (const auto& elementType : tupleType.elementTypes) {
                    clonedElementTypes.push_back(elementType ? elementType->clone() : nullptr);
                }
                return makeFwdUnique<const TypeExpression>(
                    Tuple(std::move(clonedElementTypes)),
                    location);
            }
            case VariantType::typeIndexOf<TypeOf>(): {
                const auto& typeOf = variant.get<TypeOf>();
                return makeFwdUnique<const TypeExpression>(
                    TypeOf(typeOf.expression->clone()),
                    location);
            }
            default: std::abort(); return nullptr;
        }
    }
}