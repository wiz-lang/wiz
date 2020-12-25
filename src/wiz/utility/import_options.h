#ifndef WIZ_UTILITY_IMPORT_OPTIONS_H
#define WIZ_UTILITY_IMPORT_OPTIONS_H

#include <wiz/utility/bitwise_overloads.h>

namespace wiz {
    enum class ImportOptions {
        None,

        AppendExtension = 0x01,
        AllowShellResources = 0x02,
    };

    WIZ_BITWISE_OVERLOADS(ImportOptions)
}

#endif