#pragma once
#include <tuple>
#include <type_traits>

namespace channels {
namespace detail {

// is_move_assignable

template<typename...>
struct is_move_assignable;

template<>
struct is_move_assignable<> : std::true_type {};

template<typename T, typename... Ts>
struct is_move_assignable<T, Ts...>
	: std::integral_constant<bool, std::is_move_assignable<T>::value && is_move_assignable<Ts...>::value> {};

template<typename... Ts>
struct is_move_assignable<std::tuple<Ts...>> : is_move_assignable<Ts...> {};

} // namespace detail
} // namespace channels
