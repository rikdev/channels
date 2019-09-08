#include "utility/sync_connection_manager.h"

namespace channels {
inline namespace utility {

sync_connection_manager::~sync_connection_manager() noexcept
{
	sync_release();
}

void sync_connection_manager::sync_release() noexcept
{
	connections_.clear();
	tracker_.sync_release();
}

const sync_tracker& sync_connection_manager::get_tracker() const noexcept
{
	return tracker_;
}

connection& sync_connection_manager::add_connection(connection&& connection)
{
	return *connections_.insert_after(connections_.before_begin(), std::move(connection));
}

} // namespace utility
} // namespace channels
