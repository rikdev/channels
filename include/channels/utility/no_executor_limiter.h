#pragma once
#include "../channel_traits.h"
#include "../connection.h"

namespace channels {
inline namespace utility {

template<typename Channel>
class no_executor_limiter : public Channel {
	using base_type = Channel;

public:
	using base_type::connect;

	template <typename Executor, typename Callback>
	connection connect(Executor&& executor, Callback&& callback) = delete;

protected:
	using base_type::base_type;
};

} // namespace utility

template<typename Channel>
struct channel_traits<utility::no_executor_limiter<Channel>> : channel_traits<Channel> {};

} // namespace channels
