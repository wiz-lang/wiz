#ifndef WIZ_UTILITY_MISC_H
#define WIZ_UTILITY_MISC_H

#include <vector>
#include <utility>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    template <typename T>
    void moveAppendAll(std::vector<T>&) {}

    template <typename T, typename Arg, typename... Args>
    void moveAppendAll(std::vector<T>& vec, Arg&& arg, Args&&... args) {
        vec.push_back(std::move(arg));
        moveAppendAll(vec, args...);
    }

    template <typename T, typename... Ts>
    std::vector<T> makeMoveVector(T&& arg, Ts&&... args) {
        std::vector<T> result;
        moveAppendAll(result, arg, args...);
        return std::move(result);
    }

    template <typename SmartPtr>
    void extendNonOwningVector(std::vector<const typename SmartPtr::element_type*>& dest, const std::vector<SmartPtr>& source) {
        const auto sourceSize = source.size();
        dest.reserve(dest.size() + sourceSize);
        for (std::size_t i = 0; i != sourceSize; ++i) {
            dest.push_back(source[i].get());
        }
    }

    template <typename SmartPtr>
    void replaceNonOwningVector(std::vector<const typename SmartPtr::element_type*>& dest, const std::vector<SmartPtr>& source) {
        dest.clear();
        extendNonOwningVector(dest, source);
    }

    template <typename SmartPtr>
    std::vector<const typename SmartPtr::element_type*> makeNonOwningVector(const std::vector<SmartPtr>& items) {
        std::vector<const typename SmartPtr::element_type*> result;
        result.reserve(items.size());
        extendNonOwningVector(result, items);
        return result;
    }

    std::size_t log2(std::size_t value);
}

#endif