#include <functional>
#include <type_traits>
#include <utility>

#include <boost/hana/type.hpp>

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
    template <class T>
    struct is_reference_wrapper
        : std::false_type
    {};
    template <class U>
    struct is_reference_wrapper<std::reference_wrapper<U>>
        : std::true_type
    {};

    template <typename HanaType>
    auto declval(HanaType type)
        -> typename HanaType::type&&;
}

// Lift member function overload sets into lambdas, with `std::invoke`-like behaviour. The problem
// is that we can't use `std::invoke` because our function can't be a function object (overloads).
// Current defect: no support for just member data pointers
//
// Calling syntax: stuff like `MEM_FREF(std::string::size)` is valid because
// `myString.std::string::size()` is valid.
// For templates, it needs the `template` keyword: `MEM_FREF(std::template vector<int>)`.
// Better would be `MEM_FREF(size)` for both; it works for any type that has a `.size()` member
//
// Not SFINAE friendly or noexcept-aware. The code bloat for implementing those
// would be quite large.
//
// Also note that `call_ref_wrapper` and `call_ptr` can be implemented in terms of `call_normal`,
// but it's not much better
#define MEM_FREF(...)                                                                               \
    [](auto&&... args) {                                                                            \
        constexpr auto call_normal =                                                                \
            [](auto&& arg0, auto&&... args)                                                         \
                -> decltype(std::forward<decltype(arg0)>(arg0)                                      \
                                .__VA_ARGS__(std::forward<decltype(args)>(args)...))                \
            {                                                                                       \
                return std::forward<decltype(arg0)>(arg0)                                           \
                                .__VA_ARGS__(std::forward<decltype(args)>(args)...);                \
            };                                                                                      \
        constexpr auto can_call_normal = ::boost::hana::is_valid(                                   \
            [](auto arg0, auto... args)                                                             \
                -> std::void_t<decltype(ns::declval(arg0).__VA_ARGS__(ns::declval(args)...))>       \
            {});                                                                                    \
                                                                                                    \
        constexpr auto call_ref_wrapper =                                                           \
            [](auto&& arg0, auto&&... args)                                                         \
                -> decltype(arg0.get().__VA_ARGS__(std::forward<decltype(args)>(args)...))          \
            {                                                                                       \
                return arg0.get().__VA_ARGS__(std::forward<decltype(args)>(args)...);               \
            };                                                                                      \
        constexpr auto can_call_ref_wrapper =                                                       \
            [](auto arg0, auto...) {                                                                \
                return ns::is_reference_wrapper<std::decay_t<typename decltype(arg0)::type>>::value;\
            };                                                                                      \
                                                                                                    \
        constexpr auto call_ptr =                                                                   \
            [](auto&& arg0, auto&&... args) {                                                       \
                return (*arg0).__VA_ARGS__(std::forward<decltype(args)>(args)...);                  \
            };                                                                                      \
                                                                                                    \
        /* late so that it doesn't catch anything in the __VA_ARGS__ */                             \
        namespace hana = boost::hana;                                                               \
                                                                                                    \
        if constexpr (                                                                              \
                can_call_normal(hana::type_c<decltype(args)>...)) {                                 \
            return call_normal(std::forward<decltype(args)>(args)...);                              \
        } else if constexpr(can_call_ref_wrapper(hana::type_c<decltype(args)>...)) {                \
            return call_ref_wrapper(std::forward<decltype(args)>(args)...);                         \
        } else {                                                                                    \
            return call_ptr(std::forward<decltype(args)>(args)...);                                 \
        }                                                                                           \
    }

// #include <algorithm>
// #include <string>
// #include <initializer_list>
// #include <vector>

// int main() {
//     auto f = MEM_FREF(size);
//     auto myMin = FREF(std::min);

//     std::string test = "Hello, world!\n"; // size() == 14
//     std::array foo = {1, 2, 3, 4, 5};     // size() == 5
//     return f(test) + f(foo)                                // 14 + 5 = 19
//         + MEM_FREF(std::string::size)(test)                // + 14
//         + MEM_FREF(std::template array<int, 5>::size)(foo) // + 5 = 38
//         + myMin(1, 2)                                      // + 1
//         + myMin(std::initializer_list<int>{1, 2});         // + 1 = 40
// }
