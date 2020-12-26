#pragma once
#include "detail/compatibility/type_traits.h"

namespace channels {

namespace channel_traits_detail {

template<typename Channel>
struct channel_accessor : Channel {
	using Channel::send;
};

} // namespace channel_traits_detail

/// Traits class defining properties of channels.
template<typename Channel>
struct channel_traits {
	static constexpr bool is_channel = false;
};

template<typename Channel>
constexpr bool is_channel_v = channel_traits<Channel>::is_channel; // NOLINT(misc-definitions-in-headers)

// is_applicable

template<typename Channel, typename... Args>
struct is_applicable : detail::compatibility::is_invocable<
	decltype(&channel_traits_detail::channel_accessor<Channel>::send),
	channel_traits_detail::channel_accessor<Channel>,
	Args...>
{};

template<typename Channel, typename... Args>
constexpr bool is_applicable_v = is_applicable<Channel, Args...>::value;

} // namespace channels
