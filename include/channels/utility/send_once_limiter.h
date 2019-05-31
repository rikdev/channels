#pragma once
#include "../channel_traits.h"
#include "../error.h"
#include <atomic>
#include <utility>

namespace channels {
namespace utility {

/// This class limits the number of calls to the transmitter to one.
/// \note You can write your user-defined limiters using this class as an example and combine them to add constraints
///       to the channels: `channels::transmitter<limiter1<limiter2<channel_type>>>`.
///
/// Example:
/// \code
/// using canceller_type = channels::transmitter<channels::utility::send_once_limiter<channels::buffered_channel<>>>;
/// canceller_type canceller; // canceller must emit a signal only once
/// ...
/// canceller(); // emits signal
/// canceller(); // throws channels::transmitter_error
/// \endcode
template<typename Channel>
class send_once_limiter : public Channel {
	using base_type = Channel;

protected:
	using base_type::base_type;

	/// First call invokes base_type::apply_value next calls throws channels::transmitter_error
	/// \note This method is thread safe.
	/// \throw channels::transmitter_error If this method is called more than once
	template<typename... Args>
	decltype(auto) apply_value(Args&&... value);

private:
	std::atomic_flag sended_ = ATOMIC_FLAG_INIT;
};

// implementation

template<typename Channel>
template<typename... Args>
decltype(auto) send_once_limiter<Channel>::apply_value(Args&&... value)
{
	if (sended_.test_and_set(std::memory_order_relaxed))
		throw transmitter_error{"send_once_limiter: out of sends limit"};

	return base_type::apply_value(std::forward<Args>(value)...);
}

} // namespace utility

template<typename Channel>
struct channel_traits<utility::send_once_limiter<Channel>> : channel_traits<Channel> {};

} // namespace channels
