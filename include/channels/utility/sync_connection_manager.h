#pragma once
#include "../channel_traits.h"
#include "../connection.h"
#include "../detail/compatibility/compile_features.h"
#include "callback.h"
#include "sync_tracker.h"
#include <forward_list>
#include <type_traits>
#include <utility>

namespace channels {
namespace utility {

/// This class keeps `channels::connection` objects and can synchronously disconnect its.
class sync_connection_manager {
public:
	/// Connects the `callback` to the `channel`.
	/// \see channels::channel::connect
	/// \return Reference to connection object. After calling this method, the old references remain valid.
	/// \throw tracker_error If the `sync_release` method was called for this object.
	template<typename Channel, typename Callback>
	connection& connect(Channel& channel, Callback&& callback);

	/// Connects the `callback` to the `channel`.
	/// \see channels::channel::connect
	/// \return Reference to connection object. After calling this method, the old references remain valid.
	/// \throw tracker_error If the `sync_release` method was called for this object.
	template<typename Channel, typename Executor, typename Callback>
	connection& connect(Channel& channel, Executor&& executor, Callback&& callback);

	/// Removes all connections and waits until all callbacks are completed.
	/// \warning After call this method all references to connection objects are invalid.
	void sync_release() noexcept;

	CHANNELS_NODISCARD const sync_tracker& get_tracker() const noexcept;

private:
	connection& add_connection(connection&& connection);

	std::forward_list<connection> connections_;
	sync_tracker tracker_; // in the destructor the tracker should be destroyed first
};

// implementation

template<typename Channel, typename Callback>
connection& sync_connection_manager::connect(Channel& channel, Callback&& callback)
{
	static_assert(is_channel_v<std::remove_reference_t<Channel>>, "Channel type must be channel");

	connection c =
		channel.connect(make_tracking_callback(tracker_.get_tracked_object(), std::forward<Callback>(callback)));
	return add_connection(std::move(c));
}

template<typename Channel, typename Executor, typename Callback>
connection& sync_connection_manager::connect(
	Channel& channel, Executor&& executor, Callback&& callback)
{
	static_assert(is_channel_v<std::remove_reference_t<Channel>>, "Channel type must be channel");

	connection c = channel.connect(
		std::forward<Executor>(executor),
		make_tracking_callback(tracker_.get_tracked_object(), std::forward<Callback>(callback)));
	return add_connection(std::move(c));
}

inline void sync_connection_manager::sync_release() noexcept
{
	tracker_.sync_release();
	connections_.clear();
}

inline const sync_tracker& sync_connection_manager::get_tracker() const noexcept
{
	return tracker_;
}

inline connection& sync_connection_manager::add_connection(connection&& connection)
{
	return *connections_.insert_after(connections_.before_begin(), std::move(connection));
}

} // namespace utility
} // namespace channels
