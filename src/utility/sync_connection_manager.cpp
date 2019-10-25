#include "utility/sync_connection_manager.h"

namespace channels {
inline namespace utility {

sync_connection_manager::~sync_connection_manager() noexcept
{
	sync_release();
}

void sync_connection_manager::sync_release() noexcept
{
	connection_manager_.release();
	tracker_.sync_release();
}

const sync_tracker& sync_connection_manager::get_tracker() const noexcept
{
	return tracker_;
}

} // namespace utility
} // namespace channels
