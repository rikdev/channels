#pragma once
#include "compatibility/compile_features.h"
#include <cassert>
#include <iterator>
#include <type_traits>
#include <utility>

namespace channels {
namespace detail {

template<typename Base, typename T>
class cast_iterator;

// This class casts values from Base::value_type to T.
// It is designed to restore the types of objects returned by erase-types containers
// (like channels::detail::intrusive_list).
template<typename Base, typename T>
class CHANNELS_NODISCARD cast_view {
public:
	using value_type = std::remove_cv_t<T>;
	using pointer = T*;
	using reference = T&;
	using iterator = cast_iterator<decltype(std::begin(std::declval<Base>())), T>;
	using const_pointer = const T*;
	using const_reference = const T&;
	using const_iterator = cast_iterator<decltype(std::begin(std::declval<const Base>())), const T>;
	using difference_type = typename Base::difference_type;

	cast_view() = default;
	constexpr explicit cast_view(Base base);

	CHANNELS_NODISCARD constexpr iterator begin();
	CHANNELS_NODISCARD constexpr iterator end();
	CHANNELS_NODISCARD constexpr const_iterator begin() const;
	CHANNELS_NODISCARD constexpr const_iterator end() const;
	CHANNELS_NODISCARD constexpr const_iterator cbegin() const;
	CHANNELS_NODISCARD constexpr const_iterator cend() const;

	CHANNELS_NODISCARD constexpr bool empty() const;

private:
	Base base_;
};

template<typename Base, typename T>
class cast_iterator {
	template<typename UBase, typename U>
	friend class cast_iterator;

	template<typename LBase, typename L, typename RBase, typename R>
	friend constexpr bool operator==(cast_iterator<LBase, L> lhs, cast_iterator<RBase, R> rhs); // NOLINT
	template<typename LBase, typename L, typename RBase, typename R>
	friend constexpr bool operator!=(cast_iterator<LBase, L> lhs, cast_iterator<RBase, R> rhs); // NOLINT

public:
	using iterator_type = Base;
	using value_type = std::remove_cv_t<T>;
	using pointer = T*;
	using reference = T&;
	using difference_type = typename Base::difference_type;
	using iterator_category = std::forward_iterator_tag;

	cast_iterator() = default;
	constexpr explicit cast_iterator(Base base);

	template<typename UBase>
	constexpr cast_iterator(const cast_iterator<UBase, value_type>& other); // NOLINT: Allow implicit conversion
	template<typename UBase>
	constexpr cast_iterator& operator=(const cast_iterator<UBase, value_type>& other);

	constexpr cast_iterator& operator++();
	constexpr cast_iterator operator++(int);

	constexpr pointer operator->() const;
	constexpr reference operator*() const;

private:
	Base base_;
};

// implementation

// cast_view

template<typename Base, typename T>
constexpr cast_view<Base, T>::cast_view(Base base)
	: base_{std::move(base)}
{
#ifndef NDEBUG
	for (auto& item : base_) {
		assert(dynamic_cast<T*>(std::addressof(item)));
	}
#endif
}

template<typename Base, typename T>
constexpr typename cast_view<Base, T>::iterator cast_view<Base, T>::begin()
{
	return iterator{base_.begin()};
}

template<typename Base, typename T>
constexpr typename cast_view<Base, T>::iterator cast_view<Base, T>::end()
{
	return iterator{base_.end()};
}

template<typename Base, typename T>
constexpr typename cast_view<Base, T>::const_iterator cast_view<Base, T>::begin() const
{
	return const_iterator{base_.begin()};
}

template<typename Base, typename T>
constexpr typename cast_view<Base, T>::const_iterator cast_view<Base, T>::end() const
{
	return const_iterator{base_.end()};
}

template<typename Base, typename T>
constexpr typename cast_view<Base, T>::const_iterator cast_view<Base, T>::cbegin() const
{
	return const_iterator{base_.cbegin()};
}

template<typename Base, typename T>
constexpr typename cast_view<Base, T>::const_iterator cast_view<Base, T>::cend() const
{
	 return const_iterator{base_.cend()};
}

 template<typename Base, typename T>
 constexpr bool cast_view<Base, T>::empty() const
 {
	 return base_.empty();
 }

// cast_iterator

template<typename Base, typename T>
constexpr cast_iterator<Base, T>::cast_iterator(Base base)
	: base_{base}
{}

template<typename Base, typename T>
template<typename UBase>
constexpr cast_iterator<Base, T>::cast_iterator(const cast_iterator<UBase, value_type>& other)
	: base_{other.base_}
{}

template<typename Base, typename T>
template<typename UBase>
constexpr cast_iterator<Base, T>& cast_iterator<Base, T>::operator=(const cast_iterator<UBase, value_type>& other)
{
	base_ = other.base_;
	return *this;
}

template<typename Base, typename T>
constexpr cast_iterator<Base, T>& cast_iterator<Base, T>::operator++()
{
	++base_;
	return *this;
}

template<typename Base, typename T>
constexpr cast_iterator<Base, T> cast_iterator<Base, T>::operator++(int)
{
	return cast_iterator<Base, T>(base_++);
}

template<typename Base, typename T>
constexpr typename cast_iterator<Base, T>::pointer cast_iterator<Base, T>::operator->() const
{
	return static_cast<pointer>(base_.operator->());
}

template<typename Base, typename T>
constexpr typename cast_iterator<Base, T>::reference cast_iterator<Base, T>::operator*() const
{
	return static_cast<reference>(*base_);
}

template<typename LBase, typename L, typename RBase, typename R>
CHANNELS_NODISCARD constexpr bool operator==(const cast_iterator<LBase, L> lhs, const cast_iterator<RBase, R> rhs)
{
	return lhs.base_ == rhs.base_;
}

template<typename LBase, typename L, typename RBase, typename R>
CHANNELS_NODISCARD constexpr bool operator!=(const cast_iterator<LBase, L> lhs, const cast_iterator<RBase, R> rhs)
{
	return !(lhs == rhs);
}

} // namespace detail
} // namespace channels
