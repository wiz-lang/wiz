#include <wiz/compiler/ir_node.h>

namespace wiz {
    template <>
    void FwdDeleter<IrNode>::operator()(const IrNode* ptr) {
        delete ptr;
    }

    IrNode::~IrNode() {
        switch (kind) {
            case IrNodeKind::PushRelocation: pushRelocation.~PushRelocation(); break;
            case IrNodeKind::PopRelocation: popRelocation.~PopRelocation(); break;
            case IrNodeKind::Label: label.~Label(); break;
            case IrNodeKind::Code: code.~Code(); break;
            case IrNodeKind::Var: var.~Var(); break;
        }
    }
}