#ifndef WIZ_UTILITY_SCOPE_GUARD_H
#define WIZ_UTILITY_SCOPE_GUARD_H

namespace wiz {
    template<typename T>
    class ScopeGuard {
        public:
            ScopeGuard() = delete;

            ScopeGuard(T finalizer)
            : active(true),
            finalizer(finalizer) {}

            ScopeGuard(const ScopeGuard&) = delete;

            ScopeGuard(ScopeGuard&& other)
            : active(std::move(other.active)),
            finalizer(std::move(other.finalizer)) {}

            ~ScopeGuard() {
                if (active) {
                    finalizer();
                }
            }

            ScopeGuard& operator=(const ScopeGuard&) = delete;

            ScopeGuard& operator=(ScopeGuard&& other) {
                if (this != &other) {
                    if (active) {
                        finalizer();
                    }
                    active = std::move(other.active);
                    finalizer = std::move(other.finalizer);
                }
                return *this;
            }

            operator bool() const {
                return true;
            }

            bool isActive() const {
                return active;
            }

            void activate() {
                active = true;
            }

            void deactivate() {
                active = false;
            }

        private:
            bool active;
            T finalizer;
    };

    template<typename T>
    ScopeGuard<T> makeScopeGuard(T&& finalizer) {
        return ScopeGuard<T>(std::forward<T>(finalizer));
    }
}

#endif