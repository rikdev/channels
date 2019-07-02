#pragma once
#include "functional.h"
#include <type_traits>

namespace channels {
namespace detail {
namespace compatibility {

#if __cpp_lib_is_invocable
using std::is_invocable; // NOLINT
using std::is_invocable_v; // NOLINT
using std::invoke_result; // NOLINT
using std::invoke_result_t; // NOLINT
#else
namespace type_traits_detail {

template<typename, typename... Ts>
struct is_invocable_impl : std::false_type {};

template<typename... Ts>
struct is_invocable_impl<decltype(invoke(std::declval<Ts>()...)), Ts...> : std::true_type {};

} // namespace type_traits_detail

template<typename F, typename... Args>
struct is_invocable : type_traits_detail::is_invocable_impl<void, F, Args...> {};

template<typename F, typename... Args>
constexpr bool is_invocable_v = is_invocable<F, Args...>::value;

template<typename F, typename... Args>
struct invoke_result {
	using type = decltype(invoke(std::declval<F>(), std::declval<Args>()...));
};

template<typename F, typename... Args>
using invoke_result_t = typename invoke_result<F, Args...>::type;
#endif

// remove_cvref
template<typename T>
struct remove_cvref {
	using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template<typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

} // namespace compatibility
} // namespace detail
} // namespace channels
