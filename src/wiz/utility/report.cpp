#include <wiz/utility/report.h>
#include <wiz/utility/logger.h>

namespace wiz {
    namespace {
        ReportErrorSeverity getSeverity(ReportErrorFlags flags, ReportErrorFlags previousFlags) {
            if (flags.has<ReportErrorFlagType::InternalError>()) {
                return ReportErrorSeverity::InternalError;
            } else if (flags.has<ReportErrorFlagType::Fatal>()) {
                return ReportErrorSeverity::Fatal;
            } else if (previousFlags.has<ReportErrorFlagType::Continued>()) {
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

            bool aborting = flags.has<ReportErrorFlagType::Fatal>() || previousFlags.has<ReportErrorFlagType::Fatal>();
            previousFlags = flags | (previousFlags.intersect<ReportErrorFlagType::Fatal>());

            if (flags.has<ReportErrorFlagType::Continued>()) {
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