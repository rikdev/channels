#pragma once
#include "functional.h"
#include <tuple>
#include <type_traits>
#include <utility>

namespace channels {
namespace detail {
namespace compatibility {

#if __cpp_lib_apply
using std::apply; // NOLINT
#else
template<typename F, typename Tuple, size_t... I>
constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
	(void) t;
	return invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
}

template<typename F, typename Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t)
{
	return apply_impl(
		std::forward<F>(f), std::forward<Tuple>(t),
		std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>());
}
#endif

} // namespace compatibility
} // namespace detail
} // namespace channels
