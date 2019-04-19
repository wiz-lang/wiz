#ifndef WIZ_UTILITY_REPORT_ERROR_FLAGS_H
#define WIZ_UTILITY_REPORT_ERROR_FLAGS_H

#include <wiz/utility/bit_flags.h>
#include <wiz/utility/string_view.h>

namespace wiz {
    enum class ReportErrorFlagType {
        Fatal,
        Continued,
        InternalError,

        Count
    };

    using ReportErrorFlags = BitFlags<ReportErrorFlagType, ReportErrorFlagType::Count>;

    enum class ReportErrorSeverity {
        Fatal,
        Error,
        InternalError,
        Note,
    };

    StringView getReportErrorSeverityName(ReportErrorSeverity severity);
}

#endif
