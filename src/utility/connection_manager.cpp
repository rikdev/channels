#include "utility/connection_manager.h"

namespace channels {
inline namespace utility {

void connection_manager::release() noexcept
{
	connections_.clear();
}

connection& connection_manager::add_connection(connection&& connection)
{
	return *connections_.insert_after(connections_.before_begin(), std::move(connection));
}

} // namespace utility
} // namespace channels
