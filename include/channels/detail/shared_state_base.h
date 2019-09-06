#pragma once
#include "compatibility/compile_features.h"
#include "intrusive_list.h"
#include "range_view.h"
#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>

namespace channels {
namespace detail {

class socket_base : public intrusive_list::node {
	friend class shared_state_base;

public:
	void set_blocked(bool blocked) noexcept;
	CHANNELS_NODISCARD bool is_blocked() const noexcept;

private:
	void add_reference() noexcept;
	CHANNELS_NODISCARD std::size_t remove_reference() noexcept;

	std::atomic<bool> blocked_{false};
	std::size_t references_count_{1};
};

class sockets_shared_view;

// This class is a part of the channels shared state that is independent of the channel template parameters.
// It provides resources for objects of type `channels::connection` (which are also independent of the channel template
// parameters) and for `channels::detail::shared_state`.
class shared_state_base {
public:
	shared_state_base(const shared_state_base&) = delete;
	shared_state_base(shared_state_base&&) = delete;
	shared_state_base& operator=(const shared_state_base&) = delete;
	shared_state_base& operator=(shared_state_base&&) = delete;

	void remove(socket_base& socket);

protected:
	friend class sockets_shared_view;

	shared_state_base() = default;
	~shared_state_base() = default;

	void add(std::shared_ptr<socket_base> socket);
	sockets_shared_view get_sockets();

private:
	using sockets_container_type = intrusive_list;
	using sockets_mutex_type = std::mutex;
	using sockets_unique_lock_type = std::lock_guard<sockets_mutex_type>;
	using sockets_shared_lock_type = sockets_unique_lock_type;

	void remove_reference(socket_base& socket, const sockets_unique_lock_type& lock) noexcept;

	mutable sockets_mutex_type sockets_mutex_;
	sockets_container_type sockets_;
};

class CHANNELS_NODISCARD sockets_shared_view : public range_view<shared_state_base::sockets_container_type::iterator> {
	using base_type = range_view<shared_state_base::sockets_container_type::iterator>;

public:
	sockets_shared_view() = default;
	sockets_shared_view(
		shared_state_base& shared_state,  base_type::base_iterator_type first_item, base_type::size_type size);

	sockets_shared_view(const sockets_shared_view&) = delete;
	sockets_shared_view(sockets_shared_view&& other) noexcept;

	sockets_shared_view& operator=(const sockets_shared_view&) = delete;
	sockets_shared_view& operator=(sockets_shared_view&& other) noexcept;

	~sockets_shared_view() noexcept;

private:
	void reset() noexcept;

	shared_state_base* shared_state_{};
};

} // namespace detail
} // namespace channels
