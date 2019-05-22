#ifndef WIZ_UNIQUE_PTR_H
#define WIZ_UNIQUE_PTR_H

#include <cstddef>
#include <type_traits>

#include <wiz/utility/macros.h>

namespace wiz {
    template <typename T>
    struct DefaultDeleter {
        WIZ_FORCE_INLINE void operator ()(const T* ptr) const {
            static_assert(sizeof(T) > 0, "T cannot be an incomplete type");
            delete ptr;
        }
    };

    template <typename T, typename Deleter, bool = std::is_empty<Deleter>() && !std::is_final<Deleter>()>
    class UniquePtrBase : private Deleter {
        public:
            using PointerType = std::add_pointer_t<T>;

            template<typename CompatiblePointerType>
            UniquePtrBase(CompatiblePointerType ptr)
            : ptr(ptr) {}

	        template<typename CompatiblePointerType, typename CompatibleDeleterType>
            UniquePtrBase(CompatiblePointerType ptr, CompatibleDeleterType&&)
            : ptr(ptr) {}

            WIZ_FORCE_INLINE PointerType& pointer() {
                return ptr;
            }

            WIZ_FORCE_INLINE const PointerType& pointer() const {
                return ptr;
            }

            WIZ_FORCE_INLINE Deleter& deleter() {
                return *this;
            }

            WIZ_FORCE_INLINE const Deleter& deleter() const {
                return *this;
            }

        private:
            PointerType ptr;
    };

    template <typename T, typename Deleter>
    class UniquePtrBase<T, Deleter, false> {
        public:
            using PointerType = std::add_pointer_t<T>;

            template<typename CompatiblePointerType>
            UniquePtrBase(CompatiblePointerType ptr)
            : ptr(ptr), del() {}

	        template<typename CompatiblePointerType, typename CompatibleDeleterType>
            UniquePtrBase(CompatiblePointerType ptr, CompatibleDeleterType&& del)
            : ptr(ptr), del(std::forward<CompatibleDeleterType>(del)) {}

            WIZ_FORCE_INLINE PointerType& pointer() {
                return ptr;
            }

            WIZ_FORCE_INLINE const PointerType& pointer() const {
                return ptr;
            }

            WIZ_FORCE_INLINE Deleter& deleter() {
                return del;
            }

            WIZ_FORCE_INLINE const Deleter& deleter() const {
                return del;
            }

        private:
            PointerType ptr;
            Deleter del;
    };

    // A subset of what std::unique_ptr supports, with the intent of better inlining.
    template <typename T, typename Deleter = DefaultDeleter<T>>
    class UniquePtr : public UniquePtrBase<T, Deleter> {
        public:
            using PointerType = typename UniquePtrBase<T, Deleter>::PointerType;
            using ElementType = T;
            using DeleterType = Deleter;

            UniquePtr()
            : UniquePtrBase<T, Deleter>(nullptr) {}

            UniquePtr(std::nullptr_t)
            : UniquePtrBase<T, Deleter>(nullptr) {}

            UniquePtr(PointerType ptr)
            : UniquePtrBase<T, Deleter>(ptr) {}

            UniquePtr(PointerType ptr, const DeleterType& del)
            : UniquePtrBase<T, Deleter>(ptr, del) {}

            UniquePtr(PointerType ptr, DeleterType&& del)
            : UniquePtrBase<T, Deleter>(ptr, std::move(del)) {}

            UniquePtr(const UniquePtr&) = delete;

            UniquePtr(UniquePtr&& other) 
            : UniquePtrBase<T, Deleter>(other.release(), std::forward<Deleter>(other.deleter())) {}

        	template<typename CompatiblePointerType, typename CompatibleDeleterType>
            UniquePtr(UniquePtr<CompatiblePointerType, CompatibleDeleterType>&& other)
            : UniquePtrBase<T, Deleter>(other.release(), std::forward<CompatibleDeleterType>(other.deleter())) {}

            ~UniquePtr() {
                this->deleter()(this->pointer());
            }

            UniquePtr& operator =(const UniquePtr&) = delete;

            UniquePtr& operator =(std::nullptr_t) {
                this->pointer() = nullptr;
                return *this;
            }

            UniquePtr& operator =(UniquePtr&& other) {
                if (this != &other) {
                    this->pointer() = other.release();
                    this->deleter() = std::forward<Deleter>(other.deleter());
                }
                return *this;
            }

            WIZ_FORCE_INLINE explicit operator bool() const {
                return this->pointer() != nullptr;
            }

            WIZ_FORCE_INLINE PointerType operator ->() const {
                return this->pointer();
            }

            WIZ_FORCE_INLINE T& operator *() const {
                return *this->pointer();
            }

            WIZ_FORCE_INLINE void reset() {
                this->pointer() = nullptr;
            }

            WIZ_FORCE_INLINE PointerType release() {
                const auto result = this->pointer();
                this->pointer() = nullptr;
                return result;
            }

            WIZ_FORCE_INLINE PointerType get() const {
                return this->pointer();
            }

            WIZ_FORCE_INLINE bool operator ==(const UniquePtr& other) const {
                return this->pointer() == other.pointer();
            }

            WIZ_FORCE_INLINE bool operator !=(const UniquePtr& other) const {
                return this->pointer() != other.pointer();
            }

            WIZ_FORCE_INLINE bool operator <(const UniquePtr& other) const {
                return this->pointer() < other.pointer();
            }

            WIZ_FORCE_INLINE bool operator >(const UniquePtr& other) const {
                return this->pointer() > other.pointer();
            }

            WIZ_FORCE_INLINE bool operator <=(const UniquePtr& other) const {
                return this->pointer() <= other.pointer();
            }

            WIZ_FORCE_INLINE bool operator >=(const UniquePtr& other) const {
                return this->pointer() >= other.pointer();
            }

    };
}

namespace std {
    template <typename T, typename Deleter>
    struct hash<wiz::UniquePtr<T, Deleter>> {
        WIZ_FORCE_INLINE std::size_t operator()(const wiz::UniquePtr<T, Deleter>& ptr) const {
            return std::hash<typename wiz::UniquePtr<T, Deleter>::PointerType>();
        }
    };
}

#endif
