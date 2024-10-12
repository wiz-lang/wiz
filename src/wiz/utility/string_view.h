#ifndef WIZ_UTILITY_STRING_VIEW_H
#define WIZ_UTILITY_STRING_VIEW_H

#include <cstddef>
#include <cstring>
#include <string>
#include <cstdint>

#include <wiz/utility/macros.h>

namespace wiz {
    class StringView {
        public:
            constexpr StringView()
            : data(""), length(0) {}

            constexpr StringView(const StringView& other)
            : data(other.data), length(other.length) {}

            explicit StringView(const std::string& text)
            : data(text.data()), length(text.length()) {}

            explicit StringView(const char* text)
            : data(text), length(std::strlen(text)) {}

            constexpr StringView(const char* text, std::size_t length)
            : data(text), length(length) {}

            WIZ_FORCE_INLINE constexpr const char* getData() const {
                return data;
            }

            WIZ_FORCE_INLINE constexpr std::size_t getLength() const {
                return length;
            }

            WIZ_FORCE_INLINE std::size_t size() const {
                return length;
            }

            WIZ_FORCE_INLINE constexpr const char* begin() const {
                return data;
            }

            WIZ_FORCE_INLINE constexpr const char* end() const {
                return data + length;
            }

            WIZ_FORCE_INLINE std::string toString() const {
                return std::string(data, data + length);
            }

            constexpr int compare(const StringView& other) const {
                if (data == other.data && length == other.length) {
                    return 0;
                }

                for (std::size_t i = 0; i != length && i != other.length; ++i) {
                    const auto a = data[i];
                    const auto b = other.data[i];
                    if (a != b) {
                        return a < b ? -1 : 1;
                    }
                }

                if (length != other.length) {
                    return length < other.length ? -1 : 1;
                }

                return 0;
            }

            constexpr StringView sub(std::size_t index, std::size_t size = SIZE_MAX) const {
                if (index > length) {
                    index = length;
                }
                if (size > length - index) {
                    size = length - index;
                }
                return StringView(data + index, size);
            }

            constexpr std::size_t find(StringView sub, std::size_t offset = 0) const {
                const auto subLength = sub.getLength();
                if (offset < length && offset + subLength <= length) {
                    for (std::size_t i = offset, end = length - subLength + 1; i != end; ++i) {
                        bool match = true;

                        for (std::size_t j = 0; j != subLength; ++j) {
                            if (data[i + j] != sub[j]) {
                                match = false;
                                break;
                            }
                        } 

                        if (match) {
                            return i;
                        }
                    }
                }

                return SIZE_MAX;
            }

            constexpr std::size_t rfind(StringView sub, std::size_t offset = SIZE_MAX) const {
                const auto subLength = sub.getLength();

                if (offset >= length) {
                    offset = length - 1;
                }

                for (std::size_t i = offset; i < length; i--) {
                    bool match = true;

                    for (std::size_t j = 0; j != subLength; ++j) {
                        if (data[i + j] != sub[j]) {
                            match = false;
                            break;
                        }
                    }

                    if (match) {
                        return i;
                    }
                }

                return SIZE_MAX;
            }

            constexpr std::size_t findFirstOf(StringView set, std::size_t offset = 0) const {
                for (std::size_t i = offset; i < length; ++i) {
                    for (std::size_t j = 0; j != set.getLength(); ++j) {
                        if (data[i] == set[j]) {
                            return i;
                        }
                    }
                }

                return SIZE_MAX;
            }

            constexpr std::size_t findLastOf(StringView set, std::size_t offset = SIZE_MAX) const {
                if (offset >= length) {
                    offset = length - 1;
                }

                for (std::size_t i = offset; i < length; --i) {
                    for (std::size_t j = 0; j != set.getLength(); ++j) {
                        if (data[i] == set[j]) {
                            return i;
                        }
                    }
                }

                return SIZE_MAX;
            }

            constexpr bool contains(StringView sub) const {
                return find(sub) < length;
            }

            constexpr bool startsWith(StringView sub) const {
                return sub.getLength() > 0
                && rfind(sub, 0) == 0;
            }

            constexpr bool endsWith(StringView sub) const {
                return length >= sub.getLength()
                && find(sub, length - sub.getLength()) == length - sub.getLength();
            }

            WIZ_FORCE_INLINE StringView& operator=(const StringView& other) {
                data = other.data;
                length = other.length;
                return *this;
            }

            WIZ_FORCE_INLINE constexpr const char& operator [](std::size_t index) const {
                return data[index];
            }

            WIZ_FORCE_INLINE constexpr bool operator ==(const StringView& other) const {
                return compare(other) == 0;
            }

            WIZ_FORCE_INLINE constexpr bool operator !=(const StringView& other) const {
                return compare(other) != 0;
            }

            WIZ_FORCE_INLINE constexpr bool operator <(const StringView& other) const {
                return compare(other) < 0;
            }

            WIZ_FORCE_INLINE constexpr bool operator >(const StringView& other) const {
                return compare(other) > 0;
            }

            WIZ_FORCE_INLINE constexpr bool operator <=(const StringView& other) const {
                return compare(other) <= 0;
            }

            WIZ_FORCE_INLINE constexpr bool operator >=(const StringView& other) const {
                return compare(other) >= 0;
            }

        private:
            const char* data;
            std::size_t length;
    };    
}

constexpr wiz::StringView operator "" _sv(const char* text, std::size_t length) {
    return wiz::StringView(text, length);
}

namespace std {
    template <> struct hash<wiz::StringView> {
        std::size_t operator ()(const wiz::StringView& value) const {
            // Uses djb2 algorithm.
            // Algorithm could be replaced if better performance / less collisions needed.
            std::size_t result = 5381;

            const auto data = value.getData();
            const auto length = value.getLength();
            for (std::size_t i = 0; i != length; ++i) {
                result = (result << 5) + result + data[i];
            }
            return result;
        }
    };
}

#endif
