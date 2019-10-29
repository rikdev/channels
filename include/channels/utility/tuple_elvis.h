#pragma once
#include "../detail/compatibility/compile_features.h"
#include "../detail/compatibility/type_traits.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#ifdef CHANNELS_CPP_LIB_OPTIONAL
#include <optional>
#endif

namespace channels {
inline namespace utility {

namespace tuple_elvis_detail {

#if __cpp_concepts
template<typename T>
concept Optional = requires(T x) {
	static_cast<bool>(x);
	*x;
};

template<typename T, std::size_t I>
concept Tuple = requires(T x) {
	// is tuple
	std::get<I>(x);
};
#endif

} // namespace tuple_elvis_detail

/// This class gets optional of the `std::tuple` and index of the element from this tuple and looks like optional of this
/// element.
/// \tparam Optional Type of object that has optional-like interface of tuple-like interface
///                  (for example `std::optional<std::tuple<int>>`, `std::unique_ptr<std::pair<int, float>>, etc.).
/// \tparam I Index of element form tuple.
///
/// Example:
/// \code
/// struct Point {
/// 	int x;
/// 	int y;
/// };
/// channels::buffered_channel<Point> point_channel;
/// ...
/// const auto point = channels::utility::make_tuple_elvis(point_channel.get_value());
/// // or `channels::utility::tuple_elvis point = point_channel.get_value();` for C++17
/// if (point)
/// 	std::cout << "Point: x=" << point->x << " y=" << point->y;
/// \endcode
template<typename Optional, std::size_t I = 0>
//	requires tuple_elvis_detail::Optional<Optional> && tuple_elvis_detail::Tuple<decltype(*std::declval<Optional>()), I>
class tuple_elvis {
	using tuple_type = detail::compatibility::remove_cvref_t<decltype(*std::declval<Optional>())>;

public:
	using value_type = std::tuple_element_t<I, tuple_type>;

	// constructors

	tuple_elvis() = default;

	tuple_elvis(const tuple_elvis&) = default;
	tuple_elvis(tuple_elvis&&) = default;

	template<
		typename U,
		std::enable_if_t<std::is_constructible<Optional, U&&>::value && !std::is_convertible<U&&, Optional>::value, int> = 0>
	constexpr explicit tuple_elvis(U&& data);

	template<
		typename U,
		std::enable_if_t<std::is_constructible<Optional, U&&>::value && std::is_convertible<U&&, Optional>::value, int> = 0>
	constexpr tuple_elvis(U&& data); // NOLINT: Allow implicit conversion

	// destructor

	~tuple_elvis() = default;

	// assignments

	tuple_elvis& operator=(const tuple_elvis&) = default;
	tuple_elvis& operator=(tuple_elvis&&) = default;

	template<typename U, typename = std::enable_if_t<std::is_assignable<Optional&, U>::value>>
	tuple_elvis& operator=(U&& data);

	// swap

	void swap(tuple_elvis& other) noexcept;

	// observers

	CHANNELS_NODISCARD constexpr decltype(auto) operator->() const noexcept;
	CHANNELS_NODISCARD constexpr decltype(auto) operator->() noexcept;

	CHANNELS_NODISCARD constexpr decltype(auto) operator*() const& noexcept;
	CHANNELS_NODISCARD constexpr decltype(auto) operator*() & noexcept;
	CHANNELS_NODISCARD constexpr decltype(auto) operator*() const&& noexcept;
	CHANNELS_NODISCARD constexpr decltype(auto) operator*() && noexcept;

	// these methods require the value method of the Object type
	CHANNELS_NODISCARD constexpr decltype(auto) value() const&;
	CHANNELS_NODISCARD constexpr decltype(auto) value() &;
	CHANNELS_NODISCARD constexpr decltype(auto) value() const&&;
	CHANNELS_NODISCARD constexpr decltype(auto) value() &&;

	template<typename U>
	CHANNELS_NODISCARD constexpr value_type value_or(U&& default_value) const&;
	template<typename U>
	CHANNELS_NODISCARD constexpr value_type value_or(U&& default_value) &&;

	constexpr explicit operator bool() const noexcept;
	CHANNELS_NODISCARD constexpr bool has_value() const noexcept;

	// conversions

	/// converts tuple_elvis to another optional type U
	template<typename U
#ifdef CHANNELS_CPP_LIB_OPTIONAL
		= std::optional<value_type>
#endif
	>
	U to_optional() const &;

	template<typename U
#ifdef CHANNELS_CPP_LIB_OPTIONAL
		= std::optional<value_type>
#endif
	>
	U to_optional() &;

	template<typename U
#ifdef CHANNELS_CPP_LIB_OPTIONAL
		= std::optional<value_type>
#endif
	>
	U to_optional() const &&;

	template<typename U
#ifdef CHANNELS_CPP_LIB_OPTIONAL
		= std::optional<value_type>
#endif
	>
	U to_optional() &&;

private:
	Optional data_{};
};

#if __cpp_deduction_guides
template<typename Optional>
tuple_elvis(Optional) -> tuple_elvis<Optional>;
#endif

// # non-member functions

// ## relational operations

template<typename LO, std::size_t LI, typename RO, std::size_t RI>
CHANNELS_NODISCARD constexpr bool operator==(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs);
template<typename LO, std::size_t LI, typename RO, std::size_t RI>
CHANNELS_NODISCARD constexpr bool operator!=(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs);
template<typename LO, std::size_t LI, typename RO, std::size_t RI>
CHANNELS_NODISCARD constexpr bool operator<(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs);
template<typename LO, std::size_t LI, typename RO, std::size_t RI>
CHANNELS_NODISCARD constexpr bool operator>(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs);
template<typename LO, std::size_t LI, typename RO, std::size_t RI>
CHANNELS_NODISCARD constexpr bool operator<=(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs);
template<typename LO, std::size_t LI, typename RO, std::size_t RI>
CHANNELS_NODISCARD constexpr bool operator>=(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs);

// ### comparison with T

template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator==(const tuple_elvis<O, I>& lhs, const U& rhs);
template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator==(const U& lhs, const tuple_elvis<O, I>& rhs);
template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator!=(const tuple_elvis<O, I>& lhs, const U& rhs);
template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator!=(const U& lhs, const tuple_elvis<O, I>& rhs);
template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator<(const tuple_elvis<O, I>& lhs, const U& rhs);
template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator<(const U& lhs, const tuple_elvis<O, I>& rhs);
template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator<=(const tuple_elvis<O, I>& lhs, const U& rhs);
template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator<=(const U& lhs, const tuple_elvis<O, I>& rhs);
template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator>(const tuple_elvis<O, I>& lhs, const U& rhs);
template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator>(const U& lhs, const tuple_elvis<O, I>& rhs);
template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator>=(const tuple_elvis<O, I>& lhs, const U& rhs);
template<typename O, std::size_t I, typename U>
CHANNELS_NODISCARD constexpr bool operator>=(const U& lhs, const tuple_elvis<O, I>& rhs);

// ## specialized algorithms

template<typename Optional, std::size_t I>
void swap(tuple_elvis<Optional, I>& lhs, tuple_elvis<Optional, I>& rhs) noexcept(noexcept(lhs.swap(rhs)));

template<std::size_t I = 0, typename Optional>
CHANNELS_NODISCARD constexpr tuple_elvis<std::decay_t<Optional>, I> make_tuple_elvis(Optional&& v);

// # implementation

// ## tuple_elvis

template<typename Optional, std::size_t I>
template<
	typename U,
	std::enable_if_t<std::is_constructible<Optional, U&&>::value && !std::is_convertible<U&&, Optional>::value, int>>
constexpr tuple_elvis<Optional, I>::tuple_elvis(U&& data)
	: data_{std::forward<U>(data)}
{}

template<typename Optional, std::size_t I>
template<
	typename U,
	std::enable_if_t<std::is_constructible<Optional, U&&>::value && std::is_convertible<U&&, Optional>::value, int>>
constexpr tuple_elvis<Optional, I>::tuple_elvis(U&& data)
	: data_{std::forward<U>(data)}
{}

template<typename Optional, std::size_t I>
template<typename U, typename>
tuple_elvis<Optional, I>& tuple_elvis<Optional, I>::operator=(U&& data)
{
	data_ = std::forward<U>(data);

	return *this;
}

template<typename Optional, std::size_t I>
void tuple_elvis<Optional, I>::swap(tuple_elvis& other) noexcept
{
	std::swap(data_, other.data_);
}

template<typename Optional, std::size_t I>
constexpr decltype(auto) tuple_elvis<Optional, I>::operator->() const noexcept
{
	return std::addressof(**this);
}

template<typename Optional, std::size_t I>
constexpr decltype(auto) tuple_elvis<Optional, I>::operator->() noexcept
{
	return std::addressof(**this);
}

template<typename Optional, std::size_t I>
constexpr decltype(auto) tuple_elvis<Optional, I>::operator*() const & noexcept
{
	return std::get<I>(*data_);
}

template<typename Optional, std::size_t I>
constexpr decltype(auto) tuple_elvis<Optional, I>::operator*() & noexcept
{
	return std::get<I>(*data_);
}

template<typename Optional, std::size_t I>
constexpr decltype(auto) tuple_elvis<Optional, I>::operator*() const && noexcept
{
	return std::get<I>(*std::move(data_));
}

template<typename Optional, std::size_t I>
constexpr decltype(auto) tuple_elvis<Optional, I>::operator*() && noexcept
{
	return std::get<I>(*std::move(data_));
}

template<typename Optional, std::size_t I>
constexpr decltype(auto) tuple_elvis<Optional, I>::value() const &
{
	return std::get<I>(data_.value());
}

template<typename Optional, std::size_t I>
constexpr decltype(auto) tuple_elvis<Optional, I>::value() &
{
	return std::get<I>(data_.value());
}

template<typename Optional, std::size_t I>
constexpr decltype(auto) tuple_elvis<Optional, I>::value() const &&
{
	return std::get<I>(std::move(data_).value());
}

template<typename Optional, std::size_t I>
constexpr decltype(auto) tuple_elvis<Optional, I>::value() &&
{
	return std::get<I>(std::move(data_).value());
}

template<typename Optional, std::size_t I>
template<typename U>
constexpr typename tuple_elvis<Optional, I>::value_type tuple_elvis<Optional, I>::value_or(U&& default_value) const &
{
	if (!has_value())
		return std::forward<U>(default_value);

	return **this;
}

template<typename Optional, std::size_t I>
template<typename U>
constexpr typename tuple_elvis<Optional, I>::value_type tuple_elvis<Optional, I>::value_or(U&& default_value) &&
{
	if (!has_value())
		return std::forward<U>(default_value);

	return *std::move(*this);
}

template<typename Optional, std::size_t I>
constexpr tuple_elvis<Optional, I>::operator bool() const noexcept
{
	return has_value();
}

template<typename Optional, std::size_t I>
constexpr bool tuple_elvis<Optional, I>::has_value() const noexcept
{
	return static_cast<bool>(data_);
}

template<typename Optional, std::size_t I>
template<typename U>
U tuple_elvis<Optional, I>::to_optional() &
{
	if (!has_value())
		return U{};

	return U{**this};
}

template<typename Optional, std::size_t I>
template<typename U>
U tuple_elvis<Optional, I>::to_optional() const &
{
	if (!has_value())
		return U{};

	return U{**this};
}

template<typename Optional, std::size_t I>
template<typename U>
U tuple_elvis<Optional, I>::to_optional() &&
{
	if (!has_value())
		return U{};

	return U{*std::move(*this)};
}

template<typename Optional, std::size_t I>
template<typename U>
U tuple_elvis<Optional, I>::to_optional() const &&
{
	if (!has_value())
		return U{};

	return U{*std::move(*this)};
}

// ## non-member functions

// ### relational operations

template<typename LO, std::size_t LI, typename RO, std::size_t RI>
constexpr bool operator==(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs)
{
	if (static_cast<bool>(lhs) != static_cast<bool>(rhs))
		return false;

	if (!lhs)
		return true;

	return *lhs == *rhs;
}

template<typename LO, std::size_t LI, typename RO, std::size_t RI>
constexpr bool operator!=(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs)
{
	if (static_cast<bool>(lhs) != static_cast<bool>(rhs))
		return true;

	if (!lhs)
		return false;

	return *lhs != *rhs;
}

template<typename LO, std::size_t LI, typename RO, std::size_t RI>
constexpr bool operator<(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs)
{
	if (!rhs)
		return false;

	if (!lhs)
		return true;

	return *lhs < *rhs;
}

template<typename LO, std::size_t LI, typename RO, std::size_t RI>
constexpr bool operator>(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs)
{
	if (!lhs)
		return false;

	if (!rhs)
		return true;

	return *lhs > *rhs;
}

template<typename LO, std::size_t LI, typename RO, std::size_t RI>
constexpr bool operator<=(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs)
{
	if (!lhs)
		return true;

	if (!rhs)
		return false;

	return *lhs <= *rhs;
}

template<typename LO, std::size_t LI, typename RO, std::size_t RI>
constexpr bool operator>=(const tuple_elvis<LO, LI>& lhs, const tuple_elvis<RO, RI>& rhs)
{
	if (!rhs)
		return true;

	if (!lhs)
		return false;

	return *lhs >= *rhs;
}

// #### comparison with T

template<typename O, std::size_t I, typename U>
constexpr bool operator==(const tuple_elvis<O, I>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs == rhs : false;
}

template<typename O, std::size_t I, typename U>
constexpr bool operator==(const U& lhs, const tuple_elvis<O, I>& rhs)
{
	return static_cast<bool>(rhs) ? lhs == *rhs : false;
}

template<typename O, std::size_t I, typename U>
constexpr bool operator!=(const tuple_elvis<O, I>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs != rhs : true;
}

template<typename O, std::size_t I, typename U>
constexpr bool operator!=(const U& lhs, const tuple_elvis<O, I>& rhs)
{
	return static_cast<bool>(rhs) ? lhs != *rhs : true;
}

template<typename O, std::size_t I, typename U>
constexpr bool operator<(const tuple_elvis<O, I>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs < rhs : true;
}

template<typename O, std::size_t I, typename U>
constexpr bool operator<(const U& lhs, const tuple_elvis<O, I>& rhs)
{
	return static_cast<bool>(rhs) ? lhs < *rhs : false;
}

template<typename O, std::size_t I, typename U>
constexpr bool operator<=(const tuple_elvis<O, I>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs <= rhs : true;
}

template<typename O, std::size_t I, typename U>
constexpr bool operator<=(const U& lhs, const tuple_elvis<O, I>& rhs)
{
	return static_cast<bool>(rhs) ? lhs <= *rhs : false;
}

template<typename O, std::size_t I, typename U>
constexpr bool operator>(const tuple_elvis<O, I>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs > rhs : false;
}

template<typename O, std::size_t I, typename U>
constexpr bool operator>(const U& lhs, const tuple_elvis<O, I>& rhs)
{
	return static_cast<bool>(rhs) ? lhs > *rhs : true;
}

template<typename O, std::size_t I, typename U>
constexpr bool operator>=(const tuple_elvis<O, I>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs >= rhs : false;
}

template<typename O, std::size_t I, typename U>
constexpr bool operator>=(const U& lhs, const tuple_elvis<O, I>& rhs)
{
	return static_cast<bool>(rhs) ? lhs >= *rhs : true;
}

// ### specialized algorithms

template<typename Optional, std::size_t I>
void swap(tuple_elvis<Optional, I>& lhs, tuple_elvis<Optional, I>& rhs) noexcept(noexcept(lhs.swap(rhs)))
{
	lhs.swap(rhs);
}

template<std::size_t I, typename Optional>
constexpr tuple_elvis<std::decay_t<Optional>, I> make_tuple_elvis(Optional&& v)
{
	return tuple_elvis<std::decay_t<Optional>, I>(std::forward<Optional>(v));
}

} // namespace utility
} // namespace channels

namespace std {

template<typename Optional, std::size_t I>
class hash<channels::utility::tuple_elvis<Optional, I>> {
	using value_type = typename channels::utility::tuple_elvis<Optional, I>::value_type;

	static_assert(
		std::is_default_constructible<std::hash<std::remove_const_t<value_type>>>::value,
		"For tuple_elvis::value_type must be declared hash");

public:
	CHANNELS_NODISCARD std::size_t operator()(const channels::utility::tuple_elvis<Optional, I>& v) const noexcept
	{
		return v ? hash<std::remove_const_t<value_type>>()(*v) : 0;
	}
};

} // namespace std
