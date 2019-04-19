#ifndef WIZ_UTILITY_LOGGER_H
#define WIZ_UTILITY_LOGGER_H

#include <string>
#include <vector>
#include <cstdio>

#include <wiz/utility/source_location.h>
#include <wiz/utility/report_error_flags.h>

namespace wiz {
    class SourceLocation;

    class Logger {
        public:
            enum class ColorSetting {
                None,
                Auto,
                ForceAnsi,
            };

            virtual ~Logger() {}

            virtual void log(const std::string& message) = 0;
            virtual void error(const SourceLocation& location, ReportErrorSeverity severity, const std::string& message) = 0;
            virtual void notice(const std::string& message) = 0;

            virtual ColorSetting getColorSetting() const = 0;
            virtual void setColorSetting(ColorSetting value) = 0;
    };

    class FileLogger : public Logger {
        public:
            enum class ColorMethod {
                None,
                Ansi,
                Windows,
            };

            FileLogger(FILE* file, ColorSetting colorSetting);
            virtual ~FileLogger() override;

            virtual void log(const std::string& message) override;
            virtual void error(const SourceLocation& location, ReportErrorSeverity severity, const std::string& message) override;
            virtual void notice(const std::string& message) override;

            virtual ColorSetting getColorSetting() const override;
            virtual void setColorSetting(ColorSetting value) override;

        private:
            FILE* file;
            ColorSetting colorSetting;
            ColorMethod colorMethod;
    };

    class MemoryLogger : public Logger {
        public:
            MemoryLogger();
            virtual ~MemoryLogger() override;
            
            virtual void log(const std::string& message) override;
            virtual void error(const SourceLocation& location, ReportErrorSeverity severity, const std::string& message) override;
            virtual void notice(const std::string& message) override;

            virtual ColorSetting getColorSetting() const override { return ColorSetting::None; }
            virtual void setColorSetting(ColorSetting) override {}

            class ErrorMessage {
                public:
                    ErrorMessage(const SourceLocation& location, ReportErrorSeverity severity, const std::string& message);
                    ~ErrorMessage();

                    SourceLocation location;
                    ReportErrorSeverity severity;
                    std::string message;
            };

            std::vector<std::string> logs;
            std::vector<ErrorMessage> errors;
            std::vector<std::string> notices;
    };
}

#endif

