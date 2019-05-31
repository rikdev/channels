#include "detail/shared_state_base.h"
#include <cassert>
#include <utility>

namespace channels {
namespace detail {

// ## socket_base

void socket_base::set_blocked(const bool blocked) noexcept
{
	blocked_.store(blocked, std::memory_order_relaxed);
}

bool socket_base::is_blocked() const noexcept
{
	return blocked_.load(std::memory_order_relaxed);
}

// ## shared_state_base

void channels::detail::shared_state_base::remove(socket_base* const socket)
{
	assert(socket); // NOLINT

	socket->set_blocked(true);

	const sockets_unique_lock_type sockets_lock{sockets_mutex_};
	sockets_.erase(socket);
}

shared_state_base::sockets_unique_lock_type shared_state_base::add(std::shared_ptr<socket_base> socket)
{
	assert(socket); // NOLINT

	sockets_unique_lock_type sockets_lock{sockets_mutex_};
	sockets_.push_back(std::move(socket));

	return sockets_lock;
}

shared_state_base::sockets_lock_view_type shared_state_base::get_sockets()
{
	sockets_shared_lock_type sockets_lock{sockets_mutex_};
	return sockets_lock_view_type{std::move(sockets_lock), sockets_};
}

} // namespace detail
} // namespace channels
