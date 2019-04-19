#ifndef WIZ_UTILITY_ARRAY_VIEW_H
#define WIZ_UTILITY_ARRAY_VIEW_H

#include <functional>
#include <cstddef>
#include <vector>
#include <type_traits>

namespace wiz {
    template <typename T>
    class ArrayView {
        public:
            ArrayView()
            : data(nullptr), length(0) {}

            ArrayView(const ArrayView<T>& other)
            : data(other.data), length(other.length) {}

            ArrayView(std::add_pointer_t<const T> data, std::size_t length)
            : data(data), length(length) {}

            ArrayView(const std::vector<T>& other)
            : data(other.data()), length(other.size()) {}

            std::add_pointer_t<const T> getData() const {
                return data;
            }

            std::size_t getLength() const {
                return length;
            }

            std::add_pointer_t<const T> begin() const {
                return data;
            }

            std::add_pointer_t<const T> end() const {
                return data + length;
            }

            int compare(const ArrayView<T>& other) const {
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

            ArrayView<T> sub(std::size_t index, std::size_t size = SIZE_MAX) const {
                if (index > length) {
                    index = length;
                }
                if (size > length - index) {
                    size = length - index;
                }
                return ArrayView<T>(data + index, size);
            }

            ArrayView<T>& operator=(const ArrayView<T>& other) {
                data = other.data;
                length = other.length;
                return *this;
            }

            const T& operator [](std::size_t index) const {
                return data[index];
            }

            bool operator ==(const ArrayView<T>& other) const {
                return compare(other) == 0;
            }

            bool operator !=(const ArrayView<T>& other) const {
                return compare(other) != 0;
            }

            bool operator <(const ArrayView<T>& other) const {
                return compare(other) < 0;
            }

            bool operator >(const ArrayView<T>& other) const {
                return compare(other) > 0;
            }

            bool operator <=(const ArrayView<T>& other) const {
                return compare(other) <= 0;
            }

            bool operator >=(const ArrayView<T>& other) const {
                return compare(other) >= 0;
            }

        private:
            std::add_pointer_t<const T> data;
            std::size_t length;
    };
}

namespace std {
    template <typename T>
    struct hash<wiz::ArrayView<T>> {
        std::size_t operator ()(const wiz::ArrayView<T>& value) const {
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
