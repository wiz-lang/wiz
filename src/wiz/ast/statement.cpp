#include <cassert>

#include <wiz/ast/expression.h>
#include <wiz/ast/statement.h>
#include <wiz/ast/type_expression.h>
#include <wiz/utility/overload.h>

namespace wiz {
    template<>
    void FwdDeleter<Statement>::operator()(const Statement* ptr) {
        delete ptr;
    }

    FwdUniquePtr<const Statement> Statement::clone() const {
        switch (variant.index()) {
            case VariantType::typeIndexOf<Attribution>(): {
                const auto& attribution = variant.get<Attribution>();
                std::vector<std::unique_ptr<const Attribution::Attribute>> clonedAttributes;
                clonedAttributes.reserve(attribution.attributes.size());
                for (const auto& attribute : attribution.attributes) {
                    if (attribute != nullptr) {
                        std::vector<FwdUniquePtr<const Expression>> clonedArguments;
                        clonedArguments.reserve(attribute->arguments.size());
                        for (const auto& argument : attribute->arguments) {
                            clonedArguments.push_back(argument ? argument->clone() : nullptr);
                        }
                        clonedAttributes.push_back(
                            std::make_unique<const Attribution::Attribute>(
                                attribute->name,
                                std::move(clonedArguments),
                                attribute->location));
                    }
                }
                return makeFwdUnique<const Statement>(
                    Attribution(
                        std::move(clonedAttributes),
                        attribution.body ? attribution.body->clone() : nullptr),
                    location);
            }
            case VariantType::typeIndexOf<Bank>(): {
                const auto& bank = variant.get<Bank>();
                std::vector<FwdUniquePtr<const Expression>> clonedAddresses;
                clonedAddresses.reserve(bank.addresses.size());
                for (const auto& address : bank.addresses) {
                    clonedAddresses.push_back(address ? address->clone() : nullptr);
                }
                return makeFwdUnique<const Statement>(
                    Bank(
                        bank.names,
                        std::move(clonedAddresses),
                        bank.typeExpression ? bank.typeExpression->clone() : nullptr),
                    location);
            }
            case VariantType::typeIndexOf<Block>(): {
                const auto& block = variant.get<Block>();
                std::vector<FwdUniquePtr<const Statement>> clonedItems;
                clonedItems.reserve(block.items.size());
                for (const auto& item : block.items) {
                    clonedItems.push_back(item ? item->clone() : nullptr);
                }
                return makeFwdUnique<const Statement>(
                    Block(std::move(clonedItems)),
                    location);
            }
            case VariantType::typeIndexOf<Branch>(): {
                const auto& branch = variant.get<Branch>();
                return makeFwdUnique<const Statement>(
                    Branch(
                        branch.distanceHint,
                        branch.kind,
                        branch.destination ? branch.destination->clone() : nullptr,
                        branch.returnValue ? branch.returnValue->clone() : nullptr,
                        branch.condition ? branch.condition->clone() : nullptr),
                    location);
            }
            case VariantType::typeIndexOf<Config>(): {
                const auto& configStatement = variant.get<Config>();
                std::vector<std::unique_ptr<const Config::Item>> clonedItems;
                clonedItems.reserve(configStatement.items.size());
                for (const auto& item : configStatement.items) {
                    if (item != nullptr) {
                        clonedItems.push_back(
                            std::make_unique<const Config::Item>(
                                item->name,
                                item->value->clone()));
                    }
                }
                return makeFwdUnique<const Statement>(
                    Config(std::move(clonedItems)),
                    location);
            }
            case VariantType::typeIndexOf<DoWhile>(): {
                const auto& doWhile = variant.get<DoWhile>();
                return makeFwdUnique<const Statement>(
                    DoWhile(
                        doWhile.distanceHint,
                        doWhile.body ? doWhile.body->clone() : nullptr,
                        doWhile.condition ? doWhile.condition->clone() : nullptr),
                    location);
            }
            case VariantType::typeIndexOf<Enum>(): {
                const auto& enumDeclaration = variant.get<Enum>();
                std::vector<std::unique_ptr<const Enum::Item>> clonedItems;
                clonedItems.reserve(enumDeclaration.items.size());
                for (const auto& item : enumDeclaration.items) {
                    if (item != nullptr) {
                        clonedItems.push_back(
                            std::make_unique<const Enum::Item>(
                                item->name,
                                item->value->clone(),
                                item->location));
                    }
                }
                return makeFwdUnique<const Statement>(
                    Enum(
                        enumDeclaration.name,
                        enumDeclaration.underlyingTypeExpression->clone(),
                        std::move(clonedItems)),
                    location);
            }
            case VariantType::typeIndexOf<ExpressionStatement>(): {
                const auto& expressionStatement = variant.get<ExpressionStatement>();
                return makeFwdUnique<const Statement>(
                    ExpressionStatement(
                        expressionStatement.expression ? expressionStatement.expression->clone() : nullptr),
                    location);  
            }
            case VariantType::typeIndexOf<File>(): {
                const auto& file = variant.get<File>();
                std::vector<FwdUniquePtr<const Statement>> clonedItems;
                clonedItems.reserve(file.items.size());
                for (const auto& item : file.items) {
                    clonedItems.push_back(item ? item->clone() : nullptr);
                }
                return makeFwdUnique<const Statement>(
                    File(
                        std::move(clonedItems),
                        file.originalPath,
                        file.expandedPath,
                        file.description),
                    location);
            }
            case VariantType::typeIndexOf<For>(): {
                const auto& forStatement = variant.get<For>();
                return makeFwdUnique<const Statement>(
                    For(
                        forStatement.distanceHint,
                        forStatement.counter ? forStatement.counter->clone() : nullptr,                        
                        forStatement.sequence ? forStatement.sequence->clone() : nullptr,
                        forStatement.body ? forStatement.body->clone() : nullptr),
                    location);
            }
            case VariantType::typeIndexOf<Func>(): {
                const auto& func = variant.get<Func>();
                std::vector<std::unique_ptr<const Func::Parameter>> clonedParameters;
                clonedParameters.reserve(func.parameters.size());
                for (const auto& parameter : func.parameters) {
                    if (parameter != nullptr) {
                        clonedParameters.push_back(
                            std::make_unique<const Func::Parameter>(
                                parameter->name,
                                parameter->typeExpression ? parameter->typeExpression->clone() : nullptr,
                                parameter->location));
                    }
                }
                return makeFwdUnique<const Statement>(
                    Func(
                        func.inlined,
                        func.far,
                        func.name,
                        std::move(clonedParameters),
                        func.returnTypeExpression ? func.returnTypeExpression->clone() : nullptr,
                        func.body ? func.body->clone() : nullptr),
                    location);            
            }
            case VariantType::typeIndexOf<If>(): {
                const auto& ifStatement = variant.get<If>();
                return makeFwdUnique<const Statement>(
                    If(
                        ifStatement.distanceHint,
                        ifStatement.condition ? ifStatement.condition->clone() : nullptr,
                        ifStatement.body ? ifStatement.body->clone() : nullptr,
                        ifStatement.alternative ? ifStatement.alternative->clone() : nullptr),
                    location);            
            }
            case VariantType::typeIndexOf<In>(): {
                const auto& in = variant.get<In>();
                return makeFwdUnique<const Statement>(
                    In(
                        in.pieces,
                        in.dest ? in.dest->clone() : nullptr,
                        in.body ? in.body->clone() : nullptr),
                    location);            
            }
            case VariantType::typeIndexOf<InlineFor>(): {
                const auto& inlineFor = variant.get<InlineFor>();
                return makeFwdUnique<const Statement>(
                    InlineFor(
                        inlineFor.name,
                        inlineFor.sequence ? inlineFor.sequence->clone() : nullptr,
                        inlineFor.body ? inlineFor.body->clone() : nullptr),
                    location);                      
            }
            case VariantType::typeIndexOf<ImportReference>(): {
                const auto& importReference = variant.get<ImportReference>();
                return makeFwdUnique<const Statement>(
                    ImportReference(
                        importReference.originalPath,
                        importReference.expandedPath,
                        importReference.description),
                    location);                  
            }
            case VariantType::typeIndexOf<InternalDeclaration>(): {
                return makeFwdUnique<const Statement>(
                    InternalDeclaration(),
                    location);  
            }
            case VariantType::typeIndexOf<Label>(): {
                const auto& label = variant.get<Label>();
                return makeFwdUnique<const Statement>(
                    Label(label.far, label.name),
                    location);       
            }
            case VariantType::typeIndexOf<Let>(): {
                const auto& let = variant.get<Let>();
                return makeFwdUnique<const Statement>(
                    Let(
                        let.name,
                        let.isFunction,
                        let.parameters,
                        let.value ? let.value->clone() : nullptr),
                    location);            
            }
            case VariantType::typeIndexOf<Namespace>(): {
                const auto& ns = variant.get<Namespace>();
                return makeFwdUnique<const Statement>(
                    Namespace(
                        ns.name,
                        ns.body ? ns.body->clone() : nullptr),
                    location);
            }
            case VariantType::typeIndexOf<Struct>(): {
                const auto& structDeclaration = variant.get<Struct>();
                std::vector<std::unique_ptr<const Struct::Item>> clonedItems;
                clonedItems.reserve(structDeclaration.items.size());
                for (const auto& item : structDeclaration.items) {
                    if (item != nullptr) {
                        clonedItems.push_back(
                            std::make_unique<const Struct::Item>(
                                item->name,
                                item->typeExpression->clone(),
                                item->location));
                    }
                }
                return makeFwdUnique<const Statement>(
                    Struct(
                        structDeclaration.kind,
                        structDeclaration.name,
                        std::move(clonedItems)),
                    location);
            }
            case VariantType::typeIndexOf<TypeAlias>(): {
                const auto& alias = variant.get<TypeAlias>();
                return makeFwdUnique<const Statement>(
                    TypeAlias(
                        alias.name,
                        alias.typeExpression ? alias.typeExpression->clone() : nullptr),
                    location);
            }
            case VariantType::typeIndexOf<Var>(): {
                const auto& var = variant.get<Var>();
                std::vector<FwdUniquePtr<const Expression>> clonedAddresses;
                clonedAddresses.reserve(var.addresses.size());
                for (const auto& address : var.addresses) {
                    clonedAddresses.push_back(address ? address->clone() : nullptr);
                }
                return makeFwdUnique<const Statement>(
                    Var(
                        var.qualifiers,
                        var.names,
                        std::move(clonedAddresses),
                        var.typeExpression ? var.typeExpression->clone() : nullptr,
                        var.value ? var.value->clone() : nullptr),
                    location);
            }
            case VariantType::typeIndexOf<While>(): {
                const auto& whileStatement = variant.get<While>();
                return makeFwdUnique<const Statement>(
                    While(
                        whileStatement.distanceHint,
                        whileStatement.condition ? whileStatement.condition->clone() : nullptr,
                        whileStatement.body ? whileStatement.body->clone() : nullptr),
                    location);               
            }
            default: std::abort(); return nullptr;
        }
    }

    StringView Statement::getDescription() const {
        switch (variant.index()) {
            case VariantType::typeIndexOf<Attribution>(): return "attributed statement"_sv;
            case VariantType::typeIndexOf<Bank>(): return "`bank` declaration"_sv;
            case VariantType::typeIndexOf<Block>(): return "block statement"_sv;
            case VariantType::typeIndexOf<Branch>(): {
                const auto& branch = variant.get<Branch>();
                switch (branch.kind) {
                    case BranchKind::Break: return "`break` statement"_sv;
                    case BranchKind::Continue: return "`continue` statement"_sv;
                    case BranchKind::Goto: return "`goto` statement"_sv;
                    case BranchKind::IrqReturn: return "`irqreturn` statement"_sv;
                    case BranchKind::NmiReturn: return "`nmireturn` statement"_sv;
                    case BranchKind::Return: return "`return` statement"_sv;
                    case BranchKind::Call: return "call statement"_sv;
                    case BranchKind::None:
                    default: {
                        std::abort();
                        break;
                    }
                }
            }
            case VariantType::typeIndexOf<Config>(): return "`config` directive"_sv;
            case VariantType::typeIndexOf<DoWhile>(): return "`do` ... `while` statement"_sv;
            case VariantType::typeIndexOf<Enum>(): return "`enum` declaration"_sv;
            case VariantType::typeIndexOf<ExpressionStatement>(): return "expression statement"_sv;
            case VariantType::typeIndexOf<File>(): return variant.get<File>().description;
            case VariantType::typeIndexOf<For>(): return "`for` statement"_sv;
            case VariantType::typeIndexOf<Func>(): return "`func` statement"_sv;
            case VariantType::typeIndexOf<If>(): return "`if` statement"_sv;
            case VariantType::typeIndexOf<In>(): return "`in` statement"_sv;
            case VariantType::typeIndexOf<InlineFor>(): return "`inline for` statement"_sv;
            case VariantType::typeIndexOf<ImportReference>(): return variant.get<ImportReference>().description;
            case VariantType::typeIndexOf<InternalDeclaration>(): return "compiler-internal declaration"_sv;
            case VariantType::typeIndexOf<Label>(): return "label declaration"_sv;
            case VariantType::typeIndexOf<Let>(): return "`let` declaration"_sv;
            case VariantType::typeIndexOf<Namespace>(): return "`namespace` declaration"_sv;
            case VariantType::typeIndexOf<Struct>(): {
                const auto& structDeclaration = variant.get<Struct>();
                switch (structDeclaration.kind) {
                    case StructKind::Struct: return "`struct` declaration"_sv;
                    case StructKind::Union: return "`union` declaration"_sv;
                    default: {
                        std::abort();
                        break;
                    }
                }
            }
            case VariantType::typeIndexOf<TypeAlias>(): return "`typealias` declaration"_sv;
            case VariantType::typeIndexOf<Var>(): {
                const auto& varDeclaration = variant.get<Var>();
                if (varDeclaration.qualifiers.has<Qualifier::Const>()) {
                    return "`const` declaration"_sv;
                } else if (varDeclaration.qualifiers.has<Qualifier::WriteOnly>()) {
                    return "`writeonly` declaration"_sv;
                } else {
                    return "`var` declaration"_sv;
                }
            }
            case VariantType::typeIndexOf<While>(): return "`while` statement"_sv;
            default: std::abort(); return ""_sv;
        }
    }
}