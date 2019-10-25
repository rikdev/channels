#pragma once
#include "../channel_traits.h"
#include "../connection.h"
#include "../detail/compatibility/compile_features.h"
#include "../detail/compatibility/type_traits.h"
#include <forward_list>
#include <utility>

namespace channels {
inline namespace utility {

/// This class just keeps `channels::connection` objects.
class connection_manager {
public:
	/// Connects the `callback` to the `channel`.
	/// \see channels::channel::connect
	/// \return Reference to connection object. After calling this method, the old references remain valid.
	template<typename Channel, typename Callback>
	connection& connect(const Channel& channel, Callback&& callback);

	/// Connects the `callback` to the `channel`.
	/// \see channels::channel::connect
	/// \return Reference to connection object. After calling this method, the old references remain valid.
	template<typename Channel, typename Executor, typename Callback>
	connection& connect(const Channel& channel, Executor&& executor, Callback&& callback);

	/// Removes all connections.
	/// \warning After call this method all references to connection objects are invalid.
	void release() noexcept;

private:
	CHANNELS_NODISCARD connection& add_connection(connection&& connection);

	std::forward_list<connection> connections_;
};

// implementation

template<typename Channel, typename Callback>
connection& connection_manager::connect(const Channel& channel, Callback&& callback)
{
	static_assert(is_channel_v<detail::compatibility::remove_cvref_t<Channel>>, "Channel type must be channel");

	return add_connection(channel.connect(std::forward<Callback>(callback)));
}

template<typename Channel, typename Executor, typename Callback>
connection& connection_manager::connect(
	const Channel& channel, Executor&& executor, Callback&& callback)
{
	static_assert(is_channel_v<detail::compatibility::remove_cvref_t<Channel>>, "Channel type must be channel");

	return add_connection(channel.connect(std::forward<Executor>(executor), std::forward<Callback>(callback)));
}

} // namespace utility
} // namespace channels
