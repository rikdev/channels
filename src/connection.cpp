#include "connection.h"
#include "detail/shared_state_base.h"
#include <cassert>
#include <utility>

namespace channels {

// connection

connection::connection(connection&& other) noexcept
	: shared_state_{std::move(other.shared_state_)}
	, socket_{other.socket_}
{
	other.socket_ = nullptr;
}

connection& connection::operator=(connection&& other) noexcept
{
	if (this == &other)
		return *this;

	disconnect();

	shared_state_ = std::move(other.shared_state_);
	socket_ = other.socket_;
	other.socket_ = nullptr;

	return *this;
}

connection::~connection() noexcept
{
	disconnect();
}

void connection::disconnect() noexcept
{
	const std::shared_ptr<detail::shared_state_base> shared_state = shared_state_.lock();
	if (!shared_state)
		return;

	shared_state->remove(socket_);

	shared_state_.reset();
	socket_ = nullptr;
}

bool connection::is_connected() const noexcept
{
	return !shared_state_.expired();
}

connection::connection(
	std::weak_ptr<detail::shared_state_base> shared_state, detail::socket_base* const socket) noexcept
	: shared_state_{std::move(shared_state)}
	, socket_{socket}
{
	assert(!shared_state_.expired()); // NOLINT
	assert(socket_); // NOLINT
}

} // namespace channels
