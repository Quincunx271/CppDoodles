#pragma once

// This doodle has been pulled out into a proper repo: https://github.com/Quincunx271/TerseLambda

#include <utility>

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/seq/fold_left.hpp>
#include <boost/preprocessor/seq/to_tuple.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#ifndef TL_MAX_PARAMS
#define TL_MAX_PARAMS 8
#endif

#define TL_DETAIL_MAX_PARAMS_P1 BOOST_PP_INC(TL_MAX_PARAMS)

#define TL_DETAIL_NUM_SEQ                                                      \
    BOOST_PP_VARIADIC_TO_SEQ(                                                  \
        BOOST_PP_ENUM_SHIFTED_PARAMS(TL_DETAIL_MAX_PARAMS_P1, ))

#define TL_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

#define TL_DETAIL_CREATE_ARG_N(s, state, elem)                                 \
    state;                                                                     \
    [[maybe_unused]]                                                           \
    auto&& BOOST_PP_CAT(_, elem) = ::tl::detail::nth<elem - 1>(TL_FWD(_args)...);

#define TL_DETAIL_ARGS                                                         \
    BOOST_PP_SEQ_FOLD_LEFT(TL_DETAIL_CREATE_ARG_N, , TL_DETAIL_NUM_SEQ)

#define TL(...)                                                                \
    (auto&&... _args)->decltype(auto)                                          \
    {                                                                          \
        TL_DETAIL_ARGS;                                                        \
        return __VA_ARGS__;                                                    \
    }

namespace tl::detail {
    struct not_a_parameter
    {};

    template <int N, typename T, typename... Ts>
    constexpr decltype(auto) nth_impl(T&& t, Ts&&... ts)
    {
        if constexpr (N == 0) {
            return TL_FWD(t);
        } else {
            return tl::detail::nth_impl<N - 1>(TL_FWD(ts)...);
        }
    }

    template <int N, typename... Ts>
    constexpr decltype(auto) nth(Ts&&... ts)
    {
        if constexpr (N < sizeof...(Ts)) {
            return tl::detail::nth_impl<N>(TL_FWD(ts)...);
        } else {
            return not_a_parameter{};
        }
    }
}

/*
struct foo
{
    int value;
};

int main()
{
    return [] TL(_1.value + 3)(foo{42});
}
*/
