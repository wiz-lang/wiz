#ifndef WIZ_UTILITY_TEXT_H
#define WIZ_UTILITY_TEXT_H

#include <string>
#include <vector>
#include <iterator>
#include <type_traits>

#include <wiz/utility/string_view.h>

namespace wiz {
    namespace text {
        template <typename Iter, std::enable_if_t<std::is_same<typename std::iterator_traits<Iter>::value_type, std::string>::value>* = nullptr>
        std::string join(Iter begin, Iter end, const std::string& separator) {
            std::size_t length = 0;
            bool prepend = false;

            for (auto it = begin; it != end; ++it) {
                length += (prepend ? separator.length() : 0) + it->length();
                prepend = true;
            }

            std::string result;
            result.reserve(length);

            prepend = false;
            for (auto it = begin; it != end; ++it) {
                result += (prepend ? separator : "") + *it;
                prepend = true;
            }
            return result;
        }

        template <typename Iter, std::enable_if_t<std::is_same<typename std::iterator_traits<Iter>::value_type, StringView>::value>* = nullptr>
        std::string join(Iter begin, Iter end, const std::string& separator) {
            std::size_t length = 0;
            bool prepend = false;

            for (auto it = begin; it != end; ++it) {
                length += (prepend ? separator.length() : 0) + it->getLength();
                prepend = true;
            }

            std::string result;
            result.reserve(length);

            prepend = false;
            for (auto it = begin; it != end; ++it) {
                result += (prepend ? separator : "") + it->toString();
                prepend = true;
            }
            return result;
        }

        std::string escape(StringView text, char quote);
        std::string truncate(StringView text, std::size_t length);
        std::vector<StringView> split(StringView text, StringView delimiters, std::size_t offset = 0);
        std::string replaceAll(const std::string& text, const std::string& search, const std::string& replace);

#ifdef _WIN32
        constexpr StringView OsNewLine = "\r\n"_sv;
#else
        constexpr StringView OsNewLine = "\n"_sv;
#endif
    }
}

#endif