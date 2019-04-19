#ifndef WIZ_AST_MODIFIERS_H
#define WIZ_AST_MODIFIERS_H

#include <wiz/utility/bit_flags.h>

namespace wiz {
    enum class Modifier {
        Const,
        WriteOnly,
        Extern,
        Far,

        Count
    };

    using Modifiers = BitFlags<Modifier, Modifier::Count>;

    enum class PointerQualifier {
        Const,
        WriteOnly,
        Far,

        Count
    };

    using PointerQualifiers = BitFlags<PointerQualifier, PointerQualifier::Count>;
}
#endif
