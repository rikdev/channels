#pragma once
#include "../channel_traits.h"
#include <tuple>
#include <type_traits>
#include <utility>

namespace channels {
inline namespace utility {

/// This class is a wrapper for a `buffered_channel` that skips sending args to the channel if its equals to the last
/// args sent.
///
/// Example:
/// \code
/// channels::transmitter<channels::utility::new_only_limiter<channels::buffered_channel<int>>> int_transmitter;
/// ...
/// int_transmitter(1); // emits signal
/// int_transmitter(1); // skips to emit signal
/// int_transmitter(2); // emits signal
/// int_transmitter(3); // emits signal
/// \endcode
template<typename Channel>
class new_only_limiter : public Channel {
	using base_type = Channel;

protected:
	using base_type::base_type;

	/// Invokes base_type::send if `value` argument isn't equal to `base_type::get_value()` result.
	/// \warning This class isn't thread-safe.
	template<typename... Args>
	std::enable_if_t<is_applicable_v<Channel, Args...>> send(Args&&... value);
};

// implementation

template<typename Channel>
template<typename... Args>
std::enable_if_t<is_applicable_v<Channel, Args...>> new_only_limiter<Channel>::send(Args&&... value)
{
	if (this->get_value() == std::tuple<std::add_const_t<std::add_lvalue_reference_t<Args>>...>{value...})
		return;

	base_type::send(std::forward<Args>(value)...);
}

} // namespace utility

template<typename Channel>
struct channel_traits<utility::new_only_limiter<Channel>> : channel_traits<Channel> {};

} // namespace channels
