#pragma once
#include "channel.h"
#include "continuation_status.h"
#include "detail/compatibility/apply.h"
#include "detail/compatibility/compile_features.h"
#include "detail/future_shared_state.h"
#include "fwd.h"
#include <cassert>
#include <exception>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>

namespace channels {

namespace aggregating_channel_detail {

template<typename R, typename... Ts>
class execution_shared_state_interface;

} // namespace aggregating_channel_detail

#if __cpp_concepts
template<typename T, typename R>
concept ChannelAggregator = requires(T a, R r) {
	{a.apply_result(std::move(r))} -> continuation_status;
	{a.apply_exception(std::exception_ptr{})} -> continuation_status;
};
#endif

/// The class `aggregating_channel` is similar to `channels::channel` but it can collect return values from callback
/// functions.
/// \tparam R Return value type of callback functions
/// \tparam Ts Types of parameters passed to callback functions.
template<typename R, typename... Ts>
class aggregating_channel<R(Ts...)>
	: protected channel<std::shared_ptr<aggregating_channel_detail::execution_shared_state_interface<R, Ts...>>> {

	template<typename F>
	friend bool operator==(const aggregating_channel<F>& lhs, const aggregating_channel<F>& rhs) noexcept; // NOLINT
	template<typename F>
	friend bool operator!=(const aggregating_channel<F>& lhs, const aggregating_channel<F>& rhs) noexcept; // NOLINT

	using execution_shared_state_interface = aggregating_channel_detail::execution_shared_state_interface<R, Ts...>;
	using base_type = channel<std::shared_ptr<execution_shared_state_interface>>;

	template<typename... Args>
	struct is_applicable_type;

public:
	/// Return value type of callback functions
	using aggregator_argument_type = R;

	/// \see channels::is_applicable
	template<typename... Args>
	static constexpr bool is_applicable = is_applicable_type<Args...>::value;

	/// \see channels::channel::channel
	aggregating_channel() = default;

	/// \see channels::channel::is_valid
	CHANNELS_NODISCARD bool is_valid() const noexcept;

	/// \see channels::channel::connect
	template<typename Callback>
	CHANNELS_NODISCARD connection connect(Callback&& callback) const;

	/// \see channels::channel::connect
	template<typename Executor, typename Callback>
	CHANNELS_NODISCARD connection connect(Executor&& executor, Callback&& callback) const;

protected:
	using base_type::base_type;

	/// This method is similar to method `channel::apply_value` but it additionally has the parameter `aggregator`.
	/// `Aggregator` is used to combine return values and exceptions from callback functions. If the callback function
	/// returns a value then the `aggregating_channel` calls the `apply_result` method on the `aggregator`.
	/// If the callback function throws a exception then the `aggregating_channel` calls the `apply_exception` method on
	/// the `aggregator`.
	/// If the aggregator method returns `continuation_status::to_continue`, the `aggregating_channel` calls the next
	/// callback function; otherwise this method stops execution and returns the aggregator to the future.
	/// \note The aggregator is protected by a mutex so if the executors call the callback functions from different
	///       threads they will wait for the queue to access the aggregator.
	/// \param aggregator Reference to the aggregator. Aggregator type must match the concept `ChannelAggregator`.
	/// \param args Arguments to pass to the callback functions.
	///             Types `Args` must be convertible to template channel parameters `Ts`.
	/// \return Future to aggregator. When all callback functions in all executors are completed, the future will be
	///         ready. If the aggregator throws an exception it will be returned to the future.
	/// \pre `is_valid() == true`. The behavior is undefined if `is_valid() == false` before the call to this method.
	template<typename Aggregator, typename... Args>
	CHANNELS_NODISCARD std::future<std::decay_t<Aggregator>> apply_value(Aggregator&& aggregator, Args&&... args);

private:
	template<typename Callback>
	class aggregating_callback;
};

// implementation

// channel_traits

template<typename Channel>
struct channel_traits;

template<typename F>
struct channel_traits<aggregating_channel<F>> {
	static constexpr bool is_channel = true;
};

// aggregating_channel::is_applicable_type

template<typename R, typename... Ts>
template<typename... Args>
struct aggregating_channel<R(Ts...)>::is_applicable_type : std::false_type {};

// \todo add Aggregator checking
template<typename R, typename... Ts>
template<typename Aggregator, typename... Args>
struct aggregating_channel<R(Ts...)>::is_applicable_type<Aggregator, Args...>
	: std::is_constructible<std::tuple<Ts...>, Args... >
{};

namespace aggregating_channel_detail {

// execution_shared_state derived lattice
//  execution_shared_state_interface<R, Ts...>  ->  execution_shared_state_result_interface_base<R>  -> execution_shared_state_interface_base
//                   ^                                                     ^                                              ^
//                   |                                                     |                                              |
// execution_shared_state<Aggregator, R, Ts...> -> execution_shared_state_result_base<Aggregator, R> -> execution_shared_state_base<Aggregator>

struct execution_shared_state_interface_base {
	virtual ~execution_shared_state_interface_base() = default;
	virtual void apply_exception(std::exception_ptr callback_exception) = 0;
	CHANNELS_NODISCARD virtual bool is_ready() const noexcept = 0;
};

template<typename R>
struct execution_shared_state_result_interface_base : virtual execution_shared_state_interface_base {
	virtual void apply_result(R&&) = 0;
};

template<>
struct execution_shared_state_result_interface_base<void> : virtual execution_shared_state_interface_base {
	virtual void apply_result() = 0;
};

template<typename R, typename... Ts>
class execution_shared_state_interface
	: public virtual execution_shared_state_result_interface_base<R>
	, private std::tuple<Ts...> // empty base optimization
{
public:
	using arguments_type = std::tuple<Ts...>;

	CHANNELS_NODISCARD const arguments_type& get_arguments() const noexcept
	{
		return *this;
	}

protected:
	template<typename... Args>
	explicit execution_shared_state_interface(Args&&... args)
		: arguments_type{std::forward<Args>(args)...}
	{}
};

template<typename Aggregator>
class execution_shared_state_base : public virtual execution_shared_state_interface_base {
	static_assert(
		std::is_nothrow_move_constructible<Aggregator>::value && std::is_nothrow_move_assignable<Aggregator>::value,
		"Aggregator must be nothrow movable or copyable");

public:
	execution_shared_state_base(const execution_shared_state_base&) = delete;
	execution_shared_state_base(execution_shared_state_base&&) = delete;
	execution_shared_state_base& operator=(const execution_shared_state_base&) = delete;
	execution_shared_state_base& operator=(execution_shared_state_base&&) = delete;

	~execution_shared_state_base()
	{
		if (!is_ready())
			future_shared_state_.make_ready();
	}

	void apply_exception(std::exception_ptr callback_exception) final
	{

		try {
			const std::unique_lock<std::mutex> aggregator_lock = get_aggregator_lock();
			if (!aggregator_lock)
				return;

			const continuation_status aggregator_result =
				get_aggregator().apply_exception(std::move(callback_exception));
			apply_aggregator_result(aggregator_result);
		}
		catch (const std::exception&) {
			apply_aggregator_exception(std::current_exception());
		}
	}

	CHANNELS_NODISCARD bool is_ready() const noexcept final
	{
		return future_shared_state_.is_ready();
	}

	CHANNELS_NODISCARD std::future<Aggregator> get_future()
	{
		return future_shared_state_.get_future();
	}

protected:
	template<typename A>
	explicit execution_shared_state_base(A&& aggregator)
		: future_shared_state_{std::forward<A>(aggregator)}
	{}

	CHANNELS_NODISCARD std::unique_lock<std::mutex> get_aggregator_lock()
	{
		if (is_ready())
			return std::unique_lock<std::mutex>{};

		std::unique_lock<std::mutex> aggregator_lock{aggregator_mutex_};

		if (is_ready())
			return std::unique_lock<std::mutex>{};

		return aggregator_lock;
	}

	void apply_aggregator_result(const continuation_status continuation)
	{
		assert(!is_ready()); // NOLINT

		if (continuation == continuation_status::stop)
			future_shared_state_.make_ready();
	}

	void apply_aggregator_exception(std::exception_ptr exception)
	{
		assert(exception); // NOLINT
		assert(!is_ready()); // NOLINT

		future_shared_state_.make_ready(std::move(exception));
	}

	CHANNELS_NODISCARD Aggregator& get_aggregator() noexcept
	{
		assert(!is_ready()); // NOLINT

		return future_shared_state_.get_value();
	}

private:
	std::mutex aggregator_mutex_;
	detail::future_shared_state<Aggregator> future_shared_state_;
};

#ifdef _MSC_VER
#pragma warning(push)
// https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4250
#pragma warning(disable: 4250)
#endif

template<typename Aggregator, typename R>
class execution_shared_state_result_base
	: public virtual execution_shared_state_result_interface_base<R>
	, public execution_shared_state_base<Aggregator> {
public:
	void apply_result(R&& result) final
	{
		const std::unique_lock<std::mutex> aggregator_lock = this->get_aggregator_lock();
		if (!aggregator_lock)
			return;

		try {
			const continuation_status aggregator_result = this->get_aggregator().apply_result(std::move(result));
			this->apply_aggregator_result(aggregator_result);
		}
		catch (...) {
			this->apply_aggregator_exception(std::current_exception());
		}
	}

protected:
	using execution_shared_state_base<Aggregator>::execution_shared_state_base;
};

template<typename Aggregator>
class execution_shared_state_result_base<Aggregator, void>
	: public virtual execution_shared_state_result_interface_base<void>
	, public execution_shared_state_base<Aggregator> {
public:
	void apply_result() final
	{
		const std::unique_lock<std::mutex> aggregator_lock = this->get_aggregator_lock();
		if (!aggregator_lock)
			return;

		try {
			const continuation_status aggregator_result = this->get_aggregator().apply_result();
			this->apply_aggregator_result(aggregator_result);
		}
		catch (...) {
			this->apply_aggregator_exception(std::current_exception());
		}
	}

protected:
	using execution_shared_state_base<Aggregator>::execution_shared_state_base;
};

template<typename Aggregator, typename R, typename... Ts>
class execution_shared_state final
	: public execution_shared_state_interface<R, Ts...>
	, public execution_shared_state_result_base<Aggregator, R>
{
public:
	template<typename A, typename... Args>
	explicit execution_shared_state(A&& aggregator, Args&&... args)
		: execution_shared_state_interface<R, Ts...>{std::forward<Args>(args)...}
		, execution_shared_state_result_base<Aggregator, R>{std::forward<A>(aggregator)}
	{}
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace aggregating_channel_detail

template<typename R, typename... Ts>
template<typename Callback>
class aggregating_channel<R(Ts...)>::aggregating_callback {
	// Fake parameter is needed to avoid the GCC compilation error: "explicit specialization on non-namespace scope"
	template<typename T, typename Fake = void>
	struct binder {
#if __cpp_lib_is_invocable
		static_assert(
			std::is_convertible<std::invoke_result_t<Callback, Ts...>, R>::value,
			"R must be convertible from Callback return value");
#endif
		void operator()(execution_shared_state_interface& shared_state) const
		{
			decltype(auto) result = detail::compatibility::apply(callback, shared_state.get_arguments());
			shared_state.apply_result(std::move(result));
		}

		Callback callback; // NOLINT(misc-non-private-member-variables-in-classes)
	};

	template<typename Fake>
	struct binder<void, Fake> {
		void operator()(execution_shared_state_interface& shared_state) const
		{
			detail::compatibility::apply(callback, shared_state.get_arguments());
			shared_state.apply_result();
		}

		Callback callback; // NOLINT(misc-non-private-member-variables-in-classes)
	};

public:
	template<typename F>
	explicit aggregating_callback(F&& callback) noexcept(std::is_nothrow_move_constructible<F>::value)
		: binder_{std::forward<F>(callback)}
	{}

	void operator()(const std::shared_ptr<execution_shared_state_interface>& shared_state) const
	{
		assert(shared_state); // NOLINT

		if (shared_state->is_ready())
			return;

		try {
			binder_(*shared_state);
		}
		catch (...) {
			shared_state->apply_exception(std::current_exception());
		}
	}

private:
	binder<R> binder_;
};

template<typename R, typename... Ts>
template<typename Callback>
connection aggregating_channel<R(Ts...)>::connect(Callback&& callback) const
{
	aggregating_callback<std::decay_t<Callback>> aggregating_callback{std::forward<Callback>(callback)};
	return base_type::connect(std::move(aggregating_callback));
}

template<typename R, typename... Ts>
template<typename Executor, typename Callback>
connection aggregating_channel<R(Ts...)>::connect(Executor&& executor, Callback&& callback) const
{
	aggregating_callback<std::decay_t<Callback>> aggregating_callback{std::forward<Callback>(callback)};
	return base_type::connect(std::forward<Executor>(executor), std::move(aggregating_callback));
}

template<typename R, typename... Ts>
bool aggregating_channel<R(Ts...)>::is_valid() const noexcept
{
	return base_type::is_valid();
}

template<typename R, typename... Ts>
template<typename Aggregator, typename... Args>
std::future<std::decay_t<Aggregator>> aggregating_channel<R(Ts...)>::apply_value(
	Aggregator&& aggregator, Args&&... args)
{
	using execution_shared_state_type =
		aggregating_channel_detail::execution_shared_state<std::decay_t<Aggregator>, R, Ts...>;
	auto execution_shared_state =
		std::make_shared<execution_shared_state_type>(
			std::forward<Aggregator>(aggregator), std::forward<Args>(args)...);
	auto future = execution_shared_state->get_future();

	base_type::apply_value(std::move(execution_shared_state));
	return future;
}

template<typename F>
bool operator==(const aggregating_channel<F>& lhs, const aggregating_channel<F>& rhs) noexcept
{
	using base_type = typename aggregating_channel<F>::base_type;

	return static_cast<const base_type&>(lhs) == static_cast<const base_type&>(rhs);
}

template<typename F>
bool operator!=(const aggregating_channel<F>& lhs, const aggregating_channel<F>& rhs) noexcept
{
	return !(lhs == rhs);
}

} // namespace channels
