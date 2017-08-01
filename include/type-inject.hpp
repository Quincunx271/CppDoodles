#pragma once

#include <metal.hpp>

namespace muse {
    template <std::size_t I, template <typename...> class InjectClass, typename... Args>
    struct inject
    {
        template <typename... Rest>
        using type = metal::apply<
            metal::lambda<InjectClass>,
            metal::join<
                metal::take<metal::list<Rest...>, metal::number<I>>,
                metal::list<Args...>,
                metal::drop<metal::list<Rest...>, metal::number<I>>>>;
    };

    template <template <typename...> class InjectClass, typename... Args>
    struct inject<0, InjectClass, Args...>
    {
        template <typename... Rest>
        using type = InjectClass<Args..., Rest...>;
    };

    template <template <typename...> class TClass>
    struct twrap
    {};

    template <typename MetalMap>
    struct injector_impl
    {
        template <template <typename...> class Type, typename... Args>
        using type = typename metal::at_key<MetalMap, twrap<Type>>::template type<Args...>;
    };

    template <typename... Pairs>
    using injector = injector_impl<metal::map<Pairs...>>;

    template <std::size_t I, template <typename...> class InjectClass, typename... Args>
    using injection = metal::pair<twrap<InjectClass>, inject<I, InjectClass, Args...>>;

    template <typename bank, template <typename...> class InjectClass, typename... Args>
    using inject_bank = typename bank::template type<InjectClass, Args...>;
}

// example usage:
// template <typename A, typename B, typename C, typename D>
// struct Test
// {};

// using registry = muse::injector<
//     muse::injection<1, Test, int, float>,
//     muse::injection<1, std::vector, std::allocator<int>>,
//     muse::injection<3, std::tuple, int, int, double, long, long long>
// >;

// int main () {
//     // Test<float, int, float, short>
//     muse::inject_bank<registry, Test, float, short>::hello();
//     // std::vector<int, std::allocator<int>>
//     muse::inject_bank<registry, std::vector, int>::hello();
//     // std::tuple<float, short, double, int, int, double, long, long long, double>
//     muse::inject_bank<registry, std::tuple, float, short, double, double>::hello();
// }
