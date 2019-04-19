#ifndef WIZ_UTILITY_OVERLOAD_H
#define WIZ_UTILITY_OVERLOAD_H

namespace wiz {
    template <typename... Fs>
    struct Overload;

    template <typename F>
    struct Overload<F> {
        public:
            Overload(F&& f) : f(std::forward<F>(f)) {}
            
            template <typename... Ts>
            auto operator()(Ts&&... args) const
            -> decltype(std::declval<F>()(std::forward<Ts>(args)...)) {
                return f(std::forward<Ts>(args)...);
            }

        private:
            F f;
    };

    template <typename F, typename... Fs>
    struct Overload<F, Fs...> : Overload<F>, Overload<Fs...> {
        using Overload<F>::operator();
        using Overload<Fs...>::operator();

        Overload(F&& f, Fs&&... fs) :
            Overload<F>(std::forward<F>(f)),
            Overload<Fs...>(std::forward<Fs>(fs)...) {}
    };

    template <typename... Fs>
    auto makeOverload(Fs... fs) {
        return Overload<Fs...>(std::forward<Fs>(fs)...);
    }
}

#endif