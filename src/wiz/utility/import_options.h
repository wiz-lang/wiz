#ifndef WIZ_UTILITY_IMPORT_OPTIONS_H
#define WIZ_UTILITY_IMPORT_OPTIONS_H
    #include <wiz/utility/bit_flags.h>

    namespace wiz {
        enum class ImportOptionType {
            AppendExtension,
            AllowShellResources,

            Count,
        };

        using ImportOptions = BitFlags<ImportOptionType, ImportOptionType::Count>;
    }
#endif