#pragma once

namespace channels {

/// Traits class defining properties of channels.
template<typename Channel>
struct channel_traits {
	static constexpr bool is_channel = false;
};

template<typename Channel>
constexpr bool is_channel_v = channel_traits<Channel>::is_channel;

} // namespace channels
