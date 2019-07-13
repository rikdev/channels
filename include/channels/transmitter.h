#pragma once
#include "channel_traits.h"
#include "detail/compatibility/compile_features.h"
#include <utility>

namespace channels {

/// A class `transmitter` is the side of the channel that sends the values (like `std::promise`).
/// \tparam Channel A type of channel object that the transmitter creates in its constructor
///                 and to which it sends values.
/// \see channels::channel
///
/// Example:
/// \code
/// using channel_type = channels::channel<int>;
/// channel_type channel;
/// assert( ! channel.is_valid());
///
/// channels::transmitter<channel_type> transmitter;
/// channel = transmitter.get_channel();
/// assert(channel.is_valid());
///
/// channels::connection c1 = channel.connect([](int v) { .... });
/// channels::connection c2 = transmitter.get_channel().connect([](int v) { .... });
/// transmitter(42);
/// \endcode
///
/// \note All copies of one transmitter link to one channel.
/// \code
/// channels::transmitter<channels::channel<>> t1;
/// auto t2 = t1;
/// channels::connection c = t2.get_channel().connect([] { .... });
/// t1(); // it will call the callback function connected to the channel from the t2
/// \endcode
template<typename Channel>
class transmitter {
public:
	using channel_type = Channel;

	/// Sends args to the channel.
	/// \note This method is thread safe.
	template<typename... Args>
	decltype(auto) operator()(Args&&... args);

	/// \return Reference to the channel object. This channel object is always valid.
	/// \note This method is thread safe.
	CHANNELS_NODISCARD const Channel& get_channel() const noexcept;

private:
	class transmit_channel;

	transmit_channel channel_;
};

// implementation

// transmitter

template<typename Channel>
template<typename... Args>
decltype(auto) transmitter<Channel>::operator()(Args&&... args)
{
	return channel_(std::forward<Args>(args)...);
}

template<typename Channel>
const Channel& transmitter<Channel>::get_channel() const noexcept
{
	return channel_;
}

// transmitter::transmit_channel

/// A `transmit_channel` is a channel that can send values.
template<typename Channel>
class transmitter<Channel>::transmit_channel : public Channel {
	static_assert(is_channel_v<Channel>, "Channel must be channel");
	using base_type = Channel;

public:
	/// Default constructor. Constructs a `transmit_channel` object with shared state.
	transmit_channel()
		: base_type{typename base_type::make_shared_state_tag{}}
	{}

	/// Sends args to the channel.
	/// \note It just calls the method Channel::apply_value
	/// \note This method is thread safe.
	template<typename... Args>
	decltype(auto) operator()(Args&&... args)
	{
		return this->apply_value(std::forward<Args>(args)...);
	}
};

} // namespace channels
