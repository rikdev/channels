#pragma once
#include "compatibility/compile_features.h"
#include "compatibility/shared_mutex.h"
#include "intrusive_list.h"
#include "lock_view.h"
#include "view.h"
#include <atomic>
#include <memory>
#include <mutex>

namespace channels {
namespace detail {

class socket_base : public intrusive_list::node {
public:
	void set_blocked(bool blocked) noexcept;
	CHANNELS_NODISCARD bool is_blocked() const noexcept;

private:
	std::atomic<bool> blocked_{false};
};

// This class is a part of the channels shared state that is independent of the channel template parameters.
// It provides resources for objects of type `channels::connection` (which are also independent of the channel template
// parameters) and for `channels::detail::shared_state`.
class shared_state_base {
	using sockets_container_type = intrusive_list;
	using sockets_mutex_type = compatibility::shared_mutex;
	using sockets_shared_lock_type = std::shared_lock<sockets_mutex_type>;

public:
	shared_state_base(const shared_state_base&) = delete;
	shared_state_base(shared_state_base&&) = delete;
	shared_state_base& operator=(const shared_state_base&) = delete;
	shared_state_base& operator=(shared_state_base&&) = delete;

	void remove(socket_base* socket);

protected:
	shared_state_base() = default;
	~shared_state_base() = default;

	using sockets_lock_view_type = lock_view<sockets_shared_lock_type, view<sockets_container_type>>;
	using sockets_unique_lock_type = std::unique_lock<sockets_mutex_type>;

	sockets_unique_lock_type add(std::shared_ptr<socket_base> socket);
	sockets_lock_view_type get_sockets();

private:
	mutable sockets_mutex_type sockets_mutex_;
	sockets_container_type sockets_;
};

} // namespace detail
} // namespace channels
