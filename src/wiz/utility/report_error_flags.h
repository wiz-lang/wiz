#ifndef WIZ_UTILITY_REPORT_ERROR_FLAGS_H
#define WIZ_UTILITY_REPORT_ERROR_FLAGS_H

#include <wiz/utility/string_view.h>
#include <wiz/utility/bitwise_overloads.h>

namespace wiz {
    enum class ReportErrorFlags {
        None,

        Fatal = 0x01,
        Continued = 0x02,
        InternalError = 0x04,
    };
    WIZ_BITWISE_OVERLOADS(ReportErrorFlags)

    enum class ReportErrorSeverity {
        Fatal,
        Error,
        InternalError,
        Note,
    };

    StringView getReportErrorSeverityName(ReportErrorSeverity severity);
}

#endif
