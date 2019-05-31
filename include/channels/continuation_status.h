#pragma once

namespace channels {

/// Result returned by aggregator methods.
/// \see channels::aggregating_channel::apply_value
enum class continuation_status {
	to_continue,
	stop,
};

} // namespace channels
