#pragma once
#include "compatibility/compile_features.h"
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>

namespace channels {
namespace detail {

namespace intrusive_list_detail {

class node;

template<typename T>
class iterator;

template<typename T>
class reverse_iterator;

} // namespace intrusive_list_detail

// This class implements a list to efficiently store objects of different types with common base.
class intrusive_list {
public:
	// Base class for all types of objects stored in the list
	using node = intrusive_list_detail::node;
	using owner_pointer = std::shared_ptr<node>;

	using value_type = owner_pointer;
	using pointer = node*;
	using reference = node&;
	using const_pointer = const node*;
	using const_reference = const node&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using iterator = intrusive_list_detail::iterator<node>;
	using const_iterator = intrusive_list_detail::iterator<const node>;
	using reverse_iterator = intrusive_list_detail::reverse_iterator<node>;
	using const_reverse_iterator = intrusive_list_detail::reverse_iterator<const node>;

	intrusive_list() = default;
	// \see insert
	intrusive_list(std::initializer_list<value_type> ilist) noexcept;

	intrusive_list(const intrusive_list&) = delete;
	intrusive_list& operator=(const intrusive_list&) = delete;
	intrusive_list(intrusive_list&&) = default;
	intrusive_list& operator=(intrusive_list&&) = default;

	~intrusive_list() = default;

	// \see insert
	void push_back(value_type value) noexcept;
	void pop_back() noexcept;
	// \see insert
	void push_front(value_type value) noexcept;
	void pop_front() noexcept;

	// Inserts item to list.
	// \param position Iterator before which the content will be inserted. Position may be the end() iterator.
	// \pre The behavior is undefined if position inserted to another list.
	// \param item Pointer to the object to be inserted.
	// \pre The behavior is undefined if item is null.
	// \pre The behavior is undefined if item inserted to another list.
	// \return Iterator pointing to the inserted value.
	iterator insert(iterator position, value_type item) noexcept;

	// Removes item from list.
	// \param position Iterator to the object to remove.
	// \pre The behavior is undefined if position == end().
	// \pre The behavior is undefined if position inserted to another list.
	// \return Iterator following the last removed element.
	iterator erase(iterator position) noexcept;

	// Inserts item to list.
	// \param position Pointer to the object before which item will be inserted.
	//                 If position is null item will be inserted after last object.
	// \pre The behavior is undefined if position is not inserted or inserted to another list.
	// \param item Pointer to the object to be inserted.
	// \pre The behavior is undefined if item is null.
	// \pre The behavior is undefined if item inserted to another list.
	void insert(pointer position, owner_pointer item) noexcept;

	// Removes item from list.
	// \param position Pointer to the object to remove.
	// \pre The behavior is undefined if position is null.
	// \pre The behavior is undefined if position inserted to another list.
	void erase(pointer position) noexcept;

	void clear() noexcept;

	void swap(intrusive_list& other) noexcept;

	CHANNELS_NODISCARD reference front() noexcept;
	CHANNELS_NODISCARD reference back() noexcept;
	CHANNELS_NODISCARD const_reference front() const noexcept;
	CHANNELS_NODISCARD const_reference back() const noexcept;

	CHANNELS_NODISCARD iterator begin() noexcept;
	CHANNELS_NODISCARD iterator end() noexcept;
	CHANNELS_NODISCARD const_iterator begin() const noexcept;
	CHANNELS_NODISCARD const_iterator end() const noexcept;
	CHANNELS_NODISCARD const_iterator cbegin() const noexcept;
	CHANNELS_NODISCARD const_iterator cend() const noexcept;

	CHANNELS_NODISCARD reverse_iterator rbegin() noexcept;
	CHANNELS_NODISCARD reverse_iterator rend() noexcept;
	CHANNELS_NODISCARD const_reverse_iterator rbegin() const noexcept;
	CHANNELS_NODISCARD const_reverse_iterator rend() const noexcept;
	CHANNELS_NODISCARD const_reverse_iterator crbegin() const noexcept;
	CHANNELS_NODISCARD const_reverse_iterator crend() const noexcept;

	CHANNELS_NODISCARD size_type size() const noexcept;
	CHANNELS_NODISCARD bool empty() const noexcept;

private:
	CHANNELS_NODISCARD bool contains(const_pointer item_ptr) const noexcept;

	owner_pointer first_;
	pointer last_{nullptr};
	size_type size_{0};
};

void swap(intrusive_list& lhs, intrusive_list& rhs) noexcept;

namespace intrusive_list_detail {

class node {
	friend intrusive_list;

	template<typename>
	friend class iterator;
	template<typename>
	friend class reverse_iterator;

public:
	node() = default;

	node(const node&) = delete;
	node(node&&) = delete;
	node& operator=(const node&) = delete;
	node& operator=(node&&) = delete;

	virtual ~node() = default;

private:
	using pointer = intrusive_list::pointer;
	using owner_pointer = intrusive_list::owner_pointer;

	owner_pointer next_;
	pointer prev_{nullptr};
};

template<typename T>
class iterator { // NOLINT(cppcoreguidelines-special-member-functions)
	template<typename>
	friend class iterator;

	friend intrusive_list;

	template<typename L, typename R>
	friend bool operator==(iterator<L> lhs, iterator<R> rhs) noexcept; // NOLINT
	template<typename L, typename R>
	friend bool operator!=(iterator<L> lhs, iterator<R> rhs) noexcept; // NOLINT

public:
	using value_type = std::remove_const_t<T>;
	using pointer = T*;
	using reference = T&;
	using difference_type = intrusive_list::difference_type;
	using iterator_category = std::forward_iterator_tag;

	iterator() = default;

	explicit iterator(const pointer item) noexcept
		: item_{item}
	{}

	iterator(const iterator<value_type>& other) noexcept // NOLINT: Allow implicit conversion
		: item_{other.item_}
	{}

	iterator& operator=(const iterator<value_type>& other) noexcept
	{
		item_ = other.item_;
		return *this;
	}

	iterator& operator++() noexcept
	{
		item_ = item_->next_.get();
		return *this;
	}

	iterator operator++(int) noexcept
	{
		const pointer tmp = item_;
		++(*this);
		return iterator{tmp};
	}

	CHANNELS_NODISCARD pointer operator->() const noexcept
	{
		return item_;
	}

	CHANNELS_NODISCARD reference operator*() const noexcept
	{
		return *item_;
	}

private:
	pointer item_{nullptr};
};

template<typename T>
class reverse_iterator { // NOLINT(cppcoreguidelines-special-member-functions)
	template<typename>
	friend class reverse_iterator;

	template<typename L, typename R>
	friend bool operator==(reverse_iterator<L> lhs, reverse_iterator<R> rhs) noexcept; // NOLINT
	template<typename L, typename R>
	friend bool operator!=(reverse_iterator<L> lhs, reverse_iterator<R> rhs) noexcept; // NOLINT

public:
	using value_type = std::remove_const_t<T>;
	using pointer = T*;
	using reference = T&;
	using difference_type = intrusive_list::difference_type;
	using iterator_category = std::forward_iterator_tag;

	reverse_iterator() = default;

	explicit reverse_iterator(const pointer item) noexcept
		: item_{item}
	{}

	reverse_iterator(const reverse_iterator<value_type>& other) noexcept // NOLINT: Allow implicit conversion
		: item_{other.item_}
	{}

	reverse_iterator& operator=(const reverse_iterator<value_type>& other) noexcept
	{
		item_ = other.item_;
		return *this;
	}

	reverse_iterator& operator++() noexcept
	{
		item_ = item_->prev_;
		return *this;
	}

	reverse_iterator operator++(int) noexcept
	{
		const pointer tmp = item_;
		++(*this);
		return reverse_iterator{tmp};
	}

	CHANNELS_NODISCARD pointer operator->() const noexcept
	{
		return item_;
	}

	CHANNELS_NODISCARD reference operator*() const noexcept
	{
		return *item_;
	}

private:
	pointer item_{nullptr};
};

// implementation

// iterator

template<typename L, typename R>
CHANNELS_NODISCARD bool operator==(const iterator<L> lhs, const iterator<R> rhs) noexcept
{
	return lhs.item_ == rhs.item_;
}

template<typename L, typename R>
CHANNELS_NODISCARD bool operator!=(const iterator<L> lhs, const iterator<R> rhs) noexcept
{
	return !(lhs == rhs);
}

// reverse_iterator

template<typename L, typename R>
CHANNELS_NODISCARD bool operator==(const reverse_iterator<L> lhs, const reverse_iterator<R> rhs) noexcept
{
	return lhs.item_ == rhs.item_;
}

template<typename L, typename R>
CHANNELS_NODISCARD bool operator!=(const reverse_iterator<L> lhs, const reverse_iterator<R> rhs) noexcept
{
	return !(lhs == rhs);
}

} // namespace intrusive_list_detail

} // namespace detail
} // namespace channels
