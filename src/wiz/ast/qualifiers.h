#ifndef WIZ_AST_MODIFIERS_H
#define WIZ_AST_MODIFIERS_H

#include <wiz/utility/bit_flags.h>

namespace wiz {
    enum class Qualifier {
        Const,
        WriteOnly,
        Extern,
        Far,
        LValue,

        Count
    };

    using Qualifiers = BitFlags<Qualifier, Qualifier::Count>;
}
#endif
