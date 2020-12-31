#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/compiler/definition.h>

namespace wiz {
    template <>
    void FwdDeleter<Definition>::operator()(const Definition* ptr) {
        delete ptr;
    }

    Definition::~Definition() {
        switch (kind) {
            case DefinitionKind::BuiltinBankType: builtinBankType.~BuiltinBankType(); break;
            case DefinitionKind::BuiltinBoolType: builtinBoolType.~BuiltinBoolType(); break;
            case DefinitionKind::BuiltinIntegerType: builtinIntegerType.~BuiltinIntegerType(); break;
            case DefinitionKind::BuiltinIntegerExpressionType: builtinIntegerExpressionType.~BuiltinIntegerExpressionType(); break;
            case DefinitionKind::BuiltinIntrinsicType: builtinIntrinsicType.~BuiltinIntrinsicType(); break;
            case DefinitionKind::BuiltinLetType: builtinLetType.~BuiltinLetType(); break;
            case DefinitionKind::BuiltinLoadIntrinsic: builtinLoadIntrinsic.~BuiltinLoadIntrinsic(); break;
            case DefinitionKind::BuiltinRangeType: builtinRangeType.~BuiltinRangeType(); break;
            case DefinitionKind::BuiltinRegister: builtinRegister.~BuiltinRegister(); break;
            case DefinitionKind::BuiltinTypeOfType: builtinTypeOfType.~BuiltinTypeOfType(); break;
            case DefinitionKind::BuiltinVoidIntrinsic: builtinVoidIntrinsic.~BuiltinVoidIntrinsic(); break;
            case DefinitionKind::Bank: bank.~Bank(); break;
            case DefinitionKind::Enum: enum_.~Enum(); break;
            case DefinitionKind::EnumMember: enumMember.~EnumMember(); break;
            case DefinitionKind::Func: func.~Func(); break;
            case DefinitionKind::Let: let.~Let(); break;
            case DefinitionKind::Namespace: namespace_.~Namespace(); break;
            case DefinitionKind::Struct: struct_.~Struct(); break;
            case DefinitionKind::StructMember: structMember.~StructMember(); break;
            case DefinitionKind::TypeAlias: typeAlias.~TypeAlias(); break;
            case DefinitionKind::Var: var.~Var(); break;
        }
    }

    Optional<Address> Definition::getAddress() const {
        switch (kind) {
            case DefinitionKind::Var: return var.address;
            case DefinitionKind::Func: return func.address;
            default: return Optional<Address>();
        }
    }
}