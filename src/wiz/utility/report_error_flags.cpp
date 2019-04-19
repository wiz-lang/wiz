#include <wiz/utility/report_error_flags.h>

namespace wiz {
    StringView getReportErrorSeverityName(ReportErrorSeverity severity) {
        switch (severity) {
            case ReportErrorSeverity::Fatal: return "fatal"_sv;
            default: case ReportErrorSeverity::Error: return "error"_sv;
            case ReportErrorSeverity::InternalError: return "internal error"_sv;
            case ReportErrorSeverity::Note: return "note"_sv;
        }
    }
}