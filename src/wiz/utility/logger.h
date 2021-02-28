#ifndef WIZ_UTILITY_LOGGER_H
#define WIZ_UTILITY_LOGGER_H

#include <string>
#include <vector>
#include <cstdio>

#include <wiz/utility/source_location.h>
#include <wiz/utility/report_error_flags.h>

namespace wiz {
    class SourceLocation;

    enum class LoggerColorSetting {
        None,
        Auto,
        ForceAnsi,
    };

    enum class LoggerColorAttributeFlags {
        None,

        BackgroundRed = 0x01,
        BackgroundGreen = 0x02,
        BackgroundBlue = 0x04,
        BackgroundIntensity = 0x08,
        ForegroundRed = 0x10,
        ForegroundGreen = 0x20,
        ForegroundBlue = 0x40,
        ForegroundIntensity = 0x80,
        DefaultBackgroundColor = 0x100,
        DefaultForegroundColor = 0x200,
    };
    WIZ_BITWISE_OVERLOADS(LoggerColorAttributeFlags)

    class Logger {
        public:
            virtual ~Logger() {}

            virtual void log(const std::string& message) = 0;
            virtual void error(const SourceLocation& location, ReportErrorSeverity severity, const std::string& message) = 0;
            virtual void notice(const std::string& message) = 0;

            virtual LoggerColorSetting getColorSetting() const = 0;
            virtual void setColorSetting(LoggerColorSetting value) = 0;
    };

    class FileLogger : public Logger {
        public:
            enum class ColorMethod {
                None,
                Ansi,
                Windows,
            };

            FileLogger(FILE* file, LoggerColorSetting colorSetting);
            virtual ~FileLogger() override;

            virtual void log(const std::string& message) override;
            virtual void error(const SourceLocation& location, ReportErrorSeverity severity, const std::string& message) override;
            virtual void notice(const std::string& message) override;

            virtual LoggerColorSetting getColorSetting() const override;
            virtual void setColorSetting(LoggerColorSetting value) override;

        private:
            FILE* file;
            LoggerColorSetting colorSetting;
            ColorMethod colorMethod;
    };

    class MemoryLogger : public Logger {
        public:
            MemoryLogger();
            virtual ~MemoryLogger() override;
            
            virtual void log(const std::string& message) override;
            virtual void error(const SourceLocation& location, ReportErrorSeverity severity, const std::string& message) override;
            virtual void notice(const std::string& message) override;

            virtual LoggerColorSetting getColorSetting() const override { return LoggerColorSetting::None; }
            virtual void setColorSetting(LoggerColorSetting) override {}

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

