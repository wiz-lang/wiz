#include <wiz/utility/report.h>
#include <wiz/utility/logger.h>

namespace wiz {
    namespace {
        ReportErrorSeverity getSeverity(ReportErrorFlags flags, ReportErrorFlags previousFlags) {
            if ((flags & ReportErrorFlags::InternalError) != ReportErrorFlags::None) {
                return ReportErrorSeverity::InternalError;
            } else if ((flags & ReportErrorFlags::Fatal) != ReportErrorFlags::None) {
                return ReportErrorSeverity::Fatal;
            } else if ((previousFlags & ReportErrorFlags::Continued) != ReportErrorFlags::None) {
                return ReportErrorSeverity::Note;
            } else {
                return ReportErrorSeverity::Error;
            }
        }
    }

    Report::Report(std::unique_ptr<Logger> logger)
    : logger(std::move(logger)), aborted(false), errors(0), previousFlags() {}

    Report::~Report() {}

    void Report::error(const std::string& message, const SourceLocation& location, ReportErrorFlags flags) {
        if (!aborted) {
            auto severity = getSeverity(flags, previousFlags);
            logger->error(location, severity, message);

            bool aborting = (flags & ReportErrorFlags::Fatal) != ReportErrorFlags::None || (previousFlags & ReportErrorFlags::Fatal) != ReportErrorFlags::None;
            previousFlags = flags | (previousFlags & ReportErrorFlags::Fatal);

            if ((flags & ReportErrorFlags::Continued) != ReportErrorFlags::None) {
                aborting = false;
            } else {
                errors++;
            }

            if (errors >= MaxErrors) {
                logger->error(location, ReportErrorSeverity::Fatal, "too many errors encountered. stopping.");
                aborting = true;
            }

            if (aborting) {
                abort();
            }
        }
    }

    bool Report::validate() {
        if (errors > 0) {
            abort();
        }
        return alive();
    }

    bool Report::alive() const {
        return !aborted;
    }

    void Report::notice(const std::string& message) {
        logger->notice(message);
    }

    void Report::log(const std::string& message) {
        logger->log(message);
    }

    void Report::abort() {
        if (!aborted) {
            notice("failed with " + std::to_string(errors) + " error(s).");
            aborted = true;
        }
    }

    Logger* Report::getLogger() const {
        return logger.get();
    }
}