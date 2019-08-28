#ifndef WIZ_AST_TYPE_EXPRESION_H
#define WIZ_AST_TYPE_EXPRESION_H

#include <memory>
#include <vector>
#include <cstddef>

#include <wiz/ast/qualifiers.h>
#include <wiz/utility/variant.h>
#include <wiz/utility/bit_flags.h>
#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/source_location.h>

namespace wiz {
    struct Expression;
    struct Definition;

    struct TypeExpression {
        public:
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
                Function(
                    bool far,
                    std::vector<FwdUniquePtr<const TypeExpression>> parameterTypes,
                    FwdUniquePtr<const TypeExpression> returnType)
                : far(far),
                parameterTypes(std::move(parameterTypes)),
                returnType(std::move(returnType)) {}

                bool far;
                std::vector<FwdUniquePtr<const TypeExpression>> parameterTypes;
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

            template <typename T>
            TypeExpression(
                T&& variant,
                SourceLocation location)
            : variant(std::forward<T>(variant)),
            location(location) {}

            FwdUniquePtr<const TypeExpression> clone() const;

            using VariantType = Variant<
                Array,
                DesignatedStorage,
                Function,
                Identifier,
                Pointer,
                ResolvedIdentifier,
                Tuple,
                TypeOf
            >;

            VariantType variant;
            SourceLocation location;
    };
}
#endif
