#include <cassert>

#include <wiz/ast/expression.h>
#include <wiz/ast/statement.h>
#include <wiz/ast/type_expression.h>

namespace wiz {
    template<>
    void FwdDeleter<Statement>::operator()(const Statement* ptr) {
        delete ptr;
    }

    Statement::~Statement() {
        switch (kind) {
            case StatementKind::Attribution: attribution.~Attribution(); break;
            case StatementKind::Bank: bank.~Bank(); break;
            case StatementKind::Block: block.~Block(); break;
            case StatementKind::Branch: branch.~Branch(); break;
            case StatementKind::Config: config.~Config(); break;
            case StatementKind::DoWhile: doWhile.~DoWhile(); break;
            case StatementKind::Enum: enum_.~Enum(); break;
            case StatementKind::ExpressionStatement: expressionStatement.~ExpressionStatement(); break;
            case StatementKind::File: file.~File(); break;
            case StatementKind::For: for_.~For(); break;
            case StatementKind::Func: func.~Func(); break;
            case StatementKind::If: if_.~If(); break;
            case StatementKind::In: in.~In(); break;
            case StatementKind::InlineFor: inlineFor.~InlineFor(); break;
            case StatementKind::ImportReference: importReference.~ImportReference(); break;
            case StatementKind::InternalDeclaration: internalDeclaration.~InternalDeclaration(); break;
            case StatementKind::Label: label.~Label(); break;
            case StatementKind::Let: let.~Let(); break;
            case StatementKind::Namespace: namespace_.~Namespace(); break;
            case StatementKind::Struct: struct_.~Struct(); break;
            case StatementKind::TypeAlias: typeAlias.~TypeAlias(); break;
            case StatementKind::Var: var.~Var(); break;
            case StatementKind::While: while_.~While(); break;
            default: std::abort();
        }
    }


    FwdUniquePtr<const Statement> Statement::clone() const {
        switch (kind) {
            case StatementKind::Attribution: {
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
            case StatementKind::Bank: {
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
            case StatementKind::Block: {
                std::vector<FwdUniquePtr<const Statement>> clonedItems;
                clonedItems.reserve(block.items.size());
                for (const auto& item : block.items) {
                    clonedItems.push_back(item ? item->clone() : nullptr);
                }
                return makeFwdUnique<const Statement>(
                    Block(std::move(clonedItems)),
                    location);
            }
            case StatementKind::Branch: {
                return makeFwdUnique<const Statement>(
                    Branch(
                        branch.distanceHint,
                        branch.kind,
                        branch.destination ? branch.destination->clone() : nullptr,
                        branch.returnValue ? branch.returnValue->clone() : nullptr,
                        branch.condition ? branch.condition->clone() : nullptr),
                    location);
            }
            case StatementKind::Config: {
                std::vector<std::unique_ptr<const Config::Item>> clonedItems;
                clonedItems.reserve(config.items.size());
                for (const auto& item : config.items) {
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
            case StatementKind::DoWhile: {
                return makeFwdUnique<const Statement>(
                    DoWhile(
                        doWhile.distanceHint,
                        doWhile.body ? doWhile.body->clone() : nullptr,
                        doWhile.condition ? doWhile.condition->clone() : nullptr),
                    location);
            }
            case StatementKind::Enum: {
                std::vector<std::unique_ptr<const Enum::Item>> clonedItems;
                clonedItems.reserve(enum_.items.size());
                for (const auto& item : enum_.items) {
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
                        enum_.name,
                        enum_.underlyingTypeExpression->clone(),
                        std::move(clonedItems)),
                    location);
            }
            case StatementKind::ExpressionStatement: {
                return makeFwdUnique<const Statement>(
                    ExpressionStatement(
                        expressionStatement.expression ? expressionStatement.expression->clone() : nullptr),
                    location);  
            }
            case StatementKind::File: {
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
            case StatementKind::For: {
                return makeFwdUnique<const Statement>(
                    For(
                        for_.distanceHint,
                        for_.counter ? for_.counter->clone() : nullptr,                        
                        for_.sequence ? for_.sequence->clone() : nullptr,
                        for_.body ? for_.body->clone() : nullptr),
                    location);
            }
            case StatementKind::Func: {
                std::vector<std::unique_ptr<const Func::Parameter>> clonedParameters;
                clonedParameters.reserve(func.parameters.size());
                for (const auto& parameter : func.parameters) {
                    if (parameter != nullptr) {
                        clonedParameters.push_back(
                            std::make_unique<const Func::Parameter>(
                                parameter->kind,
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
            case StatementKind::If: {
                return makeFwdUnique<const Statement>(
                    If(
                        if_.distanceHint,
                        if_.condition ? if_.condition->clone() : nullptr,
                        if_.body ? if_.body->clone() : nullptr,
                        if_.alternative ? if_.alternative->clone() : nullptr),
                    location);            
            }
            case StatementKind::In: {
                return makeFwdUnique<const Statement>(
                    In(
                        in.pieces,
                        in.dest ? in.dest->clone() : nullptr,
                        in.body ? in.body->clone() : nullptr),
                    location);            
            }
            case StatementKind::InlineFor: {
                return makeFwdUnique<const Statement>(
                    InlineFor(
                        inlineFor.name,
                        inlineFor.sequence ? inlineFor.sequence->clone() : nullptr,
                        inlineFor.body ? inlineFor.body->clone() : nullptr),
                    location);                      
            }
            case StatementKind::ImportReference: {
                return makeFwdUnique<const Statement>(
                    ImportReference(
                        importReference.originalPath,
                        importReference.expandedPath,
                        importReference.description),
                    location);                  
            }
            case StatementKind::InternalDeclaration: {
                return makeFwdUnique<const Statement>(
                    InternalDeclaration(),
                    location);  
            }
            case StatementKind::Label: {
                return makeFwdUnique<const Statement>(
                    Label(label.far, label.name),
                    location);       
            }
            case StatementKind::Let: {
                return makeFwdUnique<const Statement>(
                    Let(
                        let.name,
                        let.isFunction,
                        let.parameters,
                        let.value ? let.value->clone() : nullptr),
                    location);            
            }
            case StatementKind::Namespace: {
                return makeFwdUnique<const Statement>(
                    Namespace(
                        namespace_.name,
                        namespace_.body ? namespace_.body->clone() : nullptr),
                    location);
            }
            case StatementKind::Struct: {
                std::vector<std::unique_ptr<const Struct::Item>> clonedItems;
                clonedItems.reserve(struct_.items.size());
                for (const auto& item : struct_.items) {
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
                        struct_.kind,
                        struct_.name,
                        std::move(clonedItems)),
                    location);
            }
            case StatementKind::TypeAlias: {
                return makeFwdUnique<const Statement>(
                    TypeAlias(
                        typeAlias.name,
                        typeAlias.typeExpression ? typeAlias.typeExpression->clone() : nullptr),
                    location);
            }
            case StatementKind::Var: {
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
            case StatementKind::While: {
                return makeFwdUnique<const Statement>(
                    While(
                        while_.distanceHint,
                        while_.condition ? while_.condition->clone() : nullptr,
                        while_.body ? while_.body->clone() : nullptr),
                    location);               
            }
            default: std::abort(); return nullptr;
        }
    }

    StringView Statement::getDescription() const {
        switch (kind) {
            case StatementKind::Attribution: return "attributed statement"_sv;
            case StatementKind::Bank: return "`bank` declaration"_sv;
            case StatementKind::Block: return "block statement"_sv;
            case StatementKind::Branch: {
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
            case StatementKind::Config: return "`config` directive"_sv;
            case StatementKind::DoWhile: return "`do` ... `while` statement"_sv;
            case StatementKind::Enum: return "`enum` declaration"_sv;
            case StatementKind::ExpressionStatement: return "expression statement"_sv;
            case StatementKind::File: return file.description;
            case StatementKind::For: return "`for` statement"_sv;
            case StatementKind::Func: return "`func` statement"_sv;
            case StatementKind::If: return "`if` statement"_sv;
            case StatementKind::In: return "`in` statement"_sv;
            case StatementKind::InlineFor: return "`inline for` statement"_sv;
            case StatementKind::ImportReference: return importReference.description;
            case StatementKind::InternalDeclaration: return "compiler-internal declaration"_sv;
            case StatementKind::Label: return "label declaration"_sv;
            case StatementKind::Let: return "`let` declaration"_sv;
            case StatementKind::Namespace: return "`namespace` declaration"_sv;
            case StatementKind::Struct: {
                switch (struct_.kind) {
                    case StructKind::Struct: return "`struct` declaration"_sv;
                    case StructKind::Union: return "`union` declaration"_sv;
                    default: {
                        std::abort();
                        break;
                    }
                }
            }
            case StatementKind::TypeAlias: return "`typealias` declaration"_sv;
            case StatementKind::Var: {
                if ((var.qualifiers & Qualifiers::Const) != Qualifiers::None) {
                    return "`const` declaration"_sv;
                } else if ((var.qualifiers & Qualifiers::WriteOnly) != Qualifiers::None) {
                    return "`writeonly` declaration"_sv;
                } else {
                    return "`var` declaration"_sv;
                }
            }
            case StatementKind::While: return "`while` statement"_sv;
            default: std::abort(); return ""_sv;
        }
    }
}