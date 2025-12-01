#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <memory>
#include <span>
#include <utility>
#include <cmath>

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using idx         = std::ptrdiff_t;
using usize       = std::size_t;
using isize       = std::ptrdiff_t;
using byte        = std::byte;
using voidp       = void*;
using const_voidp = const void*;

template<typename T, typename D = std::default_delete<T>>
using up = std::unique_ptr<T, D>;

namespace ycore
{

template<typename T>
[[nodiscard]] constexpr bool fuzzy_equal(T a, T b) {
    const T scale = std::same_as<T, double> ? T(1000000000000.) : T(100000.f);
    return std::fabs(a - b) * scale <= std::min(std::fabs(a), std::fabs(b));
}

template<typename T>
using param_t = std::conditional_t<std::is_trivially_copyable_v<T> && sizeof(T) <= 32, T, const T&>;

template<typename T>
auto cmp_set(T& lhs, param_t<T> rhs) noexcept -> bool {
    if constexpr (std::is_floating_point_v<T>) {
        if (! fuzzy_equal(lhs, rhs)) {
            lhs = rhs;
            return true;
        }
    } else {
        if (lhs != rhs) {
            lhs = rhs;
            return true;
        }
    }
    return false;
}

template<class F>
struct y_combinator {
    F f; // the lambda will be stored here

    // a forwarding operator():
    template<class... Args>
    decltype(auto) operator()(Args&&... args) const {
        // we pass ourselves to f, then the arguments.
        return f(*this, std::forward<Args>(args)...);
    }
};

template<class F>
y_combinator(F) -> y_combinator<F>;

} // namespace ycore