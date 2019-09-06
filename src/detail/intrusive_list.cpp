#include "detail/intrusive_list.h"
#include <algorithm>
#include <cassert>
#include <utility>

namespace channels {
namespace detail {

// intrusive_list

intrusive_list::intrusive_list(const std::initializer_list<value_type> ilist) noexcept
{
	for (const value_type& i : ilist)
		push_back(i);
}

void intrusive_list::push_back(value_type value) noexcept
{
	insert(nullptr, std::move(value));
}

void intrusive_list::pop_back() noexcept
{
	erase(last_);
}

void intrusive_list::push_front(value_type value) noexcept
{
	insert(first_.get(), std::move(value));
}

void intrusive_list::pop_front() noexcept
{
	erase(first_.get());
}

intrusive_list::iterator intrusive_list::insert(const iterator position, value_type item) noexcept
{
	iterator result{item.get()};
	insert(position.item_, std::move(item));
	return result;
}

intrusive_list::iterator intrusive_list::erase(const iterator position) noexcept
{
	iterator result = std::next(position);
	erase(position.item_);
	return result;
}

void intrusive_list::insert(const pointer position, owner_pointer item) noexcept
{
	assert(item); // NOLINT
	// item_ptr must not be added
	assert(!item->prev_); // NOLINT
	assert(!item->next_); // NOLINT
	assert(!contains(item.get())); // NOLINT
	// if position exists then it must be added
	assert(!position || contains(position)); // NOLINT

	++size_;

	pointer insert_after = nullptr;
	if (position) {
		insert_after = position->prev_;
	}
	else {
		insert_after = last_;
		last_ = item.get();
	}

	if (insert_after) {
		item->prev_ = insert_after;
		item->next_ = std::move(insert_after->next_);
		insert_after->next_ = std::move(item);
	}
	else {
		item->next_ = std::move(first_);
		first_ = std::move(item);
	}
}

void intrusive_list::erase(const pointer position) noexcept
{
	assert(size_ > 0); // NOLINT
	assert(position); // NOLINT
	// item_ptr must be added
	assert(contains(position)); // NOLINT

	--size_;

	const pointer next_item = position->next_.get();
	const pointer prev_item = position->prev_;

	if (next_item)
		next_item->prev_ = prev_item;

	if (prev_item) {
		// keep the owner-pointer to item while we working with it
		const owner_pointer keeper = std::move(prev_item->next_);
		prev_item->next_ = std::move(position->next_);
		position->prev_ = nullptr;

	}
	else {
		// keep the owner-pointer to item_ptr while we working with it
		const owner_pointer keeper = std::move(first_);
		first_ = std::move(position->next_);
	}

	if (last_ == position)
		last_ = prev_item;
}

void intrusive_list::clear() noexcept
{
	*this = intrusive_list{};
}

void intrusive_list::swap(intrusive_list& other) noexcept
{
	std::swap(first_, other.first_);
	std::swap(last_, other.last_);
}

intrusive_list::reference intrusive_list::front() noexcept
{
	return *first_;
}

intrusive_list::reference intrusive_list::back() noexcept
{
	return *last_;
}

intrusive_list::const_reference intrusive_list::front() const noexcept
{
	return *first_;
}

intrusive_list::const_reference intrusive_list::back() const noexcept
{
	return *last_;
}

intrusive_list::iterator intrusive_list::begin() noexcept
{
	return iterator{first_.get()};
}

intrusive_list::iterator intrusive_list::end() noexcept
{
	return {};
}

intrusive_list::const_iterator intrusive_list::begin() const noexcept
{
	return const_iterator{first_.get()};
}

intrusive_list::const_iterator intrusive_list::end() const noexcept
{
	return {};
}

intrusive_list::const_iterator intrusive_list::cbegin() const noexcept
{
	return begin();
}

intrusive_list::const_iterator intrusive_list::cend() const noexcept
{
	return end();
}

intrusive_list::reverse_iterator intrusive_list::rbegin() noexcept
{
	return reverse_iterator{last_};
}

intrusive_list::reverse_iterator intrusive_list::rend() noexcept
{
	return {};
}

intrusive_list::const_reverse_iterator intrusive_list::rbegin() const noexcept
{
	return const_reverse_iterator{last_};
}

intrusive_list::const_reverse_iterator intrusive_list::rend() const noexcept
{
	return {};
}

intrusive_list::const_reverse_iterator intrusive_list::crbegin() const noexcept
{
	return rbegin();
}

intrusive_list::const_reverse_iterator intrusive_list::crend() const noexcept
{
	return rend();
}

intrusive_list::size_type intrusive_list::size() const noexcept
{
	return size_;
}

bool intrusive_list::empty() const noexcept
{
	return static_cast<bool>(first_);
}

bool intrusive_list::contains(const const_pointer item_ptr) const noexcept
{
	const bool result = std::any_of(begin(), end(), [item_ptr](const node& n) { return &n == item_ptr; });
	assert(std::any_of(rbegin(), rend(), [item_ptr](const node& n) { return &n == item_ptr; }) == result); // NOLINT

	return result;
}

void swap(intrusive_list& lhs, intrusive_list& rhs) noexcept
{
	lhs.swap(rhs);
}

} // namespace detail
} // namespace channels
