#ifndef WIZ_UTILITY_REPORT_H
#define WIZ_UTILITY_REPORT_H

#include <stdexcept>
#include <string>
#include <memory>
#include <type_traits>

#include <wiz/utility/report_error_flags.h>

namespace wiz {
    class Logger;
    class SourceLocation;

    class Report {
        public:
            static const std::size_t MaxErrors = 64;

            Report(std::unique_ptr<Logger> logger);
            ~Report();

            void error(const std::string& message, const SourceLocation& location, ReportErrorFlags flags = ReportErrorFlags());
            void notice(const std::string& message);
            void log(const std::string& message);

            bool validate();
            bool alive() const;

            Logger* getLogger() const;

        private:
            Report(const Report&) = delete;
            Report& operator =(const Report&) = delete;

            void abort();

            std::unique_ptr<Logger> logger;
            bool aborted;
            std::size_t errors;
            ReportErrorFlags previousFlags;
    };
}
#endif
