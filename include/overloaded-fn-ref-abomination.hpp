#include <functional>
#include <type_traits>
#include <utility>

#include <boost/hana.hpp>

// Function reference. Similar to what I'd expect `[]someFunction` to do
// if the proposal goes through. The intention of this is to lift an
// overload set into a lambda. Calling `FREF(std::foo)` acts as if you called
// `std::foo` directly
#define FREF(...) \
    [](auto&&... args) \
        noexcept(noexcept( \
            __VA_ARGS__(std::forward<decltype(args)>(args)...) \
        )) -> decltype( \
            __VA_ARGS__(std::forward<decltype(args)>(args)...) \
        ) { return \
            __VA_ARGS__(std::forward<decltype(args)>(args)...) \
        ;}

namespace ns {
    template <typename T>
    struct is_reference_wrapper
        : std::false_type
    {};
    template <typename... Ts>
    struct is_reference_wrapper<std::reference_wrapper<Ts...>>
        : std::true_type
    {};
}

// Lift member function overload sets into lambdas, with `std::invoke`-like behaviour. The problem
// is that we can't use `std::invoke` because our function can't be a function object (overloads).
//
// Calling syntax: stuff like `MEM_FREF(std::string::size)` is valid because
// `myString.std::string::size()` is valid, but it probably doesn't work for data members -
// I'm not sure if `myString.my::namespace_::size` would work.
//
// For templates, it needs the `template` keyword: `MEM_FREF(std::template vector<int>)`.
// Better would be `MEM_FREF(size)` for both; it works for any type that has a `.size()` member
// or even just `.size` member
//
// Not SFINAE friendly or noexcept-aware, as lambdas can't be in unevaluated contexts.
#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

#define CALL_FWD_ARGS (FWD(args)...)

#define MEM_FREF_CALL_NORMAL(CALL, ...)                                                     \
    [](auto&& arg0, auto&&... args)                                                         \
           noexcept(noexcept(FWD(arg0).__VA_ARGS__ CALL))                                   \
        -> decltype(FWD(arg0).__VA_ARGS__ CALL)                                             \
    {                                                                                       \
        return FWD(arg0).__VA_ARGS__ CALL;                                                  \
    }

#define MEM_FREF_CALL_REF_WRAP(CALL, ...)                                                   \
    [](auto&& arg0, auto&&... args)                                                         \
           noexcept(noexcept(arg0.get().__VA_ARGS__ CALL))                                  \
        -> std::enable_if_t<                                                                \
               ns::is_reference_wrapper<std::decay_t<decltype(arg0)>>::value,               \
               decltype(arg0.get().__VA_ARGS__ CALL)                                        \
           >                                                                                \
    {                                                                                       \
        return arg0.get().__VA_ARGS__ CALL;                                                 \
    }

#define MEM_FREF_CALL_DEREF(CALL, ...)                                                      \
    [](auto&& arg0, auto&&... args)                                                         \
           noexcept(noexcept((*FWD(arg0)).__VA_ARGS__ CALL))                                \
        -> decltype((*FWD(arg0)).__VA_ARGS__ CALL)                                          \
    {                                                                                       \
        return (*FWD(arg0)).__VA_ARGS__ CALL;                                               \
    }

#define MEM_FREF(...)                                                                       \
    [](auto&&... args)                                                                      \
        /* Not allowed; lambda in unevaluated context (maybe C++20?) */                     \
        /* noexcept(noexcept(                                                               \
            ::boost::hana::overload_linearly(                                               \
                MEM_FREF_CALL_NORMAL(CALL_FWD_ARGS, __VA_ARGS__),                           \
                MEM_FREF_CALL_REF_WRAP(CALL_FWD_ARGS, __VA_ARGS__),                         \
                MEM_FREF_CALL_DEREF(CALL_FWD_ARGS, __VA_ARGS__),                            \
                MEM_FREF_CALL_NORMAL(, __VA_ARGS__),                                        \
                MEM_FREF_CALL_DEREF(, __VA_ARGS__)                                          \
            )(FWD(args)...)                                                                 \
        ))                                                                                  \
        -> decltype(                                                                        \
            ::boost::hana::overload_linearly(                                               \
                MEM_FREF_CALL_NORMAL(CALL_FWD_ARGS, __VA_ARGS__),                           \
                MEM_FREF_CALL_REF_WRAP(CALL_FWD_ARGS, __VA_ARGS__),                         \
                MEM_FREF_CALL_DEREF(CALL_FWD_ARGS, __VA_ARGS__),                            \
                MEM_FREF_CALL_NORMAL(, __VA_ARGS__),                                        \
                MEM_FREF_CALL_DEREF(, __VA_ARGS__)                                          \
            )(FWD(args)...)                                                                 \
        ) */                                                                                \
    {                                                                                       \
        return ::boost::hana::overload_linearly(                                            \
            MEM_FREF_CALL_NORMAL(CALL_FWD_ARGS, __VA_ARGS__),                               \
            MEM_FREF_CALL_REF_WRAP(CALL_FWD_ARGS, __VA_ARGS__),                             \
            MEM_FREF_CALL_DEREF(CALL_FWD_ARGS, __VA_ARGS__),                                \
            MEM_FREF_CALL_NORMAL(, __VA_ARGS__),                                            \
            MEM_FREF_CALL_REF_WRAP(, __VA_ARGS__),                                          \
            MEM_FREF_CALL_DEREF(, __VA_ARGS__)                                              \
        )(FWD(args)...);                                                                    \
    }

// #include <algorithm>
// #include <string>
// #include <initializer_list>
// #include <vector>
// #include <array>

// int main() {
//     auto f = MEM_FREF(size);
//     auto myMin = FREF(std::min);
//
//     std::string test = "Hello, world!\n";     // size() == 14
//     std::array<int, 5> foo = {1, 2, 3, 4, 5}; // size() == 5
//     return f(test) + f(foo)                                // 14 + 5 = 19
//         + MEM_FREF(std::string::size)(test)                // + 14
//         + MEM_FREF(std::template array<int, 5>::size)(foo) // + 5 = 38
//         + myMin(1, 2)                                      // + 1
//         + myMin(std::initializer_list<int>{1, 2});         // + 1 = 40
// }
