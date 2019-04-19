#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/compiler/definition.h>

namespace wiz {
    template <>
    void FwdDeleter<Definition>::operator()(const Definition* ptr) {
        delete ptr;
    }
}