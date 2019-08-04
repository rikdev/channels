#pragma once
#include <type_traits>

namespace channels {
namespace test {
namespace tools {

// Wrapper for type T that disables implicit casts
template<typename T>
struct relation_only {
	using value_type = T;

	constexpr relation_only() = default;
	constexpr explicit relation_only(const T& v) noexcept(std::is_nothrow_copy_constructible<T>::value)
		: value{v}
	{}

	value_type value{};
};

template<typename T, typename U>
constexpr bool operator==(const relation_only<T>& lhs, const relation_only<U>& rhs)
{
	return lhs.value == rhs.value;
}

template<typename T, typename U>
constexpr bool operator!=(const relation_only<T>& lhs, const relation_only<U>& rhs)
{
	return lhs.value != rhs.value;
}

template<typename T, typename U>
constexpr bool operator<(const relation_only<T>& lhs, const relation_only<U>& rhs)
{
	return lhs.value < rhs.value;
}

template<typename T, typename U>
constexpr bool operator<=(const relation_only<T>& lhs, const relation_only<U>& rhs)
{
	return lhs.value <= rhs.value;
}

template<typename T, typename U>
constexpr bool operator>(const relation_only<T>& lhs, const relation_only<U>& rhs)
{
	return lhs.value > rhs.value;
}

template<typename T, typename U>
constexpr bool operator>=(const relation_only<T>& lhs, const relation_only<U>& rhs)
{
	return lhs.value >= rhs.value;
}

using relation_only_int = relation_only<int>;

} // namespace tools
} // namespace test
} // namespace channels
