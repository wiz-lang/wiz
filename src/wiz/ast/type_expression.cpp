#include <wiz/ast/expression.h>
#include <wiz/ast/type_expression.h>
#include <wiz/utility/overload.h>

namespace wiz {
    template <>
    void FwdDeleter<TypeExpression>::operator()(const TypeExpression* ptr) {
        delete ptr;
    }

    FwdUniquePtr<const TypeExpression> TypeExpression::clone() const {
        return variant.visitWithUserdata(this, makeOverload(
            [](const TypeExpression* self, const Array& arrayType) {
                return makeFwdUnique<const TypeExpression>(
                    Array(
                        arrayType.elementType ? arrayType.elementType->clone() : nullptr,
                        arrayType.size ? arrayType.size->clone() : nullptr),
                    self->location);
            },
            [](const TypeExpression* self, const DesignatedStorage& dsType) {
                return makeFwdUnique<const TypeExpression>(
                    DesignatedStorage(
                        dsType.elementType ? dsType.elementType->clone() : nullptr,
                        dsType.holder ? dsType.holder->clone() : nullptr),
                    self->location);
            },
            [](const TypeExpression* self, const Function& functionType) {
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
                    self->location);
            },
            [](const TypeExpression* self, const Identifier& identifierType) {
                return makeFwdUnique<const TypeExpression>(
                    Identifier(identifierType.pieces),
                    self->location);             
            },
            [](const TypeExpression* self, const Pointer& pointerType) {
                return makeFwdUnique<const TypeExpression>(
                    Pointer(
                        pointerType.elementType ? pointerType.elementType->clone() : nullptr,
                        pointerType.qualifiers),
                    self->location);
            },
            [](const TypeExpression* self, const ResolvedIdentifier& resolvedIdentifierType) {
                return makeFwdUnique<const TypeExpression>(
                    ResolvedIdentifier(resolvedIdentifierType.definition),
                    self->location);
            },
            [](const TypeExpression* self, const Tuple& tupleType) {
                std::vector<FwdUniquePtr<const TypeExpression>> clonedElementTypes;
                clonedElementTypes.reserve(tupleType.elementTypes.size());
                for (const auto& elementType : tupleType.elementTypes) {
                    clonedElementTypes.push_back(elementType ? elementType->clone() : nullptr);
                }
                return makeFwdUnique<const TypeExpression>(
                    Tuple(std::move(clonedElementTypes)),
                    self->location);
            },
            [](const TypeExpression* self, const TypeOf& typeOf) {
                return makeFwdUnique<const TypeExpression>(
                    TypeOf(typeOf.expression->clone()),
                    self->location);
            }
        ));
    }
}