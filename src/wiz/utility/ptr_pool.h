#ifndef WIZ_UTILITY_INSTANCE_POOL_H
#define WIZ_UTILITY_INSTANCE_POOL_H

#include <vector>

#include <wiz/utility/array_view.h>
#include <wiz/utility/unique_ptr.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    template <typename T, typename Pointer>
    class PtrPoolBase {
        public:
            using RawPointer = std::add_pointer_t<T>;
            using RawConstPointer = std::add_pointer_t<const T>;

            RawPointer add(Pointer instance) {
                const auto result = instance.get();
                if (result != nullptr) {
                    instances_.push_back(std::move(instance));
                }
                return result;
            }

            WIZ_FORCE_INLINE void remove(std::size_t index) {
                instances_.erase(instances_.begin() + index);
            }

            WIZ_FORCE_INLINE void clear() {
                instances_.clear();
            }

            WIZ_FORCE_INLINE std::size_t size() const {
                return instances_.size();
            }

            WIZ_FORCE_INLINE ArrayView<Pointer> view() const {
                return ArrayView<Pointer>(instances_);
            }

            WIZ_FORCE_INLINE std::add_pointer_t<const Pointer> begin() const {
                return instances_.data();
            }

            WIZ_FORCE_INLINE std::add_pointer_t<const Pointer> end() const {
                return instances_.data() + instances_.size();
            }

            WIZ_FORCE_INLINE const Pointer& operator [](std::size_t index) const {
                return instances_[index];
            }

        private:
            std::vector<Pointer> instances_;
    };

    template <typename T>
    class PtrPool : public PtrPoolBase<T, UniquePtr<T>> {
        public:
            template <typename... Args>
            T* addNew(Args&&... args) {
                return this->add(makeUnique<T>(std::forward<Args>(args)...));
            }
    };

    template <typename T>
    class FwdPtrPool : public PtrPoolBase<T, FwdUniquePtr<T>> {
        public:
            template <typename... Args>
            T* addNew(Args&&... args) {
                return this->add(makeFwdUnique<T>(std::forward<Args>(args)...));
            }
    };
}

#endif