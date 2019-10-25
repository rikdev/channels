#pragma once
#include "../detail/compatibility/compile_features.h"
#include "connection_manager.h"
#include "executors.h"
#include "sync_tracker.h"
#include <utility>

namespace channels {
inline namespace utility {

/// This class keeps `channels::connection` objects and can synchronously disconnect its.
class sync_connection_manager {
public:
	sync_connection_manager() = default;

	/// Calls sync_release() and destructs this object.
	~sync_connection_manager() noexcept;

	sync_connection_manager(const sync_connection_manager&) = delete;
	sync_connection_manager& operator=(const sync_connection_manager&) = delete;
	sync_connection_manager(sync_connection_manager&&) = default;
	sync_connection_manager& operator=(sync_connection_manager&&) = default;

	/// Connects the `callback` to the `channel`.
	/// \see channels::channel::connect
	/// \return Reference to connection object. After calling this method, the old references remain valid.
	/// \throw tracker_error If the `sync_release` method was called for this object.
	template<typename Channel, typename Callback>
	connection& connect(const Channel& channel, Callback&& callback);

	/// Connects the `callback` to the `channel`.
	/// \see channels::channel::connect
	/// \return Reference to connection object. After calling this method, the old references remain valid.
	/// \throw tracker_error If the `sync_release` method was called for this object.
	template<typename Channel, typename Executor, typename Callback>
	connection& connect(const Channel& channel, Executor&& executor, Callback&& callback);

	/// Removes all connections and waits until all callbacks are completed.
	/// \warning After call this method all references to connection objects are invalid.
	void sync_release() noexcept;

	/// Returns a reference to `sync_tracker` object to control the execution of callbacks or integrate with
	/// own code.
	CHANNELS_NODISCARD const sync_tracker& get_tracker() const noexcept;

private:
	sync_tracker tracker_;
	connection_manager connection_manager_;
};

// implementation

template<typename Channel, typename Callback>
connection& sync_connection_manager::connect(const Channel& channel, Callback&& callback)
{
	return connection_manager_.connect(
		channel, make_tracking_executor(tracker_.get_tracked_object()), std::forward<Callback>(callback));
}

template<typename Channel, typename Executor, typename Callback>
connection& sync_connection_manager::connect(
	const Channel& channel, Executor&& executor, Callback&& callback)
{
	return connection_manager_.connect(
		channel,
		make_tracking_executor(tracker_.get_tracked_object(), std::forward<Executor>(executor)),
		std::forward<Callback>(callback));
}

} // namespace utility
} // namespace channels
