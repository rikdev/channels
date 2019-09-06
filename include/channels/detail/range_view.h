#pragma once
#include "compatibility/compile_features.h"
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace channels {
namespace detail {

template<typename Base>
class range_view_iterator;

template<typename Iterator>
class CHANNELS_NODISCARD range_view {
public:
	using base_iterator_type = Iterator;
	using element_type = std::remove_reference_t<decltype(*std::declval<base_iterator_type>())>;
	using value_type = std::remove_cv_t<element_type>;
	using pointer = element_type*;
	using reference = element_type&;
	using iterator = range_view_iterator<base_iterator_type>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	range_view() = default;
	explicit constexpr range_view(base_iterator_type first, size_type size);

	CHANNELS_NODISCARD constexpr iterator begin() const;
	CHANNELS_NODISCARD constexpr iterator end() const;
	CHANNELS_NODISCARD constexpr iterator cbegin() const;
	CHANNELS_NODISCARD constexpr iterator cend() const;

	CHANNELS_NODISCARD constexpr size_type size() const noexcept;
	CHANNELS_NODISCARD constexpr bool empty() const noexcept;

private:
	Iterator first_{};
	size_type size_{};
};

template<typename Base>
class range_view_iterator {
	template<typename UBase>
	friend class range_view_iterator;

	template<typename LBase, typename RBase>
	friend constexpr bool operator==(range_view_iterator<LBase> lhs, range_view_iterator<RBase> rhs); // NOLINT
	template<typename LBase, typename RBase>
	friend constexpr bool operator!=(range_view_iterator<LBase> lhs, range_view_iterator<RBase> rhs); // NOLINT

public:
	using iterator_type = Base;
	using element_type = std::remove_reference_t<decltype(*std::declval<iterator_type>())>;
	using value_type = std::remove_cv_t<element_type>;
	using pointer = element_type*;
	using reference = element_type&;
	using size_type = std::size_t;
	using difference_type = std::size_t;
	using iterator_category = std::forward_iterator_tag;

	range_view_iterator() = default;
	constexpr explicit range_view_iterator(Base base, size_type size);

	template<typename UBase>
	constexpr range_view_iterator(const range_view_iterator<UBase>& other); // NOLINT: Allow implicit conversion
	template<typename UBase>
	constexpr range_view_iterator& operator=(const range_view_iterator<UBase>& other);

	constexpr range_view_iterator& operator++();
	constexpr range_view_iterator operator++(int);

	constexpr pointer operator->() const;
	constexpr reference operator*() const;

private:
	Base base_{};
	size_type size_{};
};

// implementation

// range_view

template<typename Iterator>
constexpr range_view<Iterator>::range_view(base_iterator_type first, const size_type size)
	: first_{std::move(first)}
	, size_{size}
{}

template<typename Iterator>
constexpr typename range_view<Iterator>::iterator range_view<Iterator>::begin() const
{
	return iterator{first_, size_};
}

template<typename Iterator>
constexpr typename range_view<Iterator>::iterator range_view<Iterator>::end() const
{
	return iterator{};
}

template<typename Iterator>
constexpr typename range_view<Iterator>::iterator range_view<Iterator>::cbegin() const
{
	return begin();
}

template<typename Iterator>
constexpr typename range_view<Iterator>::iterator range_view<Iterator>::cend() const
{
	return end();
}

template<typename Iterator>
constexpr typename range_view<Iterator>::size_type range_view<Iterator>::size() const noexcept
{
	return size_;
}

template<typename Iterator>
constexpr bool range_view<Iterator>::empty() const noexcept
{
	return size() == 0;
}

// range_view_iterator

template<typename Base>
constexpr range_view_iterator<Base>::range_view_iterator(Base base, const size_type size)
	: base_{std::move(base)}
	, size_{size}
{}

template<typename Base>
template<typename UBase>
constexpr range_view_iterator<Base>::range_view_iterator(const range_view_iterator<UBase>& other)
	: base_{other.base_}
	, size_{other.size_}
{}

template<typename Base>
template<typename UBase>
constexpr range_view_iterator<Base>& range_view_iterator<Base>::operator=(const range_view_iterator<UBase>& other)
{
	base_ = other.base_;
	size_ = other.size_;

	return *this;
}

template<typename Base>
constexpr range_view_iterator<Base>& range_view_iterator<Base>::operator++()
{
	assert(size_ > 0); // NOLINT

	if (--size_ > 0)
		++base_;
	else
		base_ = iterator_type{};

	return *this;
}

template<typename Base>
constexpr range_view_iterator<Base> range_view_iterator<Base>::operator++(int)
{
	auto temp = *this;
	++(*this);

	return temp;
}

template<typename Base>
constexpr typename range_view_iterator<Base>::pointer range_view_iterator<Base>::operator->() const
{
	return base_.operator->();
}

template<typename Base>
constexpr typename range_view_iterator<Base>::reference range_view_iterator<Base>::operator*() const
{
	return *base_;
}

template<typename LBase, typename RBase>
CHANNELS_NODISCARD constexpr bool operator==(range_view_iterator<LBase> lhs, range_view_iterator<RBase> rhs)
{
	assert((lhs.size_ == rhs.size_) == (lhs.base_ == rhs.base_)); // NOLINT

	return lhs.size_ == rhs.size_ && lhs.base_ == rhs.base_;
}

template<typename LBase, typename RBase>
CHANNELS_NODISCARD constexpr bool operator!=(range_view_iterator<LBase> lhs, range_view_iterator<RBase> rhs)
{
	return !(lhs == rhs);
}

} // namespace detail
} // namespace channels
