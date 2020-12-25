#ifndef WIZ_AST_QUALIFIERS_H
#define WIZ_AST_QUALIFIERS_H

#include <cstdint>

#include <wiz/utility/bitwise_overloads.h>

namespace wiz {
    enum class Qualifiers {
        None,

        Const = 0x01,
        WriteOnly = 0x02,
        Extern = 0x04,
        Far = 0x08,
        LValue = 0x10,
    };

    WIZ_BITWISE_OVERLOADS(Qualifiers)
}
#endif
