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
        return variant.visitWithUserdata(this, makeOverload(
            [](const Statement* self, const Attribution& attribution) {
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
                    self->location);
            },
            [](const Statement* self, const Bank& bank) {
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
                    self->location);
            },
            [](const Statement* self, const Block& block) {
                std::vector<FwdUniquePtr<const Statement>> clonedItems;
                clonedItems.reserve(block.items.size());
                for (const auto& item : block.items) {
                    clonedItems.push_back(item ? item->clone() : nullptr);
                }
                return makeFwdUnique<const Statement>(
                    Block(std::move(clonedItems)),
                    self->location);
            },
            [](const Statement* self, const Branch& branch) {
                return makeFwdUnique<const Statement>(
                    Branch(
                        branch.distanceHint,
                        branch.kind,
                        branch.destination ? branch.destination->clone() : nullptr,
                        branch.returnValue ? branch.returnValue->clone() : nullptr,
                        branch.condition ? branch.condition->clone() : nullptr),
                    self->location);
            },
            [](const Statement* self, const Config& configStatement) {
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
                    self->location);
            },
            [](const Statement* self, const DoWhile& doWhile) {
                return makeFwdUnique<const Statement>(
                    DoWhile(
                        doWhile.distanceHint,
                        doWhile.body ? doWhile.body->clone() : nullptr,
                        doWhile.condition ? doWhile.condition->clone() : nullptr),
                    self->location);
            },
            [](const Statement* self, const Enum& enumDeclaration) {
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
                    self->location);
            },
            [](const Statement* self, const ExpressionStatement& expressionStatement) {
                return makeFwdUnique<const Statement>(
                    ExpressionStatement(
                        expressionStatement.expression ? expressionStatement.expression->clone() : nullptr),
                    self->location);  
            },
            [](const Statement* self, const File& file) {
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
                    self->location);
            },
            [](const Statement* self, const For& forStatement) {
                return makeFwdUnique<const Statement>(
                    For(
                        forStatement.distanceHint,
                        forStatement.counter ? forStatement.counter->clone() : nullptr,                        
                        forStatement.sequence ? forStatement.sequence->clone() : nullptr,
                        forStatement.body ? forStatement.body->clone() : nullptr),
                    self->location);
            },
            [](const Statement* self, const Func& func) {
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
                    self->location);            
            },
            [](const Statement* self, const If& ifStatement) {
                return makeFwdUnique<const Statement>(
                    If(
                        ifStatement.distanceHint,
                        ifStatement.condition ? ifStatement.condition->clone() : nullptr,
                        ifStatement.body ? ifStatement.body->clone() : nullptr,
                        ifStatement.alternative ? ifStatement.alternative->clone() : nullptr),
                    self->location);            
            },
            [](const Statement* self, const In& in) {
                return makeFwdUnique<const Statement>(
                    In(
                        in.pieces,
                        in.dest ? in.dest->clone() : nullptr,
                        in.body ? in.body->clone() : nullptr),
                    self->location);            
            },
            [](const Statement* self, const InlineFor& inlineFor) {
                return makeFwdUnique<const Statement>(
                    InlineFor(
                        inlineFor.name,
                        inlineFor.sequence ? inlineFor.sequence->clone() : nullptr,
                        inlineFor.body ? inlineFor.body->clone() : nullptr),
                    self->location);                      
            },
            [](const Statement* self, const ImportReference& importReference) {
                return makeFwdUnique<const Statement>(
                    ImportReference(
                        importReference.originalPath,
                        importReference.expandedPath,
                        importReference.description),
                    self->location);                  
            },
            [](const Statement* self, const InternalDeclaration&) {
                return makeFwdUnique<const Statement>(
                    InternalDeclaration(),
                    self->location);  
            },
            [](const Statement* self, const Label& label) {
                return makeFwdUnique<const Statement>(
                    Label(label.far, label.name),
                    self->location);       
            },
            [](const Statement* self, const Let& let) {
                return makeFwdUnique<const Statement>(
                    Let(
                        let.name,
                        let.isFunction,
                        let.parameters,
                        let.value ? let.value->clone() : nullptr),
                    self->location);            
            },
            [](const Statement* self, const Namespace& ns) {
                return makeFwdUnique<const Statement>(
                    Namespace(
                        ns.name,
                        ns.body ? ns.body->clone() : nullptr),
                    self->location);
            },
            [](const Statement* self, const Struct& structDeclaration) {
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
                    self->location);
            },
            [](const Statement* self, const TypeAlias& alias) {
                return makeFwdUnique<const Statement>(
                    TypeAlias(
                        alias.name,
                        alias.typeExpression ? alias.typeExpression->clone() : nullptr),
                    self->location);
            },
            [](const Statement* self, const Var& var) {
                std::vector<FwdUniquePtr<const Expression>> clonedAddresses;
                clonedAddresses.reserve(var.addresses.size());
                for (const auto& address : var.addresses) {
                    clonedAddresses.push_back(address ? address->clone() : nullptr);
                }
                return makeFwdUnique<const Statement>(
                    Var(
                        var.modifiers,
                        var.names,
                        std::move(clonedAddresses),
                        var.typeExpression ? var.typeExpression->clone() : nullptr,
                        var.value ? var.value->clone() : nullptr),
                    self->location);
            },
            [](const Statement* self, const While& whileStatement) {
                return makeFwdUnique<const Statement>(
                    While(
                        whileStatement.distanceHint,
                        whileStatement.condition ? whileStatement.condition->clone() : nullptr,
                        whileStatement.body ? whileStatement.body->clone() : nullptr),
                    self->location);               
            }
        ));
    }

    StringView Statement::getDescription() const {
        return variant.visit(makeOverload(
            [](const Attribution&) { return "attributed statement"_sv; },
            [](const Bank&) { return "`bank` declaration"_sv; },
            [](const Block&) { return "block statement"_sv; },
            [](const Branch& branch) {
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
            },
            [](const Config&) { return "`config` directive"_sv; },
            [](const DoWhile&) { return "`do` ... `while` statement"_sv; },
            [](const Enum&) { return "`enum` declaration"_sv; },
            [](const ExpressionStatement&) { return "expression statement"_sv; },
            [](const File& data) { return data.description; },
            [](const For&) { return "`for` statement"_sv; },
            [](const Func&) { return "`func` statement"_sv; },
            [](const If&) { return "`if` statement"_sv; },
            [](const In&) { return "`in` statement"_sv; },
            [](const InlineFor&) { return "`inline for` statement"_sv; },
            [](const ImportReference& data) { return data.description; },
            [](const InternalDeclaration&) { return "compiler-internal declaration"_sv; },
            [](const Label&) { return "label declaration"_sv; },
            [](const Let&) { return "`let` declaration"_sv; },
            [](const Namespace&) { return "`namespace` declaration"_sv; },
            [](const Struct& structDeclaration) {
                switch (structDeclaration.kind) {
                    case StructKind::Struct: return "`struct` declaration"_sv;
                    case StructKind::Union: return "`union` declaration"_sv;
                    default: {
                        std::abort();
                        break;
                    }
                }
            },
            [](const TypeAlias&) { return "`alias` declaration"_sv; },
            [](const Var& varDeclaration) {
                if (varDeclaration.modifiers.contains<Modifier::Const>()) {
                    return "`const` declaration"_sv;
                } else if (varDeclaration.modifiers.contains<Modifier::WriteOnly>()) {
                    return "`writeonly` declaration"_sv;
                } else {
                    return "`var` declaration"_sv;
                }
            },
            [](const While&) { return "`while` statement"_sv; }
        ));
    }
}