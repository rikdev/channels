#pragma once
#include "connection.h"
#include "detail/compatibility/compile_features.h"
#include "detail/compatibility/shared_mutex.h"
#include "detail/shared_state.h"
#include "detail/type_traits.h"
#include "error.h"
#include <exception>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>

namespace channels {

/// The class `buffered_channel` is similar to `channels::channel` but it stores the last value sent.
template<typename... Ts>
class buffered_channel {
#ifdef CHANNELS_CPP_LIB_LOGICAL_TRAITS
	static_assert(
		!std::disjunction_v<std::is_reference<Ts>...>,
		"Instantiation of buffered_channel with a reference type is ill-formed");
#endif

	template<typename... Us>
	friend bool operator==(const buffered_channel<Us...>& lhs, const buffered_channel<Us...>& rhs) noexcept; // NOLINT
	template<typename... Us>
	friend bool operator!=(const buffered_channel<Us...>& lhs, const buffered_channel<Us...>& rhs) noexcept; // NOLINT

	class shared_state;

public:
	/// Looks like `std::optional<std::tuple<Ts...>>`
	using shared_value_type = typename shared_state::shared_value_type;

	/// \see channels::is_applicable
	template<typename... Args>
	static constexpr bool is_applicable = std::is_constructible<shared_value_type, cow::in_place_t, Args...>::value;

	/// \see channels::channel::channel
	buffered_channel() = default;

	/// \see channels::channel::channel
	CHANNELS_NODISCARD bool is_valid() const noexcept;

	/// This method is similar to method `channel::connect` but if the `buffered_channel` object has a value
	/// then this method will call the callback function with buffered value.
	/// \see get_value
	template<typename Callback>
	CHANNELS_NODISCARD connection connect(Callback&& callback) const;

	/// This method is similar to method `channel::connect` but if the `buffered_channel` object has a value
	/// then this method will call the callback function with buffered value.
	/// \see get_value
	template<typename Executor, typename Callback>
	CHANNELS_NODISCARD connection connect(Executor&& executor, Callback&& callback) const;

	/// Returns buffered value.
	/// \note If the `buffered_channel` object hasn't a value then `static_cast<bool>(get_value()) == false` otherwise
	///       `static_cast<bool>(get_value()) == false`
	/// \throw channel_error If `is_valid() == false`.
	CHANNELS_NODISCARD shared_value_type get_value() const;

protected:
	struct make_shared_state_tag {};

	explicit buffered_channel(make_shared_state_tag);

	/// This method is similar to method `channel::send` but it also keeps args in the `buffered_channel` object.
	/// \see get_value
	/// \warning Calling this method from the callback function will deadlock. Except when the executor breaks the
	///          stack.
	template<typename... Args>
	void send(Args&&... args);

private:
	template<typename... Args>
	CHANNELS_NODISCARD connection connect_impl(Args&&... args) const;

	std::shared_ptr<shared_state> shared_state_;
};

// implementation

template<typename Channel>
struct channel_traits;

template<typename... Ts>
struct channel_traits<buffered_channel<Ts...>> {
	static constexpr bool is_channel = true;
};

template<typename... Ts>
class buffered_channel<Ts...>::shared_state : public detail::shared_state<Ts...> {
	struct emplace_in_place_tag {};
	struct emplace_out_place_tag {};

public:
	using typename detail::shared_state<Ts...>::shared_value_type;

	using shared_value_mutex_type = detail::compatibility::shared_mutex;
	using shared_value_unique_lock_type = std::unique_lock<shared_value_mutex_type>;
	using shared_value_shared_lock_type = std::shared_lock<shared_value_mutex_type>;

	template<typename... Args>
	std::pair<shared_value_type, shared_value_unique_lock_type> emplace_value(Args&&... args)
	{
		shared_value_unique_lock_type value_lock{value_mutex_};

		using emplace_tag = std::conditional_t<
			detail::is_move_assignable<typename shared_value_type::value_type>::value,
			emplace_out_place_tag,
			emplace_in_place_tag>;
		emplace_value_impl(emplace_tag{}, std::forward<Args>(args)...);

		return {value_, std::move(value_lock)};
	}

	CHANNELS_NODISCARD std::pair<shared_value_type, shared_value_shared_lock_type> get_value() const
	{
		shared_value_shared_lock_type value_lock{value_mutex_};
		return {value_, std::move(value_lock)};
	}

private:
	template<typename... Args>
	void emplace_value_impl(emplace_in_place_tag, Args&&... args)
	{
		value_ = shared_value_type{cow::in_place, std::forward<Args>(args)...};
	}

	template<typename... Args>
	void emplace_value_impl(emplace_out_place_tag, Args&&... args)
	{
		value_ = typename shared_value_type::value_type{std::forward<Args>(args)...};
	}

	mutable shared_value_mutex_type value_mutex_;
	shared_value_type value_;
};

template<typename... Ts>
template<typename Callback>
connection buffered_channel<Ts...>::connect(Callback&& callback) const
{
	return connect_impl(std::forward<Callback>(callback));
}

template<typename... Ts>
template<typename Executor, typename Callback>
connection buffered_channel<Ts...>::connect(Executor&& executor, Callback&& callback) const
{
	return connect_impl(std::forward<Executor>(executor), std::forward<Callback>(callback));
}

template<typename... Ts>
bool buffered_channel<Ts...>::is_valid() const noexcept
{
	return static_cast<bool>(shared_state_);
}

template<typename... Ts>
typename buffered_channel<Ts...>::shared_value_type buffered_channel<Ts...>::get_value() const
{
	if (!is_valid())
		throw channel_error{"buffered_channel: has no state"};

	return shared_state_->get_value().first;
}

template<typename... Ts>
buffered_channel<Ts...>::buffered_channel(make_shared_state_tag)
	: shared_state_{std::make_shared<shared_state>()}
{}

template<typename... Ts>
template<typename... Args>
void buffered_channel<Ts...>::send(Args&&... args)
{
	static_assert(is_applicable<Args...>, "Channel parameters must be constructible from Args");

	shared_value_type shared_value;
	typename shared_state::invocable_sockets_shared_view sockets_view;

	{
		typename shared_state::shared_value_unique_lock_type shared_value_lock;

		std::tie(shared_value, shared_value_lock) = shared_state_->emplace_value(std::forward<Args>(args)...);
		// get sockets under the shared_value_lock in order to avoid duplication of the message in the callback function
		// when it is called from the connect method
		sockets_view = shared_state_->get_sockets();
	}

	callbacks_exception::exceptions_type exceptions;

	for (typename shared_state::invocable_socket& socket : sockets_view) {
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

template<typename... Ts>
template<typename... Args>
connection buffered_channel<Ts...>::connect_impl(Args&&... args) const
{
	if (!is_valid())
		throw channel_error{"buffered_channel: has no state"};

	typename shared_state::invocable_socket *socket = nullptr;

	{
		shared_value_type shared_value;
		// shared lock allows calling the connect method from the callback
		typename shared_state::shared_value_shared_lock_type shared_value_lock;
		std::tie(shared_value, shared_value_lock) = shared_state_->get_value();
		socket = &shared_state_->connect(std::forward<Args>(args)...);

		if (shared_value)
			(*socket)(std::move(shared_value));
	}

	return connection{shared_state_, *socket};
}

template<typename... Us>
bool operator==(const buffered_channel<Us...>& lhs, const buffered_channel<Us...>& rhs) noexcept
{
	return lhs.shared_state_ == rhs.shared_state_;
}

template<typename... Us>
bool operator!=(const buffered_channel<Us...>& lhs, const buffered_channel<Us...>& rhs) noexcept
{
	return !(lhs == rhs);
}

} // namespace channels
