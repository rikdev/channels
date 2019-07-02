#pragma once
#include "../channel_traits.h"
#include "../connection.h"
#include "../detail/compatibility/apply.h"
#include "../detail/compatibility/compile_features.h"
#include "../detail/compatibility/functional.h"
#include "../detail/compatibility/type_traits.h"
#include "../detail/type_traits.h"
#include "../transmitter.h"
#include <utility>

namespace channels {
namespace utility {

/// A class `transponder` receives a value from source channel and dispatch it to callback function which can process
/// or filter this value and send result to the destination channel.
///
/// Data transfer graph:
///                      __________________________________________
///                     |                transponder               |
///  ________________   |   __________      _____________________  |
/// | source channel |--|->| callback |--->| destination channel |-|--> consumers
/// |________________|  |  |__________|    |_____________________| |
///                     |__________________________________________|
///
/// \tparam Channel A type of destination channel object.
///
/// Example:
/// \code
/// struct mercator_point { double x; double y; };
/// struct geo_point { double lat; double lon; };
/// bool is_correct(const geo_point& point) noexcept;
/// ...
/// channels::transponder<channels::channel<geo_point>> location_source;
/// ...
/// using map_point_channel_type = channels::buffered_channel<mercator_point>;
/// channels::utility::transponder<map_point_channel_type> map_point_source{
/// 	location_source.get_channel(),
/// 	[](auto& transmitter, const geo_point& point) {
/// 		if (is_correct(point))
/// 			transmitter(to_mercator(point)); }
/// };
/// ...
/// map_point_channel_type map_point_channel = map_point_source.get_channel();
/// ...
/// \endcode
template<typename Channel>
class transponder {
public:
	/// Type of destination channel.
	using channel_type = Channel;
	/// Type of transmitter that passed to callback function.
	using transmitter_type = transmitter<channel_type>;

	/// Default constructor.
	/// \post `get_channel().is_valid() == false`.
	transponder() = default;

	/// Constructs `transponder` object and connects to source_channel.
	/// \see transponder::assign
	/// \post `get_channel().is_valid() == true`.
	template<typename SourceChannel, typename Callback>
	transponder(const SourceChannel& source_channel, Callback callback);

	/// Constructs `transponder` object and connects to `source_channel`.
	/// \see transponder::assign
	/// \post `get_channel().is_valid() == true`.
	template<typename SourceChannel, typename Executor, typename Callback>
	transponder(const SourceChannel& source_channel, Executor executor, Callback callback);

	/// Connects callback to source_channel.
	/// \param source_channel Reference to which the callback function will be connected.
	/// \warning source_channel object must be valid.
	/// \param callback A callback function.
	///                 Callback type must match the concept `std::Invocable<Callback, transmitter_type&, Ts...>` where
	///                 transmitter_type See definition of the `transmitter_type` in this class.
	///                 Ts - types of parameters for source channel.
	/// \post `get_channel().is_valid() == true`.
	template<typename SourceChannel, typename Callback>
	void assign(const SourceChannel& source_channel, Callback callback);

	/// Same as previous method `transponder::assign` but additionally has the parameter executor.
	/// \see `channel::connect`.
	/// \post `get_channel().is_valid() == true`.
	template<typename SourceChannel, typename Executor, typename Callback>
	void assign(const SourceChannel& source_channel, Executor executor, Callback callback);

	/// Disconnect from source channel and reset destination channel.
	/// \post `get_channel().is_valid() == false`.
	void reset() noexcept;

	/// Return reference to the destination channel.
	CHANNELS_NODISCARD const Channel& get_channel() const noexcept;

private:
	template<typename Callback>
	class reactive_transmitter;

	channel_type channel_;
	connection connection_;
};

// adaptors

/// Creates an adaptor for the class `transponder` that passes `transform_function` return value to the transmitter.
/// \param transform_function A function that receives value from transponder source channel,
///                           processes it and return result of processing.
///                           Function type must match the concept `std::Invocable<Callback, Ts...>` where
///                           Ts - types of parameters for transponder source channel.
///                           Function must return either void or std::tuple<Ds...> where
///                           Ds - types of parameters for transponder destination channel.
///
/// Example:
/// \code
/// channels::transponder<channels::channel<std::string>> lexeme_source;
/// ...
/// channels::utility::transponder<channels::channel<token_type, std::string>> tokenized_lexeme_source{
/// 	lexeme_source.get_channel(),
/// 	[](const std::string& lexeme) { return std::make_tuple(tokenize(lexeme), lexeme); }
/// };
/// ...
/// \endcode
template<typename Callback>
CHANNELS_NODISCARD constexpr auto make_transform_adaptor(Callback transform_function);

/// Creates an adaptor for the class `transponder` that passes arguments to the transmitter
/// if `filter_predicate` function return true.
/// \param filter_predicate A predicate function.
///                         Callback type must match the concept `std::Predicate<Callback, Ts...>` where
///                         Ts - types of parameters for transponder source channel.
///
/// Example:
/// \code
/// using log_channel_type = channels::channel<std::string>;
/// channels::transponder<log_channel_type> log_source;
/// ...
/// channels::utility::transponder<log_channel_type> insensitive_log_source{
/// 	log_source.get_channel(),
/// 	[](const std::string& log) { return !has_sensitive_data(log); }
/// };
/// ...
/// \endcode
template<typename Callback>
CHANNELS_NODISCARD constexpr auto make_filter_adaptor(Callback filter_predicate);

// implementation

// transponder

template<typename Channel>
template<typename SourceChannel, typename Callback>
transponder<Channel>::transponder(const SourceChannel& source_channel, Callback callback)
{
	assign(source_channel, std::move(callback));
}

template<typename Channel>
template<typename SourceChannel, typename Executor, typename Callback>
transponder<Channel>::transponder(const SourceChannel& source_channel, Executor executor, Callback callback)
{
	assign(source_channel, std::move(executor), std::move(callback));
}

template<typename Channel>
template<typename SourceChannel, typename Callback>
void transponder<Channel>::assign(const SourceChannel& source_channel, Callback callback)
{
	static_assert(is_channel_v<SourceChannel>, "SourceChannel must be channel");

	auto transmitter = reactive_transmitter<Callback>(std::move(callback));
	channel_ = transmitter.get_channel();
	connection_ = source_channel.connect(std::move(transmitter));
}

template<typename Channel>
template<typename SourceChannel, typename Executor, typename Callback>
void transponder<Channel>::assign(const SourceChannel& source_channel, Executor executor, Callback callback)
{
	static_assert(is_channel_v<SourceChannel>, "SourceChannel must be channel");

	auto transmitter = reactive_transmitter<Callback>(std::move(callback));
	channel_ = transmitter.get_channel();
	connection_ = source_channel.connect(std::move(executor), std::move(transmitter));
}

template<typename Channel>
const Channel& transponder<Channel>::get_channel() const noexcept
{
	return channel_;
}

template<typename Channel>
void transponder<Channel>::reset() noexcept
{
	connection_.disconnect();
	channel_ = channel_type{};
}

// transponder::reactive_transmitter

template<typename Channel>
template<typename Callback>
class transponder<Channel>::reactive_transmitter {
public:
	constexpr explicit reactive_transmitter(Callback callback)
		: callback_{std::move(callback)}
	{}

	template<typename... Args>
	decltype(auto) operator()(Args&&... args)
	{
		return detail::compatibility::invoke(callback_, transmitter_, std::forward<Args>(args)...);
	}

	CHANNELS_NODISCARD const Channel& get_channel() const noexcept
	{
		return transmitter_.get_channel();
	}

private:
	Callback callback_;
	transmitter_type transmitter_;
};

// transform_adaptor

namespace transponder_detail {

template<typename Function>
class transform_adaptor {
	template<typename... Args>
	using function_result_type = detail::compatibility::invoke_result_t<Function, Args...>;

public:
	constexpr explicit transform_adaptor(Function transform_function)
		: transform_function_{std::move(transform_function)}
	{}

	template<typename Transmitter, typename... Args>
	std::enable_if_t<std::is_void<function_result_type<Args...>>::value> operator()(
		Transmitter& transmitter, Args&&... args)
	{
		detail::compatibility::invoke(transform_function_, std::forward<Args>(args)...);
		transmitter();
	}

	template<typename Transmitter, typename... Args>
	std::enable_if_t<!std::is_void<function_result_type<Args...>>::value> operator()(
		Transmitter& transmitter, Args&&... args)
	{
		// transform_function_ must return tuple object
		decltype(auto) result = detail::compatibility::invoke(transform_function_, std::forward<Args>(args)...);
		detail::compatibility::apply(transmitter, std::move(result));
	}

private:
	Function transform_function_;
};

} // namespace transponder_detail

template<typename Function>
constexpr auto make_transform_adaptor(Function transform_function)
{
	return transponder_detail::transform_adaptor<Function>{std::move(transform_function)};
}

// filter_adaptor

template<typename Function>
constexpr auto make_filter_adaptor(Function filter_predicate)
{
	return [predicate = std::move(filter_predicate)](auto& transmitter, auto&&... args) mutable {
			if (detail::compatibility::invoke(predicate, args...))
				transmitter(std::forward<decltype(args)>(args)...);
		};
}

} // namespace utility
} // namespace channels
