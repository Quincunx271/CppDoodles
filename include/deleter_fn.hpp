#include <cstddef>
#include <type_traits>
#include <utility>

template <typename R, typename... Args>
using fn_ptr = R(*)(Args...);

namespace detail {
    // GCC's concept-ts implementation. Not what's in C++20.
    // Works on gcc-7.3 with the command:
    // $ g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -fconcepts
    template <typename F, typename A, typename B = A>
    concept bool CommutativelyComparable = requires(F const f, A a, B b, A const ac, B const bc) {
        // f(x, y) && f(y, z) implies f(x, z)
        { f(a, b) } -> bool;
        { f(b, a) } -> bool;
        { f(ac, b) } -> bool;
        { f(b, ac) } -> bool;
        { f(a, bc) } -> bool;
        { f(bc, a) } -> bool;
        { f(ac, bc) } -> bool;
        { f(bc, ac) } -> bool;
    };

    struct eq {
        template <typename A, typename B>
        auto operator()(A&& a, B&& b) const
            -> decltype(std::forward<A>(a) == std::forward<B>(b))
        {
            return std::forward<A>(a) == std::forward<B>(b);
        }
    };

    struct neq {
        template <typename A, typename B>
        auto operator()(A&& a, B&& b) const
            -> decltype(std::forward<A>(a) != std::forward<B>(b))
        {
            return std::forward<A>(a) != std::forward<B>(b);
        }
    };

    template <typename A, typename B = A>
    concept bool EqualityComparable = CommutativelyComparable<eq, A, B>;

    template <typename A, typename B = A>
    concept bool InequalityComparable = CommutativelyComparable<neq, A, B>;

    template <typename T>
    concept bool NullablePointerGuts =
        EqualityComparable<T>
        && std::is_default_constructible_v<T> &&  std::is_copy_constructible_v<T>
        && std::is_copy_assignable_v<T>
        && std::is_destructible_v<T>;

    template <typename T>
    concept bool NullablePointer = NullablePointerGuts<T>
        && InequalityComparable<T>
        && EqualityComparable<T, std::nullptr_t> && InequalityComparable<T, std::nullptr_t>
        && std::is_constructible_v<T, std::nullptr_t>
        && std::is_assignable_v<T, std::nullptr_t>;

    template <NullablePointerGuts T>
    class handle {
        T handle_{};
    public:
        handle(std::nullptr_t = nullptr)
        {}
        handle(T handle)
            : handle_{std::move(handle)}
        {}

        T const& get() const { return handle_; }
        T& get() { return handle_; }
        
        explicit operator bool() const { return handle_; }

        friend bool operator==(handle lhs, handle rhs) { return lhs.handle_ == rhs.handle_; }
        
        friend bool operator!=(handle lhs, handle rhs)
        {
            if constexpr (InequalityComparable<T>) {
                return lhs.handle_ != rhs.handle_;
            } else {
                return !(lhs == rhs);
            }
        }
    };

    template <typename T>
    using compute_deleter_pointer_t = std::conditional_t<NullablePointer<T>,
        T,
        handle<T>
    >;
}

template <auto f>
class deleter_fn;

template <typename R, typename Arg, fn_ptr<R, Arg> f>
class deleter_fn<f> {
    using arg_t = std::decay_t<Arg>;

public:
    using pointer = std::conditional_t<std::is_pointer_v<arg_t>,
        arg_t,
        detail::compute_deleter_pointer_t<arg_t>
    >;

    void operator()(pointer val) const
    {
        if constexpr (std::is_pointer_v<arg_t> || detail::NullablePointer<arg_t>) {
            f(val);
        } else {
            f(val.get());
        }
    }
};


// #include <cstdio>
// #include <memory>
//
// using OpaqueHandle = int;
//
// extern "C" {
//     OpaqueHandle createHandle() noexcept;
//     bool destroyHandle(OpaqueHandle) noexcept;
// }
//
// int main() {
//     std::unique_ptr<void, deleter_fn<std::free>> p{std::malloc(42)};
//     std::unique_ptr<std::FILE, deleter_fn<std::fclose>> f{std::fopen("", "r")};
//
//     std::unique_ptr<OpaqueHandle, deleter_fn<destroyHandle>> h1;
//     std::unique_ptr<OpaqueHandle, deleter_fn<destroyHandle>> h2{ createHandle() };
// }
