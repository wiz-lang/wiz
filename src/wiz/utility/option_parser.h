#ifndef WIZ_UTILITY_OPTION_PARSER_H
#define WIZ_UTILITY_OPTION_PARSER_H

#include <string>
#include <vector>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <unordered_map>
#include <initializer_list>
#include <wiz/utility/array_view.h>
#include <wiz/utility/string_view.h>

namespace wiz {
    template <typename T>
    struct OptionDefinition {
        OptionDefinition(
            T type,
            const char* longname,
            char shortname,
            bool parameterized,
            const char* parameterName,
            const char* description)
        : type(type),
        longname(longname),
        shortname(shortname),
        parameterized(parameterized),
        parameterName(parameterName),
        description(description) {}

        T type;
        StringView longname;
        char shortname;
        bool parameterized;
        StringView parameterName;
        StringView description;
    };

    template <typename T>
    struct OptionData {
        OptionData(
            T type,
            StringView value)
        : type(type),
        value(value) {}

        T type;
        StringView value;
    };

    template <typename T>
    class OptionParser {
        public:
            OptionParser(std::initializer_list<OptionDefinition<T>> definitions)
            : definitions(definitions) {
                init();
            }

            explicit OptionParser(const std::vector<OptionDefinition<T>>& definitions)
            : definitions(definitions) {
                init();
            }

            ArrayView<OptionDefinition<T>> getDefinitions() const {
                return ArrayView<OptionDefinition<T>>(definitions.data(), definitions.size());
            }

            StringView getError() const {
                return StringView(errorMessage);
            }

            ArrayView<OptionData<T>> getOptions() const {
                return ArrayView<OptionData<T>>(options.data(), options.size());
            }

            bool parse(ArrayView<const char*> arguments) {
                options.clear();
                errorMessage.clear();

                bool positionalMode = false;
                std::size_t activeOption = 0;                

                for (std::size_t i = 0, argumentCount = arguments.getLength(); i != argumentCount; ++i) {
                    const auto& argument = StringView(arguments[i]);

                    if (positionalMode) {
                        options.emplace_back(T(), argument);
                        continue;
                    }

                    if (const auto definition = getOptionDefinition(activeOption)) { 
                        activeOption = 0;

                        if (argument[0] != '-') {
                            options.emplace_back(definition->type, argument);
                            continue;
                        } else {
                            options.emplace_back(definition->type, StringView());
                        }
                    }

                    if (argument[0] == '-') {
                        if (argument[1] == 0) {
                            activeOption = shortnames[static_cast<unsigned char>('-')];
                            if (const auto definition = getOptionDefinition(activeOption)) {
                                if (!definition->parameterized) {
                                    options.emplace_back(definition->type, StringView());
                                    activeOption = 0;
                                }
                            } else {
                                return error("invalid option `-`");
                            }
                        } else if (argument[1] == '-') {
                            if (argument[2] == 0) {
                                positionalMode = true;
                            } else {
                                StringView key;
                                StringView value;
                                const auto hasValue = parseLongnameArgument(argument, key, value);
                                const auto it = longnames.find(key);

                                if (it != longnames.end()) {
                                    activeOption = it->second;
                                } else {
                                    activeOption = 0;
                                }

                                if (const auto definition = getOptionDefinition(activeOption)) {
                                    if (definition->parameterized) {
                                        if (hasValue) {
                                            options.emplace_back(definition->type, value);
                                            activeOption = 0;
                                        }
                                    } else {
                                        if (hasValue) {
                                            return error("option `" + definition->longname.toString() + "` takes no value, but `" + argument.toString() + "` was specified");
                                        } else {
                                            options.emplace_back(definition->type, StringView());
                                            activeOption = 0;
                                        }
                                    }
                                } else {
                                    return error(
                                        "invalid option `--" + key.toString()
                                        + (hasValue ? "` (specified by argument `" + argument.toString() + "`)" : "`"));
                                }
                            }
                        } else {
                            for (std::size_t j = 1, length = argument.getLength(); j != length; ++j) {
                                activeOption = shortnames[static_cast<unsigned char>(argument[j])];

                                if (const auto definition = getOptionDefinition(activeOption)) {
                                    if (definition->parameterized) {
                                        if (j != length - 1) {
                                            options.emplace_back(definition->type, argument.sub(j + 1));
                                            activeOption = 0;
                                        }
                                        break;
                                    } else {
                                        options.emplace_back(definition->type, StringView());
                                        activeOption = 0;
                                    }
                                } else {
                                    return error(std::string() + "invalid option `-" + argument[j] + "`");
                                }
                            }
                        }
                    } else {
                        options.emplace_back(T(), argument);
                    }
                }
                
                return true;
            }

        private:
            void init() {
                memset(shortnames, 0, sizeof(shortnames));

                for (std::size_t i = 0, size = definitions.size(); i != size; ++i) {
                    const auto& definition(definitions[i]);
                    if (definition.longname.getLength() != 0) {
                        longnames[definition.longname] = i + 1;
                    }
                    if (definition.shortname != 0) {
                        shortnames[static_cast<unsigned char>(definition.shortname)] = i + 1;
                    }
                }
            }

            bool error(const std::string& message) {
                errorMessage = message;
                return false;
            }

            const OptionDefinition<T>* getOptionDefinition(std::size_t index) const {
                return index != 0 ? &definitions[index - 1] : nullptr;
            }

            bool parseLongnameArgument(StringView argument, StringView& key, StringView& value) const {
                const auto data = argument.getData();
                const auto length = argument.getLength();
                const auto assignmentDelimiter = static_cast<std::size_t>(std::find(data + 2, data + length, '=') - data);
                if (assignmentDelimiter < length) {
                    key = argument.sub(2, assignmentDelimiter - 2);
                    value = argument.sub(assignmentDelimiter + 1);
                    return true;
                } else {
                    key = argument.sub(2);
                    value = StringView();
                    return false;
                }
            }
            
            std::unordered_map<StringView, std::size_t> longnames;
            std::size_t shortnames[256];
            std::vector<OptionDefinition<T>> definitions;

            std::string errorMessage;
            std::vector<OptionData<T>> options;
    };
}

#endif
