#pragma once
#include "compatibility/compile_features.h"
#include "compatibility/utility.h"
#include <array>
#include <cstddef>
#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#if __cpp_lib_optional
#include <optional>
#endif

namespace channels {
namespace detail {

#if __cpp_lib_optional
// cow_allow_inline_placement
template<typename T>
struct cow_allow_inline_placement
	: std::conjunction<std::is_trivially_copy_constructible<T>, std::is_trivially_copy_assignable<T>> {};

template<typename T>
struct cow_allow_inline_placement<std::shared_ptr<T>> : std::true_type {};

template<typename T>
struct cow_allow_inline_placement<std::weak_ptr<T>> : std::true_type {};

template<>
struct cow_allow_inline_placement<std::exception_ptr> : std::true_type {};

template<typename... Ts>
struct cow_allow_inline_placement<std::tuple<Ts...>> : std::conjunction<cow_allow_inline_placement<Ts>...> {};

template<typename... Ts>
struct cow_allow_inline_placement<std::pair<Ts...>> : std::conjunction<cow_allow_inline_placement<Ts>...> {};

template<typename T, std::size_t N>
struct cow_allow_inline_placement<std::array<T, N>> : cow_allow_inline_placement<T> {};

// cow_use_inline_storage
template<typename T, std::size_t MaxInlineStorageSize>
struct cow_use_inline_storage :
	std::bool_constant<cow_allow_inline_placement<T>::value && sizeof(T) <= MaxInlineStorageSize> {};

using std::nullopt_t;
using std::nullopt;
using std::bad_optional_access;
#else
template<typename T, std::size_t MaxInlineStorageSize>
struct cow_use_inline_storage : std::false_type {};

struct nullopt_t {};
constexpr nullopt_t nullopt{};

struct bad_optional_access : std::exception {
	CHANNELS_NODISCARD const char* what() const noexcept override {
		return "Bad optional access";
	}
};
#endif

using compatibility::in_place_t;
using compatibility::in_place;

template<typename T, std::size_t MaxInlineStorageSize = sizeof(std::shared_ptr<T>)>
constexpr bool cow_use_inline_storage_v = cow_use_inline_storage<T, MaxInlineStorageSize>::value;

/// The class `cow_optional` implements copy on write storage and provides `std::optional` like interface.
/// \tparam T Value type.
/// \tparam UseInlineStorage If false one value of T shared between all copies of the `cow_optional` object.
///                          If true each copy of the `cow_optional` object keep own copy of value object (small
///                          buffer optimization).
template<typename T, bool UseInlineStorage = cow_use_inline_storage_v<T>>
class cow_optional;

namespace cow_detail {

template<typename ToOptional, typename U>
struct direct_conversation {
	using to_value_type = typename ToOptional::value_type;

	static constexpr bool allow =
		std::is_constructible<to_value_type, U&&>::value
		&& !std::is_same<std::decay_t<U>, compatibility::in_place_t>::value
		&& !std::is_same<std::decay_t<U>, ToOptional>::value;

	static constexpr bool allow_implicit = std::is_convertible<U&&, to_value_type>::value && allow;
	static constexpr bool allow_explicit = !std::is_convertible<U&&, to_value_type>::value && allow;
};

template<typename T, typename FromOptional>
struct unwrapping {
	using from_value_type = typename FromOptional::value_type;

	static constexpr bool allow =
		!std::is_constructible<T, FromOptional&>::value
		&& !std::is_constructible<T, FromOptional&&>::value
		&& !std::is_constructible<T, const FromOptional&>::value
		&& !std::is_constructible<T, const FromOptional&&>::value
		&& !std::is_convertible<FromOptional&, T>::value
		&& !std::is_convertible<FromOptional&&, T>::value
		&& !std::is_convertible<const FromOptional&, T>::value
		&& !std::is_convertible<const FromOptional&&, T>::value;

	static constexpr bool allow_copy =
		std::is_constructible<T, const from_value_type&>::value
		&& allow;

	static constexpr bool allow_copy_implicit =
		std::is_convertible<const from_value_type&, T>::value
		&& allow_copy;

	static constexpr bool allow_copy_explicit =
		!std::is_convertible<const from_value_type&, T>::value
		&& allow_copy;

	static constexpr bool allow_move =
		std::is_constructible<T, from_value_type&&>::value
		&& allow;

	static constexpr bool allow_move_implicit =
		std::is_convertible<from_value_type&&, T>::value
		&& allow_move;

	static constexpr bool allow_move_explicit =
		!std::is_convertible<from_value_type&&, T>::value
		&& allow_move;
};

template<typename ToOptional, typename U>
struct assign_direct_conversation {
	using to_value_type = typename ToOptional::value_type;

	static constexpr bool allow =
		!std::is_same<std::decay_t<U>, ToOptional>::value
		&& !(std::is_scalar<to_value_type>::value && std::is_same<to_value_type, std::decay_t<U>>::value)
		&& std::is_constructible<to_value_type, U>::value
		&& std::is_assignable<to_value_type&, U>::value;
};

template<typename T, typename FromOptional>
struct assing_unwrapping {
	using from_value_type = typename FromOptional::value_type;

	static constexpr bool allow =
		unwrapping<T, FromOptional>::allow
		&& !std::is_assignable<T&, FromOptional&>::value
		&& !std::is_assignable<T&, FromOptional&&>::value
		&& !std::is_assignable<T&, const FromOptional&>::value
		&& !std::is_assignable<T&, const FromOptional&&>::value;

	static constexpr bool allow_copy =
		std::is_constructible<T, const from_value_type&>::value
		&& std::is_assignable<T&, const from_value_type&>::value
		&& allow;

	static constexpr bool allow_move =
		std::is_constructible<T, from_value_type>::value
		&& std::is_assignable<T&, from_value_type>::value
		&& allow;
};

} // namespace cow_detail

template<typename T>
class cow_optional<T, false> {
	static_assert(
		!std::is_reference<T>::value, "Instantiation of optional with a reference type is ill-formed");
	static_assert(
		!std::is_same<std::decay_t<T>, in_place_t>::value, "Instantiation of optional with in_place_t is ill-formed");
	static_assert(
		!std::is_same<std::decay_t<T>, nullopt_t>::value, "Instantiation of optional with nullopt_t is ill-formed");
	static_assert(
		std::is_destructible<T>::value, "Instantiation of optional with a non-destructible type is ill-formed");

	template<typename, bool>
	friend class cow_optional;

public:
	using value_type = T;

	// constructors

	cow_optional() = default;

	constexpr cow_optional(nullopt_t) noexcept // NOLINT: Allow implicit conversion
		: cow_optional{}
	{}

	cow_optional(const cow_optional&) = default;
	cow_optional(cow_optional&&) = default;

	template<typename... Args, typename = std::enable_if_t<std::is_constructible<T, Args...>::value>>
	constexpr explicit cow_optional(in_place_t, Args&&... args)
		: data_{std::make_shared<value_type>(std::forward<Args>(args)...)}
	{}

	template<
		typename U,
		typename... Args,
		typename = std::enable_if_t<std::is_constructible<T, std::initializer_list<U>&, Args&&...>::value>>
	constexpr explicit cow_optional(in_place_t, std::initializer_list<U> ilist, Args&&... args)
		: data_{std::make_shared<value_type>(ilist, std::forward<Args>(args)...)}
	{}

	template<
		typename U = T, std::enable_if_t<cow_detail::direct_conversation<cow_optional, U>::allow_implicit, int> = 0>
	constexpr cow_optional(U&& value) // NOLINT: Allow implicit conversion
		: cow_optional{in_place, std::forward<U>(value)}
	{}

	template<
		typename U = T, std::enable_if_t<cow_detail::direct_conversation<cow_optional, U>::allow_explicit, int> = 0>
	constexpr explicit cow_optional(U&& value)
		: cow_optional{in_place, std::forward<U>(value)}
	{}

	template<
		typename U,
		std::enable_if_t<
			std::is_convertible<U*, T*>::value && cow_detail::unwrapping<T, cow_optional<U, false>>::allow_copy,
			int> = 0>
	constexpr cow_optional(const cow_optional<U, false>& other) noexcept // NOLINT: Allow implicit conversion
		: data_{other.data_}
	{}

	// non-cow constructor
	template<
		typename U,
		std::enable_if_t<
			!std::is_convertible<U*, T*>::value && cow_detail::unwrapping<T, cow_optional<U, false>>::allow_copy,
			int> = 0>
	constexpr explicit cow_optional(const cow_optional<U, false>& other)
		: data_{other ? std::make_shared<value_type>(*other) : nullptr}
	{}


#if __cpp_lib_optional
	// non-cow constructor
	template<typename U, std::enable_if_t<cow_detail::unwrapping<T, cow_optional<U, true>>::allow_copy, int> = 0>
	constexpr explicit cow_optional(const cow_optional<U, true>& other)
		: data_{other ? std::make_shared<value_type>(*other) : nullptr}
	{}
#endif

	template<typename U, std::enable_if_t<std::is_convertible<U*, T*>::value, int> = 0>
	constexpr cow_optional(cow_optional<U, false>&& other) noexcept // NOLINT: Allow implicit conversion
		: data_{std::move(other.data_)}
	{}

	// non-cow constructor
	template<
		typename U,
		std::enable_if_t<
			!std::is_convertible<U*, T*>::value && cow_detail::unwrapping<T, cow_optional<U, false>>::allow_move,
			int> = 0>
	constexpr explicit cow_optional(cow_optional<U, false>&& other)
		: data_{other ? std::make_shared<value_type>(*std::move(other)) : nullptr}
	{}

#if __cpp_lib_optional
	// non-cow constructor
	template<typename U, std::enable_if_t<cow_detail::unwrapping<T, cow_optional<U, true>>::allow_move, int> = 0>
	constexpr explicit cow_optional(cow_optional<U, true>&& other)
		: data_{other ? std::make_shared<value_type>(*std::move(other)) : nullptr}
	{}
#endif

	// destructor
	~cow_optional() = default;

	// assignments

	cow_optional& operator=(nullopt_t) noexcept
	{
		reset();
		return *this;
	}

	cow_optional& operator=(const cow_optional&) = default;
	cow_optional& operator=(cow_optional&&) = default;

	template<
		typename U = T, typename = std::enable_if_t<cow_detail::assign_direct_conversation<cow_optional, U>::allow>>
	cow_optional& operator=(U&& value)
	{
		set_value(std::forward<U>(value));
		return *this;
	}

	template<
		typename U = T,
		std::enable_if_t<
			std::is_convertible<U*, T*>::value && cow_detail::assing_unwrapping<T, cow_optional<U, false>>::allow_copy,
			int> = 0>
	cow_optional& operator=(const cow_optional<U, false>& other) noexcept
	{
		data_ = other.data_;
		return *this;
	}

	// non-cow assignment
	template<
		typename U = T,
		std::enable_if_t<
			!std::is_convertible<U*, T*>::value && cow_detail::assing_unwrapping<T, cow_optional<U, false>>::allow_copy,
			int> = 0>
	cow_optional& operator=(const cow_optional<U, false>& other)
	{
		if (other)
			set_value(*other);
		else
			reset();

		return *this;
	}

#if __cpp_lib_optional
	// non-cow assignment
	template<
		typename U = T, std::enable_if_t<cow_detail::assing_unwrapping<T, cow_optional<U, true>>::allow_copy, int> = 0>
	cow_optional& operator=(const cow_optional<U, true>& other)
	{
		if (other)
			set_value(*other);
		else
			reset();

		return *this;
	}
#endif

	template<
		typename U = T,
		std::enable_if_t<
			std::is_convertible<U*, T*>::value && cow_detail::assing_unwrapping<T, cow_optional<U, false>>::allow_move,
			int> = 0>
	cow_optional& operator=(cow_optional<U, false>&& other) noexcept
	{
		data_ = std::move(other.data_);
		return *this;
	}

	// non-cow assignment
	template<
		typename U = T,
		std::enable_if_t<
			!std::is_convertible<U*, T*>::value && cow_detail::assing_unwrapping<T, cow_optional<U, false>>::allow_move,
			int> = 0>
	cow_optional& operator=(cow_optional<U, false>&& other)
	{
		if (other)
			set_value(*std::move(other));
		else
			reset();

		return *this;
	}

#if __cpp_lib_optional
	// non-cow assignment
	template<
		typename U = T, std::enable_if_t<cow_detail::assing_unwrapping<T, cow_optional<U, true>>::allow_move, int> = 0>
	cow_optional& operator=(cow_optional<U, true>&& other)
	{
		if (other)
			set_value(*std::move(other));
		else
			reset();

		return *this;
	}
#endif

	// swap

	void swap(cow_optional& other) noexcept
	{
		data_.swap(other.data_);
	}

	// observers

	CHANNELS_NODISCARD constexpr const T* operator->() const noexcept
	{
		return data_.get();
	}

	CHANNELS_NODISCARD constexpr const T& operator*() const& noexcept
	{
		return *data_;
	}

	CHANNELS_NODISCARD constexpr const T&& operator*() const&& noexcept
	{
		return std::move(*data_);
	}

	constexpr explicit operator bool() const noexcept
	{
		return has_value();
	}

	CHANNELS_NODISCARD constexpr bool has_value() const noexcept
	{
		return static_cast<bool>(data_);
	}

	CHANNELS_NODISCARD constexpr const T& value() const&
	{
		if (!data_)
			throw bad_optional_access{};

		return *data_;
	}

	CHANNELS_NODISCARD constexpr const T&& value() const&&
	{
		if (!data_)
			throw bad_optional_access{};

		return std::move(*data_);
	}

	template<typename U>
	CHANNELS_NODISCARD constexpr T value_or(U&& default_value) const&
	{
		if (!data_)
			return static_cast<value_type>(std::forward<U>(default_value));

		return *data_;
	}

	template<typename U>
	CHANNELS_NODISCARD constexpr T value_or(U&& default_value) &&
	{
		if (!data_)
			return static_cast<value_type>(std::forward<U>(default_value));

		if (data_.use_count() == 1)
			return std::move(*data_);

		return *data_;
	}

	// modifiers

	void reset() noexcept
	{
		data_.reset();
	}

private:
	template<typename U>
	void set_value(U&& value)
	{
		if (data_.use_count() == 1)
			*data_ = std::forward<U>(value);
		else
			data_ = std::make_shared<value_type>(std::forward<U>(value));
	}

	std::shared_ptr<value_type> data_;
};


// specialized algorithms

template<typename T>
void swap(cow_optional<T, false>& lhs, cow_optional<T, false>& rhs) noexcept
{
	lhs.swap(rhs);
}

#if __cpp_lib_optional
template<typename T>
class cow_optional<T, true> {
	template<typename, bool>
	friend class cow_optional;

public:
	using value_type = T;

	// constructors

	cow_optional() = default;

	constexpr cow_optional(nullopt_t) noexcept // NOLINT: Allow implicit conversion
		: data_{std::nullopt}
	{}

	cow_optional(const cow_optional&) = default;
	cow_optional(cow_optional&&) = default;

	template<typename... Args, typename = std::enable_if_t<std::is_constructible<T, Args...>::value>>
	constexpr explicit cow_optional(in_place_t, Args&&... args)
		: data_{std::in_place, std::forward<Args>(args)...}
	{}

	template<
		typename U,
		typename... Args,
		typename = std::enable_if_t<std::is_constructible<T, std::initializer_list<U>&, Args&&...>::value>>
	constexpr explicit cow_optional(in_place_t, std::initializer_list<U> ilist, Args&&... args)
		: data_{std::in_place, ilist, std::forward<Args>(args)...}
	{}

	template<
		typename U = T, std::enable_if_t<cow_detail::direct_conversation<cow_optional, U>::allow_implicit, int> = 0>
	constexpr cow_optional(U&& value) // NOLINT: Allow implicit conversion
		: data_{std::forward<U>(value)}
	{}

	template<
		typename U = T, std::enable_if_t<cow_detail::direct_conversation<cow_optional, U>::allow_explicit, int> = 0>
	constexpr explicit cow_optional(U&& value)
		: data_{std::forward<U>(value)}
	{}

	template<
		typename U, std::enable_if_t<cow_detail::unwrapping<T, cow_optional<U, true>>::allow_copy_implicit, int> = 0>
	constexpr cow_optional(const cow_optional<U, true>& other) // NOLINT: Allow implicit conversion
		: data_{other.data_}
	{}

	template<
		typename U, std::enable_if_t<cow_detail::unwrapping<T, cow_optional<U, false>>::allow_copy_implicit, int> = 0>
	constexpr cow_optional(const cow_optional<U, false>& other) // NOLINT: Allow implicit conversion
		: data_{other ? decltype(data_)(*other) : nullopt}
	{}

	template<
		typename U, std::enable_if_t<cow_detail::unwrapping<T, cow_optional<U, true>>::allow_copy_explicit, int> = 0>
	constexpr explicit cow_optional(const cow_optional<U, true>& other)
		: data_{other.data_}
	{}

	template<
		typename U, std::enable_if_t<cow_detail::unwrapping<T, cow_optional<U, false>>::allow_copy_explicit, int> = 0>
	constexpr explicit cow_optional(const cow_optional<U, false>& other)
		: data_{other ? decltype(data_)(*other) : nullopt}
	{}

	template<
		typename U, std::enable_if_t<cow_detail::unwrapping<T, cow_optional<U, true>>::allow_move_implicit, int> = 0>
	constexpr cow_optional(cow_optional<U, true>&& other) // NOLINT: Allow implicit conversion
		: data_{std::move(other.data_)}
	{}

	template<
		typename U, std::enable_if_t<cow_detail::unwrapping<T, cow_optional<U, false>>::allow_move_implicit, int> = 0>
	constexpr cow_optional(cow_optional<U, false>&& other) // NOLINT: Allow implicit conversion
		: data_{other ? decltype(data_)(*std::move(other)) : nullopt}
	{}

	template<
		typename U, std::enable_if_t<cow_detail::unwrapping<T, cow_optional<U, true>>::allow_move_explicit, int> = 0>
	constexpr explicit cow_optional(cow_optional<U, true>&& other)
		: data_{std::move(other.data_)}
	{}

	template<
		typename U, std::enable_if_t<cow_detail::unwrapping<T, cow_optional<U, false>>::allow_move_explicit, int> = 0>
	constexpr explicit cow_optional(cow_optional<U, false>&& other)
		: data_{other ? decltype(data_)(*std::move(other)) : nullopt}
	{}

	// destructor
	~cow_optional() = default;

	// assignments

	cow_optional& operator=(nullopt_t) noexcept
	{
		data_ = std::nullopt;
		return *this;
	}

	cow_optional& operator=(const cow_optional&) = default;
	cow_optional& operator=(cow_optional&&) = default;

	template<
		typename U = T, typename = std::enable_if_t<cow_detail::assign_direct_conversation<cow_optional, U>::allow>>
	cow_optional& operator=(U&& value)
	{
		data_ = std::forward<U>(value);
		return *this;
	}

	template<
		typename U = T,
		typename = std::enable_if_t<cow_detail::assing_unwrapping<T, cow_optional<U, true>>::allow_copy>>
	cow_optional& operator=(const cow_optional<U, true>& other)
	{
		data_ = other.data_;
		return *this;
	}

	template<
		typename U = T,
		typename = std::enable_if_t<cow_detail::assing_unwrapping<T, cow_optional<U, false>>::allow_copy>>
	cow_optional& operator=(const cow_optional<U, false>& other)
	{
		if (other)
			data_ = *other;
		else
			reset();

		return *this;
	}

	template<
		typename U = T,
		typename = std::enable_if_t<cow_detail::assing_unwrapping<T, cow_optional<U, true>>::allow_move>>
	cow_optional& operator=(cow_optional<U, true>&& other)
	{
		data_ = std::move(other.data_);
		return *this;
	}

	template<
		typename U = T,
		typename = std::enable_if_t<cow_detail::assing_unwrapping<T, cow_optional<U, false>>::allow_move>>
	cow_optional& operator=(cow_optional<U, false>&& other)
	{
		if (other)
			data_ = *std::move(other);
		else
			reset();

		return *this;
	}

	// swap

	void swap(cow_optional& other)
		noexcept(std::is_nothrow_move_constructible<T>::value && std::is_nothrow_swappable<T>::value)
	{
		data_.swap(other.data_);
	}

	// observers

	CHANNELS_NODISCARD constexpr const T* operator->() const
	{
		return data_.operator->();
	}

	CHANNELS_NODISCARD constexpr const T& operator*() const&
	{
		return data_.operator*();
	}

	CHANNELS_NODISCARD constexpr T&& operator*() &&
	{
		return std::move(data_).operator*();
	}

	CHANNELS_NODISCARD constexpr const T&& operator*() const&&
	{
		return std::move(data_).operator*();
	}

	constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(data_);
	}

	CHANNELS_NODISCARD constexpr bool has_value() const noexcept
	{
		return data_.has_value();
	}

	CHANNELS_NODISCARD constexpr const T& value() const&
	{
		return data_.value();
	}

	CHANNELS_NODISCARD constexpr T&& value() &&
	{
		return std::move(data_).value();
	}

	CHANNELS_NODISCARD constexpr const T&& value() const&&
	{
		return std::move(data_).value();
	}

	template<typename U>
	CHANNELS_NODISCARD constexpr T value_or(U&& default_value) const&
	{
		return data_.value_or(std::forward<U>(default_value));
	}

	template<typename U>
	CHANNELS_NODISCARD constexpr T value_or(U&& default_value) &&
	{
		return std::move(data_).value_or(std::forward<U>(default_value));
	}

	// modifiers

	void reset() noexcept
	{
		data_.reset();
	}

private:
	std::optional<value_type> data_;
};

// specialized algorithms

template<typename T>
std::enable_if_t<std::is_move_constructible<T>::value&& std::is_swappable<T>::value>
swap(cow_optional<T, true>& lhs, cow_optional<T, true>& rhs) noexcept(noexcept(lhs.swap(rhs)))
{
	lhs.swap(rhs);
}
#endif

#if __cpp_deduction_guides
template<typename T>
cow_optional(T) -> cow_optional<T>;
#endif

// # non-member functions

// ## relational operations

template<typename T, bool UseInlineStorage1, typename U, bool UseInlineStorage2>
CHANNELS_NODISCARD constexpr bool operator==(
	const cow_optional<T, UseInlineStorage1>& lhs, const cow_optional<U, UseInlineStorage2>& rhs)
{
	if (static_cast<bool>(lhs) != static_cast<bool>(rhs))
		return false;

	if (!lhs)
		return true;

	return *lhs == *rhs;
}

template<typename T, bool UseInlineStorage1, typename U, bool UseInlineStorage2>
CHANNELS_NODISCARD constexpr bool operator!=(
	const cow_optional<T, UseInlineStorage1>& lhs, const cow_optional<U, UseInlineStorage2>& rhs)
{
	if (static_cast<bool>(lhs) != static_cast<bool>(rhs))
		return true;

	if (!lhs)
		return false;

	return *lhs != *rhs;
}

template<typename T, bool UseInlineStorage1, typename U, bool UseInlineStorage2>
CHANNELS_NODISCARD constexpr bool operator<(
	const cow_optional<T, UseInlineStorage1>& lhs, const cow_optional<U, UseInlineStorage2>& rhs)
{
	if (!rhs)
		return false;

	if (!lhs)
		return true;

	return *lhs < *rhs;
}

template<typename T, bool UseInlineStorage1, typename U, bool UseInlineStorage2>
CHANNELS_NODISCARD constexpr bool operator>(
	const cow_optional<T, UseInlineStorage1>& lhs, const cow_optional<U, UseInlineStorage2>& rhs)
{
	if (!lhs)
		return false;

	if (!rhs)
		return true;

	return *lhs > *rhs;
}

template<typename T, bool UseInlineStorage1, typename U, bool UseInlineStorage2>
CHANNELS_NODISCARD constexpr bool operator<=(
	const cow_optional<T, UseInlineStorage1>& lhs, const cow_optional<U, UseInlineStorage2>& rhs)
{
	if (!lhs)
		return true;

	if (!rhs)
		return false;

	return *lhs <= *rhs;
}

template<typename T, bool UseInlineStorage1, typename U, bool UseInlineStorage2>
CHANNELS_NODISCARD constexpr bool operator>=(
	const cow_optional<T, UseInlineStorage1>& lhs, const cow_optional<U, UseInlineStorage2>& rhs)
{
	if (!rhs)
		return true;

	if (!lhs)
		return false;

	return *lhs >= * rhs;
}

// ### comparison with nullopt

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator==(const cow_optional<T, UseInlineStorage>& lhs, nullopt_t) noexcept
{
	return !lhs;
}

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator==(nullopt_t, const cow_optional<T, UseInlineStorage>& rhs) noexcept
{
	return !rhs;
}

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator!=(const cow_optional<T, UseInlineStorage>& lhs, nullopt_t) noexcept
{
	return static_cast<bool>(lhs);
}

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator!=(nullopt_t, const cow_optional<T, UseInlineStorage>& rhs) noexcept
{
	return static_cast<bool>(rhs);
}

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator<(const cow_optional<T, UseInlineStorage>&, nullopt_t) noexcept
{
	return false;
}

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator<(nullopt_t, const cow_optional<T, UseInlineStorage>& rhs) noexcept
{
	return static_cast<bool>(rhs);
}

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator<=(const cow_optional<T, UseInlineStorage>& lhs, nullopt_t) noexcept
{
	return !lhs;
}

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator<=(nullopt_t, const cow_optional<T, UseInlineStorage>&) noexcept
{
	return true;
}

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator>(const cow_optional<T, UseInlineStorage>& lhs, nullopt_t) noexcept
{
	return static_cast<bool>(lhs);
}

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator>(nullopt_t, const cow_optional<T, UseInlineStorage>&) noexcept
{
	return false;
}

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator>=(const cow_optional<T, UseInlineStorage>&, nullopt_t) noexcept
{
	return true;
}

template<typename T, bool UseInlineStorage>
CHANNELS_NODISCARD constexpr bool operator>=(nullopt_t, const cow_optional<T, UseInlineStorage>& rhs) noexcept
{
	return !rhs;
}

// ### comparison with T

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator==(const cow_optional<T, UseInlineStorage>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs == rhs : false;
}

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator==(const U& lhs, const cow_optional<T, UseInlineStorage>& rhs)
{
	return static_cast<bool>(rhs) ? lhs == *rhs : false;
}

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator!=(const cow_optional<T, UseInlineStorage>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs != rhs : true;
}

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator!=(const U& lhs, const cow_optional<T, UseInlineStorage>& rhs)
{
	return static_cast<bool>(rhs) ? lhs != *rhs : true;
}

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator<(const cow_optional<T, UseInlineStorage>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs < rhs : true;
}

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator<(const U& lhs, const cow_optional<T, UseInlineStorage>& rhs)
{
	return static_cast<bool>(rhs) ? lhs < *rhs : false;
}

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator<=(const cow_optional<T, UseInlineStorage>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs <= rhs : true;
}

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator<=(const U& lhs, const cow_optional<T, UseInlineStorage>& rhs)
{
	return static_cast<bool>(rhs) ? lhs <= *rhs : false;
}

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator>(const cow_optional<T, UseInlineStorage>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs > rhs : false;
}

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator>(const U& lhs, const cow_optional<T, UseInlineStorage>& rhs)
{
	return static_cast<bool>(rhs) ? lhs > *rhs : true;
}

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator>=(const cow_optional<T, UseInlineStorage>& lhs, const U& rhs)
{
	return static_cast<bool>(lhs) ? *lhs >= rhs : false;
}

template<typename T, bool UseInlineStorage, typename U>
CHANNELS_NODISCARD constexpr bool operator>=(const U& lhs, const cow_optional<T, UseInlineStorage>& rhs)
{
	return static_cast<bool>(rhs) ? lhs >= *rhs : true;
}

// ## specialized algorithms

template<typename T, bool UseInlineStorage = cow_use_inline_storage_v<T>>
CHANNELS_NODISCARD constexpr cow_optional<std::decay_t<T>, UseInlineStorage> make_cow_optional(T&& v)
{
	return cow_optional<std::decay_t<T>, UseInlineStorage>(std::forward<T>(v));
}

template<typename T, bool UseInlineStorage = cow_use_inline_storage_v<T>, typename...Args>
CHANNELS_NODISCARD constexpr cow_optional<T, UseInlineStorage> make_cow_optional(Args&&... args)
{
	return cow_optional<T, UseInlineStorage>(in_place, std::forward<Args>(args)...);
}

template<typename T, bool UseInlineStorage = cow_use_inline_storage_v<T>, typename U, typename... Args>
CHANNELS_NODISCARD constexpr cow_optional<T, UseInlineStorage> make_cow_optional(
	std::initializer_list<U> ilist, Args&&... args)
{
	return cow_optional<T, UseInlineStorage>(in_place, ilist, std::forward<Args>(args)...);
}

} // namespace detail
} // namespace channels

namespace std {

template<typename T, bool UseInlineStorage>
struct hash<channels::detail::cow_optional<T, UseInlineStorage>> {
	static_assert(
		std::is_default_constructible<std::hash<std::remove_const_t<T>>>::value, "For T must be declared hash");

	CHANNELS_NODISCARD std::size_t operator()(
		const channels::detail::cow_optional<T, UseInlineStorage>& co) const noexcept
	{
		return co ? hash<std::remove_const_t<T>>()(*co) : 0;
	}
};

} // namespace std
