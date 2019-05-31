#pragma once
#include "connection.h"
#include "detail/compatibility/compile_features.h"
#include "detail/shared_state.h"
#include "error.h"
#include <cassert>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace channels {

/// The class `channel` provides a non-buffered signal/slot system implementation:
/// - The provider can provide a channel object (created via `channels::transmitter`) to consumer.
/// - The receiver subscribes to events from the channel object.
///   To do this, it passes its callback function to the channel method `connect`.
/// - The provider sends the values to the transmitter object. After the channel calls the callback functions
///   provided by the consumers and passes these values to its.
/// \note A channel objects shares one shared state between all its copies and the transmitter object.
///       The lifetime of the shared state lasts at least until the last object with which it is associated is destroyed.
///       Thus the `channel` has pointer semantics.
/// \note A channel objects are only useful when they are valid. Default-constructed channel objects are not valid.
///
/// Example:
/// \code
/// using MouseClickChannel = channels::channel<Position, Button>;
/// class MouseArea {
/// public:
/// 	MouseClickChannel get_mouse_click_channel() const noexcept { return mouse_click_transmitter_; }
/// ...
/// private:
/// 	channels::transmitter<MouseClickChannel> mouse_click_transmitter_;
/// };
/// ...
/// using ButtonClickChannel = channels::channel<>;
/// class Button {
/// public:
/// 	Button(const Position& pos, const Size& size, const MouseArea& mouse_area ...)
/// 		: pos_{pos}, size_{size} ...
/// 	{
/// 		mouse_click_connection_ = mouse_area.get_mouse_click_channel().connect(
/// 			[this](const Position& click_pos, Button button) { on_mouse_click(click_pos, button); });
/// 		...
/// 	}
///
/// 	void on_mouse_click(const Position& click_pos, Button button) {
/// 		if (button == Button::Left && Rectangle(pos_, size_).include(click_pos))
/// 			click_transmitter_();
/// 	}
///
/// 	ButtonClickChannel get_click_channel() const noexcept { return click_transmitter_; }
/// ...
/// private:
/// 	channels::connection mouse_click_connection_;
/// 	channels::transmitter<ButtonClickChannel> click_transmitter_;
/// };
/// \endcode
///
/// \tparam Ts Types of parameters passed to callback functions.
template<typename... Ts>
class channel {
	template<typename... Us>
	friend bool operator==(const channel<Us...>& lhs, const channel<Us...>& rhs) noexcept; // NOLINT
	template<typename... Us>
	friend bool operator!=(const channel<Us...>& lhs, const channel<Us...>& rhs) noexcept; // NOLINT

public:
	/// Constructs a `channel` object with no shared state.
	/// \post `is_valid() == false`.
	channel() = default;

	/// Checks if the `channel` refers to a shared state.
	CHANNELS_NODISCARD bool is_valid() const noexcept;

	/// Adds callback function to be called when `channels::transmitter` sends values to the channel object.
	/// \note This method is thread safe.
	/// \param callback Reference to the callback function.
	///                 Callback type must match the concept `std::Invocable<Callback, Ts...>`.
	/// \warning Calling this method from the callback function will deadlock.
	/// \return A `channels::connection` object that controls the current connection.
	/// \warning When connection object is destroyed the connection will be disconnected.
	/// \throw channel_error If `is_valid() == false`.
	/// \throws Any exception thrown by the copy or move constructors of callback.
	/// \pre `is_valid() == true`.
	template<typename Callback>
	CHANNELS_NODISCARD connection connect(Callback&& callback);

	/// Same as previous method `channel::connect` but additionally has the parameter executor.
	/// When the transmitter sends values, the channel wraps them and the callback into a functional object with an
	/// operator() without arguments (let's call it a task) and calls the user-defined function with the signature
	/// `execute(const Executor&, std::function<void()>&)` (often this function schedules the task to execution in
	/// another thread).
	/// \note Task can be safely invoked after a channel object is destroyed.
	/// \warning Task can be invoked only once. Other invokes will have no effect.
	/// \warning Calling this method from either the callback function or `execute` function will deadlock.
	/// \param executor Reference to the executor object. You must implement function
	///                 `execute(const Executor&, std::function<void()>&)` to bind your executor with this library.
	/// \param callback See previous method `connect`.
	/// \return See previous method `connect`.
	/// \throws Any exception from previous method `connect` and any exception thrown by the copy or move
	///         constructors of `executor`.
	/// \pre `is_valid() == true`.
	template<typename Executor, typename Callback>
	CHANNELS_NODISCARD connection connect(Executor&& executor, Callback&& callback);

protected:
	struct make_shared_state_tag {};

	/// Constructs a channel object with shared state.
	/// \post `is_valid() == true`.
	explicit channel(make_shared_state_tag);

	/// Calls all connected callback functions and passes arguments to them.
	/// \note This method is thread safe.
	/// \param args Arguments to pass to the callback functions.
	///             Types `Args` must be convertible to template channel parameters `Ts`.
	/// \throw callbacks_exception If one or more either callback function or `execute` function threw exceptions.
	/// \note If the callback function or the `execute` function throws an exception this method doesn't stop executing
	///       but calls the remaining callback functions and throws the `callbacks_exception`.
	/// \pre `is_valid() == true`. The behavior is undefined if `is_valid() == false` before the call to this method.
	template<typename... Args>
	void apply_value(Args&&... args);

private:
	using shared_state_type = detail::shared_state<Ts...>;
	using shared_value_type = typename shared_state_type::shared_value_type;

	std::shared_ptr<shared_state_type> shared_state_;
};

// implementation

template<typename Channel>
struct channel_traits;

template<typename... Ts>
struct channel_traits<channel<Ts...>> {
	static constexpr bool is_channel = true;
};

template<typename... Ts>
template<typename Callback>
connection channel<Ts...>::connect(Callback&& callback)
{
	if (!is_valid())
		throw channel_error{"channel: has no state"};

	auto socket = shared_state_->connect(std::forward<Callback>(callback)).socket;
	return connection{shared_state_, std::move(socket)};
}

template<typename... Ts>
template<typename Executor, typename Callback>
connection channel<Ts...>::connect(Executor&& executor, Callback&& callback)
{
	if (!is_valid())
		throw channel_error{"channel: has no state"};

	auto socket = shared_state_->connect(std::forward<Executor>(executor), std::forward<Callback>(callback)).socket;
	return connection{shared_state_, std::move(socket)};
}

template<typename... Ts>
bool channel<Ts...>::is_valid() const noexcept
{
	return static_cast<bool>(shared_state_);
}

template<typename... Ts>
channel<Ts...>::channel(make_shared_state_tag)
	: shared_state_{std::make_shared<shared_state_type>()}
{}

template<typename... Ts>
template<typename... Args>
void channel<Ts...>::apply_value(Args&&... args)
{
#if __cpp_lib_logical_traits
	static_assert(
		std::conjunction_v<std::is_constructible<Ts, Args>...>,
		"Channel parameters must be constructible from Args");
#endif
	assert(shared_state_); // NOLINT

	callbacks_exception::exceptions_type exceptions;
	const shared_value_type shared_value{detail::in_place, std::forward<Args>(args)...};
	for (typename shared_state_type::invocable_socket& socket : shared_state_->get_sockets()) {
		try {
			socket(shared_value);
		}
		catch (...) {
			exceptions.push_back(std::current_exception());
		}
	}

	if (!exceptions.empty())
		throw callbacks_exception{std::move(exceptions)};
}

template<typename... Us>
bool operator==(const channel<Us...>& lhs, const channel<Us...>& rhs) noexcept
{
	return lhs.shared_state_ == rhs.shared_state_;
}

template<typename... Us>
bool operator!=(const channel<Us...>& lhs, const channel<Us...>& rhs) noexcept
{
	return !(lhs == rhs);
}

} // namespace channels
