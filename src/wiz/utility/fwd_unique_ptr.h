#ifndef WIZ_UTILITY_FWD_UNIQUE_PTR_H
#define WIZ_UTILITY_FWD_UNIQUE_PTR_H

#include <memory>
#include <utility>
#include <type_traits>

#include <wiz/utility/unique_ptr.h>

namespace wiz {
    template <typename T>
    struct FwdDeleter {        
        void operator()(const T* ptr);
    };

    // Unique pointer type that can use an forwarded/incomplete type declaration.
    // Requires the type to define an explicit full specialization of FwdDeleter<T> to work.
    //
    // Usually this is the implementation:
    // template<> void FwdDeleter<T>::operator()(const T* ptr) { delete ptr; }
    template <typename T>
    using FwdUniquePtr = UniquePtr<T, FwdDeleter<std::remove_cv_t<T>>>;
    //using FwdUniquePtr = std::unique_ptr<T, FwdDeleter<std::remove_cv_t<T>>>;

    template <typename T, typename... Args>
    FwdUniquePtr<T> makeFwdUnique(Args&&... args) {
        return FwdUniquePtr<T>(new T(std::forward<Args>(args)...));
    }
}

#endif