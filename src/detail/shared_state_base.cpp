#include "detail/shared_state_base.h"
#include <cassert>
#include <utility>

namespace channels {
namespace detail {

// socket_base

void socket_base::set_blocked(const bool blocked) noexcept
{
	blocked_.store(blocked, std::memory_order_relaxed);
}

bool socket_base::is_blocked() const noexcept
{
	return blocked_.load(std::memory_order_relaxed);
}

void socket_base::add_reference() noexcept
{
	++references_count_;
}

size_t socket_base::remove_reference() noexcept
{
	assert(references_count_ > 0); // NOLINT
	return --references_count_;
}

// shared_state_base

void channels::detail::shared_state_base::remove(socket_base& socket)
{
	socket.set_blocked(true);

	const sockets_unique_lock_type sockets_lock{sockets_mutex_};
	remove_reference(socket, sockets_lock);
}

void shared_state_base::add(std::shared_ptr<socket_base> socket)
{
	sockets_unique_lock_type sockets_lock{sockets_mutex_};
	sockets_.push_back(std::move(socket));
}

sockets_shared_view shared_state_base::get_sockets()
{
	sockets_shared_lock_type sockets_lock{sockets_mutex_};
	for (intrusive_list::node& socket : sockets_)
		static_cast<socket_base&>(socket).add_reference(); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

	return sockets_shared_view{*this, sockets_.begin(), sockets_.size()};
}

void shared_state_base::remove_reference(socket_base& socket, const sockets_unique_lock_type&) noexcept
{
	if (socket.remove_reference() == 0)
		sockets_.erase(&socket);
}

// sockets_shared_view

sockets_shared_view::sockets_shared_view(
	shared_state_base& shared_state, const base_type::base_iterator_type first_item, const base_type::size_type size)
	: base_type{first_item, size}
	, shared_state_{&shared_state}
{}

sockets_shared_view::sockets_shared_view(sockets_shared_view&& other) noexcept
	: base_type{std::move(other)}
	, shared_state_{other.shared_state_}
{
	other.shared_state_ = nullptr;
}

sockets_shared_view& sockets_shared_view::operator=(sockets_shared_view&& other) noexcept
{
	reset();

	shared_state_ = other.shared_state_;
	other.shared_state_ = nullptr;
	base_type::operator=(std::move(other));

	return *this;
}

sockets_shared_view::~sockets_shared_view() noexcept
{
	reset();
}

void sockets_shared_view::reset() noexcept
{
	if (!shared_state_)
		return;

	if (empty())
		return;

	const shared_state_base::sockets_unique_lock_type sockets_lock{shared_state_->sockets_mutex_};
	for (iterator it = begin(), next_it = it; it != end(); it = next_it) {
		++next_it;

		shared_state_->remove_reference(static_cast<socket_base&>(*it), sockets_lock); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
	}

	shared_state_ = nullptr;
}

} // namespace detail
} // namespace channels
