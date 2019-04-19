#ifndef WIZ_UTILITY_VARIANT_H
#define WIZ_UTILITY_VARIANT_H

#include <cstddef>
#include <cassert>
#include <type_traits>
#include <utility>
#include <tuple>

namespace wiz {
    namespace detail {
#if __cplusplus >= 201703L || _MSC_VER >= 1910
        template <typename F, typename... Args>
        using InvokeResultT = std::invoke_result_t<F, Args...>;
#else
        template <typename F, typename... Args>
        using InvokeResultT = std::result_of_t<F(Args...)>;
#endif

        template <std::size_t... Ns>
        struct MaxValue;

        template <>
        struct MaxValue<> {
            static constexpr std::size_t value = 0;
        };

        template <std::size_t N, std::size_t... Ns>
        struct MaxValue<N, Ns...> {
            static constexpr std::size_t value = MaxValue<Ns...>::value > N
                ? MaxValue<Ns...>::value
                : N;
        };



        template <typename... Ts>
        struct CommonType {};

        template <typename T>
        struct CommonType<T> {
            using type = std::decay_t<T>;
        };

        template <typename T, typename U>
        struct CommonType<T, U> {
            using type = std::decay_t<decltype(true ? std::declval<T>() : std::declval<U>())>;
        };

        template <typename T, typename U, typename... Ts>
        struct CommonType<T, U, Ts...> {
            using type = typename CommonType<typename CommonType<T, U>::type, Ts...>::type;
        };

        template <typename... Ts>
        using CommonTypeT = typename CommonType<Ts...>::type;



        template <typename U, typename... Ts>
        struct TypeIndexOf;

        template <typename U>
        struct TypeIndexOf<U> {
            static constexpr int value = -1;
        };

        template <typename U, typename... Ts>
        struct TypeIndexOf<U, U, Ts...> {
            static constexpr int value = 0;
        };

        template <typename U, typename T, typename... Ts>
        struct TypeIndexOf<U, T, Ts...> {
            static constexpr int value = TypeIndexOf<U, Ts...>::value >= 0
                ? TypeIndexOf<U, Ts...>::value + 1
                : -1;
        };

        template <typename U, typename... Ts>
        constexpr int checkedTypeIndexOf() {
            static_assert(TypeIndexOf<U, Ts...>::value >= 0, "Variant does not support the provided type.");
            return TypeIndexOf<U, Ts...>::value;
        }



        template <typename T>
        void destroyVariantKind(void* data) {
            reinterpret_cast<std::add_pointer_t<T>>(data)->~T();
        }

        template <typename... Ts>
        void destroyVariant(int tag, void* data) {
            assert(0 <= tag && tag < static_cast<int>(sizeof...(Ts)));
            using Table = void(*)(void*);
            static const Table table[] = {
                destroyVariantKind<Ts>...
            };
            table[tag](data);
        }

        template <typename T>
        void copyVariantKind(void* dest, const void* src) {
            new (dest) T(*reinterpret_cast<std::add_pointer_t<const T>>(src));        
        }

        template <typename... Ts>
        void copyVariant(int tag, void* dest, const void* src) {
            assert(0 <= tag && tag < static_cast<int>(sizeof...(Ts)));
            using Table = void(*)(void*, const void*);
            static const Table table[] = {
                copyVariantKind<Ts>...
            };
            table[tag](dest, src);
        }

        template <typename T>
        void moveVariantKind(void* dest, void* src) {
            new (dest) T(std::move(*reinterpret_cast<std::add_pointer_t<T>>(src)));
        }

        template <typename... Ts>
        void moveVariant(int tag, void* dest, void* src) {
            assert(0 <= tag && tag < static_cast<int>(sizeof...(Ts)));
            using Table = void(*)(void*, void*);
            static const Table table[] = {
                moveVariantKind<Ts>...
            };
            table[tag](dest, src);
        }

        template <typename F, typename T>
        auto visitVariantKind(F&& f, const void* data)
        -> InvokeResultT<F&&, const T&> {
            return std::forward<F>(f)(*reinterpret_cast<std::add_pointer_t<const T>>(data));
        }

        template <typename F, typename... Ts>
        auto visitVariant(int tag, F&& f, const void* data) {
            assert(0 <= tag && tag < static_cast<int>(sizeof...(Ts)));
            using Result = CommonTypeT<
                InvokeResultT<F&&, const Ts&>...
                >;
            using Table = Result(*)(F&&, const void*);
            static const Table table[] = {
                visitVariantKind<F, Ts>...
            };
            return table[tag](std::forward<F>(f), data);
        }

        template <typename F, typename UserdataType, typename T>
        auto visitVariantKindWithUserdata(F&& f, UserdataType&& userdata, const void* data)
        -> InvokeResultT<F&&, UserdataType&&, const T&> {
            return std::forward<F>(f)(std::forward<UserdataType>(userdata), *reinterpret_cast<std::add_pointer_t<const T>>(data));
        }

        template <typename F, typename UserdataType, typename... Ts>
        auto visitVariantWithUserdata(int tag, F&& f, UserdataType&& userdata, const void* data) {
            assert(0 <= tag && tag < static_cast<int>(sizeof...(Ts)));
            using Result = CommonTypeT<
                InvokeResultT<F&&, UserdataType&&, const Ts&>...
                >;
            using Table = Result(*)(F&&, UserdataType&&, const void*);
            static const Table table[] = {
                visitVariantKindWithUserdata<F, UserdataType, Ts>...
            };
            return table[tag](std::forward<F>(f), std::forward<UserdataType>(userdata), data);
        }
    }

    template <typename... Ts>
    class Variant {
        public:
            Variant() = delete;
           
            template <typename U>
            Variant(const U& value)
            : tag(typeIndexOf<U>()) {
                new(&data) U(value);
            }

            template <typename U,
                std::enable_if_t<!std::is_same<std::decay_t<U>, Variant>::value>* = nullptr,
                std::enable_if_t<!std::is_lvalue_reference<U>::value>* = nullptr
            >
            Variant(U&& value)
            : tag(typeIndexOf<U>()) {
                new(&data) U(std::forward<U>(value));
            }

            Variant(const Variant& other)
            : tag(other.tag) {
                assert(0 <= other.tag && other.tag < static_cast<int>(sizeof...(Ts)));
                detail::copyVariant<Ts...>(tag, &data, &other.data);
            }

            Variant(Variant&& other)
            : tag(other.tag) {
                assert(0 <= other.tag && other.tag < static_cast<int>(sizeof...(Ts)));
                detail::moveVariant<Ts...>(tag, &data, &other.data);
            }

            ~Variant() {
                detail::destroyVariant<Ts...>(tag, &data);
            }

            Variant& operator =(const Variant& other) {
                if (this != &other) {
                    assert(0 <= other.tag && other.tag < static_cast<int>(sizeof...(Ts)));
                    detail::destroyVariant<Ts...>(tag, &data);
                    tag = other.tag;
                    detail::copyVariant<Ts...>(tag, &data, &other.data);
                }
                return *this;
            }

            Variant& operator =(Variant&& other) {
                if (this != &other) {
                    assert(0 <= other.tag && other.tag < static_cast<int>(sizeof...(Ts)));
                    detail::destroyVariant<Ts...>(tag, &data);
                    tag = other.tag;
                    detail::moveVariant<Ts...>(tag, &data, &other.data);
                }
                return *this;
            }

            int index() const {
                return tag;
            }

            template <typename U>
            bool is() const {
                return typeIndexOf<U>() == tag;
            }

            template <typename U>
            U& get() & {
                static_cast<void>(typeIndexOf<U>());
                assert(is<U>());
                return *reinterpret_cast<std::add_pointer_t<U>>(&data);
            }

            template <typename U>
            const U& get() const & {
                static_cast<void>(typeIndexOf<U>());
                assert(is<U>());
                return *reinterpret_cast<std::add_pointer_t<const U>>(&data);
            }

            template <typename U>
            U&& get() && {
                static_cast<void>(typeIndexOf<U>());
                assert(is<U>());
                return std::move(*reinterpret_cast<std::add_pointer_t<U>>(&data));
            }

            template <typename U>
            const U&& get() const && {
                static_cast<void>(typeIndexOf<U>());
                assert(is<U>());
                return std::move(*reinterpret_cast<std::add_pointer_t<const U>>(&data));
            }

            template <typename U>
            std::add_pointer_t<U> tryGet() {
                return is<U>() ? &get<U>() : nullptr;
            }

            template <typename U>
            std::add_pointer_t<const U> tryGet() const {
                return is<U>() ? &get<U>() : nullptr;
            }

            template <typename F>
            auto visit(F&& f) const {
                return detail::visitVariant<F, Ts...>(tag, std::forward<F>(f), &data);
            }

            template <typename F, typename UserdataType>
            auto visitWithUserdata(UserdataType&& userdata, F&& f) const {
                return detail::visitVariantWithUserdata<F, UserdataType, Ts...>(tag, std::forward<F>(f), std::forward<UserdataType>(userdata), &data);
            }

            template <typename U>
            static constexpr int typeIndexOf() {
                return detail::checkedTypeIndexOf<U, Ts...>();
            }
        private:
            using DataType = std::aligned_storage_t<
                detail::MaxValue<sizeof(Ts)...>::value,
                detail::MaxValue<alignof(Ts)...>::value>;

            int tag;
            DataType data;
    };
}

#endif
