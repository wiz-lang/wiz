#include <cstdint>
#include <wiz/utility/text.h>

namespace wiz {
    namespace text {
        std::string escape(StringView text, char quote) {
            const auto str = text.getData();
            const auto length = text.getLength();

            std::string result;
            result.reserve(length);

            for (std::size_t i = 0; i != length; ++i) {
                const auto c = str[i];
                switch (c) {
                    case '\t': result += "\\t"; break;
                    case '\r': result += "\\r"; break;
                    case '\n': result += "\\n"; break;
                    case '\f': result += "\\f"; break;
                    case '\b': result += "\\b"; break;
                    case '\a': result += "\\a"; break;
                    case '\0': result += "\\0"; break;
                    case '\\': result += "\\\\"; break;
                    case '\'': {
                        if (c == quote) {
                            result += "\\'";
                        } else {
                            result += c;
                        }
                        break;
                    }
                    case '\"': {
                        if (c == quote) {
                            result += "\\\"";
                        } else {
                            result += c;
                        }
                        break;
                    }
                    default: {
                        if (c < 32 || c >= 127) {
                            const auto high = (static_cast<std::uint8_t>(c) >> 4) & 0x0F;
                            const auto low = static_cast<std::uint8_t>(c) & 0x0F;
                            result += "\\x";
                            result += static_cast<char>(high > 9 ? high - 10 + 'A' : high + '0');
                            result += static_cast<char>(low > 9 ? low - 10 + 'A' : low + '0');
                        } else {
                            result += c;
                        }
                        break;
                    }
                }
            }
            return result;
        }

        std::string truncate(StringView text, std::size_t limit) {
            if (text.getLength() > limit) {
                std::size_t sublen = limit >= 3 ? limit - 3 : 0;

                std::string result;
                result.reserve(sublen + 3);
                result += text.sub(0, sublen).toString();
                result += "...";

                return result;
            } else {
                return text.toString();
            }
        }

        std::vector<StringView> split(StringView text, StringView delimiters, std::size_t offset) {
            std::vector<StringView> result;
            std::size_t last = offset;
            std::size_t pos = 0;
            do {
                pos = text.findFirstOf(delimiters, last);
                if (pos < text.getLength()) {
                    result.push_back(text.sub(last, pos - last));
                    last = pos + 1;
                }
            } while (pos < text.getLength());

            result.push_back(text.sub(last));
            return result;
        }
    }
}