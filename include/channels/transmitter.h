#pragma once
#include "channel_traits.h"
#include <utility>

namespace channels {

/// A transmitter is a channel that can send values.
/// \tparam Channel A base channel type.
/// \see channels::channel
template<typename Channel>
class transmitter : public Channel {
	static_assert(is_channel_v<Channel>, "Channel must be channel");
	using base_type = Channel;

public:
	/// Default constructor. Constructs a `transmitter` object with shared state.
	transmitter();

	transmitter(const transmitter&) = delete;
	transmitter(transmitter&&) = default;
	transmitter& operator=(const transmitter&) = delete;
	transmitter& operator=(transmitter&&) = default;

	~transmitter() = default;

	/// Sends args to the channel.
	/// \note It just calls the method Channel::apply_value
	/// \note This method is thread safe.
	template<typename... Args>
	decltype(auto) operator()(Args&&... args);
};

// implementation

template<typename Channel>
struct channel_traits<transmitter<Channel>> : channel_traits<Channel> {};

template<typename Channel>
transmitter<Channel>::transmitter()
	: base_type{typename base_type::make_shared_state_tag{}}
{}

template<typename Channel>
template<typename... Args>
decltype(auto) transmitter<Channel>::operator()(Args&&... args)
{
	return this->apply_value(std::forward<Args>(args)...);
}

} // namespace channels
