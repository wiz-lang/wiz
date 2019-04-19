#include <wiz/compiler/ir_node.h>


namespace wiz {
    template <>
    void FwdDeleter<IrNode>::operator()(const IrNode* ptr) {
        delete ptr;
    }
}