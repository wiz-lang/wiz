#include <memory>
#include <utility>
#include <clocale>

#include <wiz/ast/statement.h>
#include <wiz/ast/expression.h>
#include <wiz/parser/parser.h>
#include <wiz/parser/scanner.h>
#include <wiz/compiler/config.h>
#include <wiz/compiler/version.h>
#include <wiz/compiler/compiler.h>
#include <wiz/compiler/definition.h>
#include <wiz/compiler/symbol_table.h>
#include <wiz/format/format.h>
#include <wiz/platform/platform.h>
#include <wiz/utility/tty.h>
#include <wiz/utility/path.h>
#include <wiz/utility/logger.h>
#include <wiz/utility/reader.h>
#include <wiz/utility/report.h>
#include <wiz/utility/writer.h>
#include <wiz/utility/optional.h>
#include <wiz/utility/overload.h>
#include <wiz/utility/array_view.h>
#include <wiz/utility/string_view.h>
#include <wiz/utility/string_pool.h>
#include <wiz/utility/option_parser.h>
#include <wiz/utility/import_manager.h>
#include <wiz/utility/resource_manager.h>

namespace wiz {
#if 0
    void dumpAddress(Report* report, const Definition* definition, FormatOutput& output) {
        const auto& variant = definition->variant;

        Optional<Address> address;

        switch (variant.index()) {
            case Definition::VariantType::typeIndexOf<Definition::Var>(): {
                address = variant.get<Definition::Var>().address;
                break;
            }
            case Definition::VariantType::typeIndexOf<Definition::Func>(): {
                address = variant.get<Definition::Func>().address;
                break;
            }
            default: break;
        }

        if (address.hasValue()) {
            if (address->relativePosition.hasValue() && address->absolutePosition.hasValue()) {
                const auto offset = address->relativePosition.get() + output.bankOffsets[address->bank];

                report->log("var " + definition->name.toString()
                    + " @ " + Int128(address->absolutePosition.get()).toString(16)
                    + (address->bank != nullptr
                        ? " (in bank " + address->bank->getName().toString() + ")"
                        : "")
                    + " -> offset = " + Int128(offset).toString(16));
            }
        }
    }
#endif

    int run(Report* report, ResourceManager* resourceManager, ArrayView<const char*> arguments) {
        StringPool stringPool;
        PlatformCollection platformCollection;
        FormatCollection formatCollection;
        StringView inputName;
        StringView outputName;
        std::vector<StringView> importDirs;
        std::unordered_map<StringView, FwdUniquePtr<const Expression>> defines;
        Platform* platform = nullptr;
        Config config;

        if (isTTY(stdout)) {
            std::setvbuf(stdout, 0, _IONBF, 0);
            std::setvbuf(stderr, 0, _IONBF, 0);
        }
    
        enum class OptionType {
            None,
            Output,
            System,
            ImportDir,
            Color,
            Version,
            Help,
            FromStdin,
        };

        std::string platformNames = "";
        for (std::size_t i = 0, count = platformCollection.getPlatformNameCount(); i != count; ++i) {
            platformNames += "\n    `" + platformCollection.getPlatformName(i).toString() + "`";
        }

        const auto systemOptionHelp = stringPool.intern(
            std::string() +
            "    indicates the target system to be used for the program.\n"
            "    (some common systems can be auto-detected from their output filename.)\n\n" +
            "    possible options:" + platformNames);

        auto optionParser = OptionParser<OptionType>{         
            {OptionType::Output, "output", 'o', true, "filename",
                "    specifies the filename where the compiled output will be written."},
            {OptionType::System, "system", 'm', true, "type",
                systemOptionHelp.getData()},
            {OptionType::ImportDir, "import-dir", 'I', true, "path",
                "    adds the directory to the import search path. (for `import`, `embed`, etc.)"},
            {OptionType::Color, "color", 0, true, "setting",
                "    changes the color settings for the terminal output.\n\n"
                "    possible options:\n"
                "    `none` - disable text coloring.\n"
                "    `auto` - automatically use text coloring, if support is available (default)\n"
                "    `ansi` - force ansi escape sequences to be used for text coloring."},
            {OptionType::Version, "version", 0, false, "",
                "    prints the current compiler version."},
            {OptionType::Help, "help", 0, false, "",
                "    displays this help message."},
            {OptionType::FromStdin, "", '-', false, "",
                "    if used as an input path, wiz will read from stdin."},                
        };

        if (!optionParser.parse(arguments)) {
            report->notice(optionParser.getError().toString());
            return 1;
        }

        bool invalidOptions = false;
        bool displayIntroMessage = true;
        const auto options = optionParser.getOptions();

        for (const auto& option : options) {
            switch (option.type) {
                case OptionType::Help:
                case OptionType::Version: {
                    displayIntroMessage = false;
                    break;
                }
                default: break;
            }
        }

        if (displayIntroMessage) {
            report->notice(std::string("version ") + wiz::version::Text);
        }

        for (const auto& option : options) {
            switch (option.type) {
                case OptionType::Output: {
                    if (outputName.getLength() == 0) {
                        outputName = option.value;
                    } else {
                        report->notice("only output filename can be specified. (previously specified as `" + outputName.toString() + "`)");
                        invalidOptions = true;
                    }
                    break;
                }
                case OptionType::System: {
                    platform = platformCollection.findByName(option.value);
                    if (!platform) {
                        report->notice("unrecognized system `" + option.value.toString() + "` provided to `--system` argument.");
                        invalidOptions = true;
                    }
                    break;
                }
                case OptionType::ImportDir: {
                    if (std::find(importDirs.begin(), importDirs.end(), option.value) == importDirs.end()) {
                        importDirs.push_back(option.value);
                    }
                    break;
                }
                case OptionType::Color: {
                    Logger::ColorSetting setting = Logger::ColorSetting::None;
                    if (option.value == "none"_sv) { setting = Logger::ColorSetting::None; } 
                    else if (option.value == "auto"_sv) { setting = Logger::ColorSetting::Auto; }
                    else if (option.value == "ansi"_sv) { setting = Logger::ColorSetting::ForceAnsi; }
                    else {
                        report->notice("unrecognized option `" + option.value.toString() + "` provided to `--color` argument.");
                    }

                    report->getLogger()->setColorSetting(setting);
                    break;
                }
                case OptionType::FromStdin: {
                    if (inputName.getLength() == 0) {
                        inputName = "-"_sv;
                    } else {
                        report->notice("only one input filename can be specified. (previously specified as `" + inputName.toString() + "`)");
                        invalidOptions = true;
                    }
                    break;
                }
                case OptionType::Version: {
                    report->log(std::string("wiz version ") + wiz::version::Text);
                    return 0;
                }
                case OptionType::Help: {
                    report->log("usage: wiz [options] <input>");
                    report->log("");
                    report->log("options:");

                    bool separator = false;

                    for (const auto& definition : optionParser.getDefinitions()) {
                        if (separator) {
                            report->log("");
                        }

                        if (definition.shortname != 0) {
                            report->log(
                                "  -"
                                + (definition.shortname != '-'
                                    ? std::string(1, definition.shortname)
                                    : "")
                                + (definition.parameterName.getLength() != 0
                                    ? " " + definition.parameterName.toString()
                                    : ""));
                        }
                        if (definition.longname.getLength() != 0) {
                            report->log(
                                "  --"
                                + definition.longname.toString()
                                + (definition.parameterName.getLength() != 0
                                    ? "=" + definition.parameterName.toString()
                                    : ""));
                        }
                        if (definition.description.getLength() != 0) {
                            report->log(definition.description.toString());
                        }

                        separator = true;
                    }

                    return 0;
                }
                case OptionType::None: {
                    if (inputName.getLength() == 0) {
                        inputName = option.value;
                    } else {
                        report->notice("only one input filename can be specified. (previously specified as `" + inputName.toString() + "`)");
                        invalidOptions = true;
                    }
                    break;
                }
            }
        }

        importDirs.push_back("."_sv);

        if (outputName.getLength() == 0) {
            report->notice("no target/output file given, please provide an output `-o` parameter.\n  type `wiz --help` to see program usage.");
            return 1;
        }

        if (invalidOptions) {
            return 1;
        }        

        if (inputName.getLength() == 0 && !isTTY(stdin)) {
            inputName = "-"_sv;
        }

        if (inputName == "-"_sv) {
            inputName = "<stdin>"_sv;

            if (!canReceiveEOF(stdin)) {
                report->notice("this TTY cannot properly indicate EOF on stdin to Windows console applications.\n  use `cat | wiz [...]` to run Unix style and use CTRL+D to indicate EOF.\n  use `winpty wiz [...]` to run Windows style and use CTRL+Z + newline for EOF.");
                return 1;
            }
        }
    
        if (inputName.getLength() == 0) {
            report->notice("no source/input file given, please provide an input argument.\n  type `wiz --help` to see program usage.");
            return 1;
        }

        if (platform == nullptr) {
            platform = platformCollection.findByFileExtension(path::getExtension(outputName));

            if (platform == nullptr) {
                report->notice("failed to auto-detect target system.\n  please provide a manual `--system` option.\n  type `wiz --help` to see program usage.");
                return 1;
            }
        }

        report->log(">> Parsing...");
        ImportManager importManager(&stringPool, resourceManager, ArrayView<StringView>(importDirs));
        Parser parser(&stringPool, &importManager, report);

        if (auto program = parser.parse(inputName)) {
            report->log(">> Compiling...");
            Compiler compiler(std::move(program), platform, &stringPool, &config, &importManager, report, std::move(defines));

            if (compiler.compile()) {
                Format* format = nullptr;

                if (const auto formatValue = config.checkString(report, "format"_sv, false)) {
                    format = formatCollection.find(formatValue->second);
                    if (format == nullptr) {
                        report->error("`format` of \"" + formatValue->second.toString() + "\" is not supported.", formatValue->first->location, ReportErrorFlags::of<ReportErrorFlagType::Fatal>());
                        return 1;
                    }
                }

                if (format == nullptr) {
                    format = formatCollection.find(path::getExtension(outputName));

                    if (format == nullptr) {
                        format = formatCollection.find("bin"_sv);
                    }
                }

                report->log(">> Writing ROM...");

                auto banks = compiler.getRegisteredBanks();
                FormatContext context(report, &stringPool, &config, outputName, banks);

                if (!format->generate(context) || !report->validate()) {
                    return 1;
                }

                {
                    auto writer = resourceManager->openWriter(outputName);
                    if (writer && writer->write(context.data)) {
                        report->log(">> Wrote to \"" + outputName.toString() + "\".");
                    } else {
                        report->error("Output file \"" + outputName.toString() + "\" could not be written.", SourceLocation(), ReportErrorFlags::of<ReportErrorFlagType::Fatal>());
                        return 1;
                    }
                }

#if 0
                const auto definitions = compiler.getRegisteredDefinitions();

                for (const auto& definition : definitions) {
                    dumpAddress(report, definition, output);
                }
#endif
                report->notice("Done.");
                return 0;
            }
        }

        return 1;
    }
}

#ifdef __EMSCRIPTEN__

#include <emscripten/bind.h>


class EmLogger : public wiz::Logger {
    public:
        EmLogger(emscripten::val logger)
        : logger(logger) {}

        virtual ~EmLogger() override {}
        
        virtual void log(const std::string& message) override {
            logger.call<void, std::string>("log", message.c_str());
        }

        virtual void error(const wiz::SourceLocation& location, wiz::ReportErrorSeverity severity, const std::string& message) override {
            logger.call<void, std::string, std::string, std::string>("error", location.toString().c_str(), getReportErrorSeverityName(severity).getData(), message.c_str());
        }

        virtual void notice(const std::string& message) override {
            logger.call<void, std::string>("notice", message.c_str());
        }

        virtual ColorSetting getColorSetting() const override { return ColorSetting::Auto; }
        virtual void setColorSetting(ColorSetting value) override { static_cast<void>(value); }

        emscripten::val logger;
};

emscripten::val compile(emscripten::val sources, emscripten::val arguments, emscripten::val logger) {
    wiz::StringPool pool;
    wiz::Report report(std::make_unique<EmLogger>(logger));
    wiz::MemoryResourceManager resourceManager;
    std::vector<const char*> internedArguments;

    const auto sourcesLength = sources["length"].as<unsigned>();
    const auto argumentsLength = arguments["length"].as<unsigned>();

    for (unsigned i = 0; i != sourcesLength; ++i) {
        const auto& source = sources[i];
        resourceManager.registerReadBuffer(pool.intern(source["filename"].as<std::string>()), source["content"].as<std::string>());
    }

    for (unsigned i = 0; i != argumentsLength; ++i) {
        internedArguments.push_back(pool.intern(arguments[i].as<std::string>()).getData());
    }

    const auto result = wiz::run(&report, &resourceManager, wiz::ArrayView<const char*>(internedArguments));

    if (result == 0) {
        std::vector<std::uint8_t> buffer;

        if (resourceManager.getWriteBuffer("hello.nes"_sv, buffer)) {
            return emscripten::val(std::string(buffer.begin(), buffer.end()));
        }
    }

    return emscripten::val::null();
}

EMSCRIPTEN_BINDINGS(wiz_module) {
    emscripten::function("compile", compile);
}

#else

int main(int argc, const char** argv) {
    wiz::Report report(std::make_unique<wiz::FileLogger>(stderr, wiz::Logger::ColorSetting::Auto));
    wiz::FileResourceManager resourceManager;
    wiz::ArrayView<const char*> arguments(argv + 1, argc > 0 ? argc - 1 : 0);
    return wiz::run(&report, &resourceManager, arguments);
}

#endif



