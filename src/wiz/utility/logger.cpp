#include <cstdint>
#include <utility>

#define WIZ_TTY_INCLUDE_LOWLEVEL
#include <wiz/utility/tty.h>
#include <wiz/utility/logger.h>
#include <wiz/utility/bitwise_overloads.h>

namespace wiz {
    namespace {
        enum class ColorAttributeFlags {
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
        WIZ_BITWISE_OVERLOADS(ColorAttributeFlags)

        ColorAttributeFlags ColorAttributeFlagsBackgroundChannels =
            ColorAttributeFlags::BackgroundRed
            | ColorAttributeFlags::BackgroundGreen
            | ColorAttributeFlags::BackgroundBlue
            | ColorAttributeFlags::BackgroundIntensity;

        ColorAttributeFlags ColorAttributeFlagsForegroundChannels =
            ColorAttributeFlags::ForegroundRed
            | ColorAttributeFlags::ForegroundGreen
            | ColorAttributeFlags::ForegroundBlue
            | ColorAttributeFlags::ForegroundIntensity;

#ifdef _WIN32
        std::int16_t toWindowsAttribute(ColorAttributeFlags flags) {
            std::uint16_t result = 0;

            if ((flags & ColorAttributeFlags::BackgroundRed) != ColorAttributeFlags::None) { result |= BACKGROUND_RED; }
            if ((flags & ColorAttributeFlags::BackgroundGreen) != ColorAttributeFlags::None) { result |= BACKGROUND_GREEN; }
            if ((flags & ColorAttributeFlags::BackgroundBlue) != ColorAttributeFlags::None) { result |= BACKGROUND_BLUE; }
            if ((flags & ColorAttributeFlags::BackgroundIntensity) != ColorAttributeFlags::None) { result |= BACKGROUND_INTENSITY; }
            if ((flags & ColorAttributeFlags::ForegroundRed) != ColorAttributeFlags::None) { result |= FOREGROUND_RED; }
            if ((flags & ColorAttributeFlags::ForegroundGreen) != ColorAttributeFlags::None) { result |= FOREGROUND_GREEN; }
            if ((flags & ColorAttributeFlags::ForegroundBlue) != ColorAttributeFlags::None) { result |= FOREGROUND_BLUE; }
            if ((flags & ColorAttributeFlags::ForegroundIntensity) != ColorAttributeFlags::None) { result |= FOREGROUND_INTENSITY; }

            return static_cast<std::int16_t>(result);
        }

        ColorAttributeFlags fromWindowsAttributes(int16_t signedMask) {
            auto result = ColorAttributeFlags::None;
            const auto flags = static_cast<std::uint16_t>(signedMask);

            if ((flags & BACKGROUND_RED) != 0) { result |= ColorAttributeFlags::BackgroundRed; }
            if ((flags & BACKGROUND_GREEN) != 0) { result |= ColorAttributeFlags::BackgroundGreen; }
            if ((flags & BACKGROUND_BLUE) != 0) { result |= ColorAttributeFlags::BackgroundBlue; }
            if ((flags & BACKGROUND_INTENSITY) != 0) { result |= ColorAttributeFlags::BackgroundIntensity; }
            if ((flags & FOREGROUND_RED) != 0) { result |= ColorAttributeFlags::ForegroundRed; }
            if ((flags & FOREGROUND_GREEN) != 0) { result |= ColorAttributeFlags::ForegroundGreen; }
            if ((flags & FOREGROUND_BLUE) != 0) { result |= ColorAttributeFlags::ForegroundBlue; }
            if ((flags & FOREGROUND_INTENSITY) != 0) { result |= ColorAttributeFlags::ForegroundIntensity; }

            return result;
        }
#endif

        FileLogger::ColorMethod getColorMethod(Logger::ColorSetting colorSetting, std::FILE* file) {
            if (colorSetting == Logger::ColorSetting::None) {
                return FileLogger::ColorMethod::None;
            }
            if (colorSetting == Logger::ColorSetting::ForceAnsi
            || isAnsiTTY(file)) {
                return FileLogger::ColorMethod::Ansi;
            }

#ifdef _WIN32
            if (const auto handle = getStdHandleForFile(file)) {
                if (handle != nullptr)  {
                    return FileLogger::ColorMethod::Windows;
                }
            }
#endif

            return FileLogger::ColorMethod::None;
        }


        struct ConsoleColorContext {
            public:
                ConsoleColorContext(std::FILE* file, FileLogger::ColorMethod colorMethod)
                : file(file),
                needsReset(false),
                colorMethod(colorMethod)
#ifdef _WIN32
                , originalAttributes(0),
                handle(getStdHandleForFile(file))
#endif
                {
#ifdef _WIN32
                    if (handle != nullptr) {
                        CONSOLE_SCREEN_BUFFER_INFO info;
                        GetConsoleScreenBufferInfo(handle, &info);
                        originalAttributes = info.wAttributes;
                    }
#endif
                }

                ~ConsoleColorContext() {
                    if (needsReset) {
                        resetColor();
                    }
                }

                void resetColor() {
                    if (colorMethod == FileLogger::ColorMethod::None) {
                        return;
                    }
                    if (colorMethod == FileLogger::ColorMethod::Ansi) {
                        std::fputs("\033[0m", file);
                    }
#ifdef _WIN32
                    if (colorMethod == FileLogger::ColorMethod::Windows && handle != nullptr) {
                        SetConsoleTextAttribute(handle, originalAttributes);
                    }
#endif

                    needsReset = false;
                }

                void setColor(ColorAttributeFlags colorFlags) {
                    if (colorMethod == FileLogger::ColorMethod::None) {
                        return;
                    }

                    if (colorMethod == FileLogger::ColorMethod::Ansi) {
                        std::fprintf(file, "\033[%d", (colorFlags & ColorAttributeFlags::ForegroundIntensity) != ColorAttributeFlags::None ? 1 : 0);
                        if ((colorFlags & ColorAttributeFlags::DefaultForegroundColor) == ColorAttributeFlags::None) {
                            std::uint8_t mask = 0;
                            if ((colorFlags & ColorAttributeFlags::ForegroundRed) != ColorAttributeFlags::None) { mask |= 0x01; }
                            if ((colorFlags & ColorAttributeFlags::ForegroundGreen) != ColorAttributeFlags::None) { mask |= 0x02; }
                            if ((colorFlags & ColorAttributeFlags::ForegroundBlue) != ColorAttributeFlags::None) { mask |= 0x04; }
                            std::fprintf(file, ";%u", mask + 30);
                        }
                        if ((colorFlags & ColorAttributeFlags::DefaultBackgroundColor) == ColorAttributeFlags::None) {
                            std::uint8_t mask = 0;
                            if ((colorFlags & ColorAttributeFlags::BackgroundRed) != ColorAttributeFlags::None) { mask |= 0x01; }
                            if ((colorFlags & ColorAttributeFlags::BackgroundGreen) != ColorAttributeFlags::None) { mask |= 0x02; }
                            if ((colorFlags & ColorAttributeFlags::BackgroundBlue) != ColorAttributeFlags::None) { mask |= 0x04; }
                            std::fprintf(file, ";%u", mask + 40);
                        }
                        std::fputs("m", file);
                    }

#ifdef _WIN32
                    if (colorMethod == FileLogger::ColorMethod::Windows && handle != nullptr) {
                        if ((colorFlags & ColorAttributeFlags::DefaultBackgroundColor) != ColorAttributeFlags::None) {
                            colorFlags |= (fromWindowsAttributes(originalAttributes) & ColorAttributeFlagsBackgroundChannels);
                        }
                        if ((colorFlags & ColorAttributeFlags::DefaultForegroundColor) != ColorAttributeFlags::None) {
                            colorFlags |= (fromWindowsAttributes(originalAttributes) & ColorAttributeFlagsForegroundChannels);
                        }
                        SetConsoleTextAttribute(handle, toWindowsAttribute(colorFlags));
                    }
#endif

                    needsReset = true;
                }

                std::pair<std::size_t, std::size_t> getWindowSize() const {
#ifdef _WIN32
                    {
                        CONSOLE_SCREEN_BUFFER_INFO info;
                        GetConsoleScreenBufferInfo(handle, &info);
                        return {static_cast<std::size_t>(info.dwSize.X), static_cast<std::size_t>(info.dwSize.Y)};
                    }
#elif defined(__unix__) && !defined(__EMSCRIPTEN__) && defined(_POSIX_SOURCE)
                    {
                        winsize ws;
                        ioctl(WIZ_FILENO(file), TIOCGWINSZ, &ws);
                        return {static_cast<std::size_t>(ws.ws_col), static_cast<std::size_t>(ws.ws_row)};
                    }
#else
                    return {0, 0};
#endif
                }

            private:
                std::FILE* file;
                bool needsReset;
                FileLogger::ColorMethod colorMethod;
#ifdef _WIN32
                std::int16_t originalAttributes;
                HANDLE handle;
#endif
        };

        ColorAttributeFlags getColorForError(ReportErrorSeverity severity) {
            switch (severity) {
                case ReportErrorSeverity::Note:
                    return ColorAttributeFlags::DefaultBackgroundColor
                    | ColorAttributeFlags::ForegroundRed
                    | ColorAttributeFlags::ForegroundBlue
                    | ColorAttributeFlags::ForegroundIntensity;
                default:
                    return ColorAttributeFlags::DefaultBackgroundColor
                    | ColorAttributeFlags::ForegroundRed
                    | ColorAttributeFlags::ForegroundIntensity;
            }
        }

        void printRichString(std::size_t& column, std::size_t windowWidth, ConsoleColorContext& context, std::FILE* file, StringView message) {
            static_cast<void>(column);
            static_cast<void>(windowWidth);
            char quote = 0;
            bool escape = false;

            // TODO: implement word-wrapping that is punctuation/spacing aware, and nicely word-wraps quoted entities.

            std::size_t lastIndex = 0;
            for (std::size_t i = 0; i != message.getLength(); ++i) {
                char c = message[i];

                if (escape) {
                    escape = false;
                } else {
                    if (c == '\\') {
                        escape = true;
                    } else if ((c == '`' || c == '"') && (quote == 0 || quote == c)) {
                        std::fwrite(&message[lastIndex], i - lastIndex + (quote != 0 ? 1 : 0), 1, file);

                        if (quote != 0) {
                            quote = 0;
                            context.resetColor();
                        } else {
                            quote = c;
                            context.setColor(
                                ColorAttributeFlags::DefaultBackgroundColor
                                | ColorAttributeFlags::DefaultForegroundColor
                                | ColorAttributeFlags::ForegroundIntensity);
                        }
                        lastIndex = quote != 0 ? i : i + 1;
                    }
                }
            }

            if (lastIndex < message.getLength()) {
                std::fwrite(&message[lastIndex], message.getLength() - lastIndex, 1, file);
            }
            context.resetColor();
        }

        void printWordWrapped(std::size_t& column, std::size_t windowWidth, std::FILE* file, StringView message) {
            static_cast<void>(column);
            static_cast<void>(windowWidth);

            // TODO: implement word-wrapping that is punctuation/spacing aware, and nicely word-wraps quoted entities.

            std::fwrite(message.getData(), message.getLength(), 1, file);
        }
    }

    FileLogger::FileLogger(FILE* file, ColorSetting colorSetting)
    : file(file),
    colorSetting(colorSetting),
    colorMethod(getColorMethod(colorSetting, file)) {}

    FileLogger::~FileLogger() {}

    // TODO: nice-to-have: word-wrapping support for TTY
    // See GetConsoleScreenBufferInfo on Windows / struct winsize size; ioctl(STDOUT_FILENO, TIOCGWINSZ, &size); on POSIX.
    void FileLogger::log(const std::string& message) {
        ConsoleColorContext context(file, colorMethod);

        std::size_t column = 0;
        auto windowSize = context.getWindowSize();

        if (message.rfind(">>", 0) == 0) {
            context.setColor(
                ColorAttributeFlags::DefaultBackgroundColor
                | ColorAttributeFlags::DefaultForegroundColor
                | ColorAttributeFlags::ForegroundIntensity);
            printWordWrapped(column, windowSize.first, file, StringView(message));
        } else {
            context.resetColor();            
            printRichString(column, windowSize.first, context, file, StringView(message));
        }
        std::fwrite("\n", 1, 1, file);
    }

    // TODO: nice-to-have: word-wrapping support for TTY
    void FileLogger::error(const SourceLocation& location, ReportErrorSeverity severity, const std::string& message) {
        ConsoleColorContext context(file, colorMethod);
        context.setColor(
            ColorAttributeFlags::DefaultBackgroundColor
            | ColorAttributeFlags::DefaultForegroundColor
            | ColorAttributeFlags::ForegroundIntensity);

        std::size_t column = 0;
        auto windowSize = context.getWindowSize();

        const auto locationString = location.toString();
        if (locationString.size() != 0) {
            printWordWrapped(column, windowSize.first, file, StringView(locationString));
            printWordWrapped(column, windowSize.first, file, ": "_sv); 
        }

        context.setColor(getColorForError(severity));
        printWordWrapped(column, windowSize.first, file, getReportErrorSeverityName(severity));
        printWordWrapped(column, windowSize.first, file, ": "_sv); 
        context.resetColor();
        printRichString(column, windowSize.first, context, file, StringView(message));
        std::fwrite("\n", 1, 1, file);
    }

    // TODO: nice-to-have: word-wrapping support for TTY
    void FileLogger::notice(const std::string& message) {
        ConsoleColorContext context(file, colorMethod);
        context.setColor(
            ColorAttributeFlags::DefaultBackgroundColor
            | ColorAttributeFlags::DefaultForegroundColor
            | ColorAttributeFlags::ForegroundIntensity);

        std::size_t column = 0;
        auto windowSize = context.getWindowSize();

        std::fputs("* wiz: ", file);
        context.resetColor();
        printRichString(column, windowSize.first, context, file, StringView(message));
        std::fwrite("\n", 1, 1, file);
    }

    Logger::ColorSetting FileLogger::getColorSetting() const {
        return colorSetting;
    }

    void FileLogger::setColorSetting(ColorSetting value) {
        colorSetting = value;
        colorMethod = getColorMethod(colorSetting, file);
    }



    MemoryLogger::MemoryLogger() {}
    MemoryLogger::~MemoryLogger() {}

    void MemoryLogger::log(const std::string& message) {
        logs.push_back(message);
    }

    void MemoryLogger::error(const SourceLocation& location, ReportErrorSeverity severity, const std::string& message) {
        errors.push_back(ErrorMessage(location, severity, message));
    }

    void MemoryLogger::notice(const std::string& message) {
        notices.push_back(message);
    }



    MemoryLogger::ErrorMessage::ErrorMessage(const SourceLocation& location, ReportErrorSeverity severity, const std::string& message)
    : location(location), severity(severity), message(message) {}

    MemoryLogger::ErrorMessage::~ErrorMessage() {}
}