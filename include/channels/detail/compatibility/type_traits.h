#pragma once
#include "compile_features.h"
#include "functional.h"
#include <type_traits>

namespace channels {
namespace detail {
namespace compatibility {

#ifdef CHANNELS_CPP_LIB_IS_INVOCABLE
using std::is_invocable; // NOLINT(misc-unused-using-decls)
using std::is_invocable_v; // NOLINT(misc-unused-using-decls)
using std::invoke_result; // NOLINT(misc-unused-using-decls)
using std::invoke_result_t; // NOLINT(misc-unused-using-decls)
#else
// is_invocable

namespace type_traits_detail {

template<typename Stub, typename... Ts>
struct is_invocable_impl : std::false_type {};

template<typename F, typename... Args>
struct is_invocable_impl<decltype(std::declval<F>()(std::declval<Args>()...)), F, Args...> : std::true_type {};

template<typename F, typename T, typename... Args>
struct is_invocable_impl<
	decltype((std::declval<T>().*std::declval<F>())(std::declval<Args>()...)), F, T, Args...> : std::true_type {};

} // namespace type_traits_detail

template<typename F, typename... Args>
struct is_invocable : type_traits_detail::is_invocable_impl<void, F, Args...> {};

template<typename F, typename... Args>
constexpr bool is_invocable_v = is_invocable<F, Args...>::value;

// invoke_result

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
